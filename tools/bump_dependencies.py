#!/usr/bin/env python3
# pyright: strict

"""Post-upgrade hook for the weekly Renovate bump: handle what Renovate has no manager for.

Renovate (renovate.json) bumps everything with a standard manager -- GitHub Actions,
pre-commit hooks, the mkdocs PyPI pins, MODULE.bazel (Bazel Central Registry) and
.bazelversion. It then runs this script (a postUpgradeTasks command) on the same branch to
handle, self-containedly, the families it has no manager for:

  cmake   fmt / googletest / benchmark in cmake/dependencies.json and nlohmann_json in
          tools/report_generator/dependencies.json. For each, query the repo's latest release
          and -- only when it differs from the pinned tag -- download the tarball and rewrite
          its url + sha256 (the sha256 is the part Renovate cannot compute).
  meson   subprojects/*.wrap, refreshed via `meson wrap update`.
  bazel   MODULE.bazel.lock, refreshed to match the bazel_dep versions Renovate bumped.
  ci      the CLANG_VERSION / NVHPC_VERSION env pins in .github/workflows/bvt-clang.yml and
          bvt-nvhpc.yml -- plain workflow env vars no manager understands -- bumped to the
          latest stable LLVM major and the latest NVIDIA HPC SDK release respectively.

The script is self-contained: it discovers versions and computes hashes itself rather than
relying on Renovate to pre-bump anything. Any problem -- a lookup or download that fails, a
missing tool, a failed subprocess -- is a hard error: the script exits non-zero, which
Renovate surfaces as a failed ``renovate/artifacts`` check on the pull request.

Progress and errors go to stdout. Only the Python standard library is used; GITHUB_TOKEN, if
present, lifts the GitHub API rate limit.
"""

import hashlib
import json
import os
import re
import shutil
import subprocess
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path
from typing import NoReturn

_REPO_ROOT = Path(__file__).resolve().parent.parent

# A GitHub archive tarball URL -> (owner/repo, tag).
_ARCHIVE_URL_RE = re.compile(
    r"https://github\.com/([^/]+/[^/]+)/archive/refs/tags/(.+)\.tar\.gz"
)
# A Meson wrap's resolved version, from its "directory = <name>-<version>" line.
_WRAP_DIR_RE = re.compile(r"^directory\s*=\s*(.+)$", re.MULTILINE)
# Marks a wrapdb-managed wrap; `meson wrap update` only handles these.
_WRAPDB_RE = re.compile(r"^wrapdb_version\s*=", re.MULTILINE)

# CI toolchain version pins (plain workflow env vars) and the sources that drive them.
_BVT_CLANG = ".github/workflows/bvt-clang.yml"
_BVT_NVHPC = ".github/workflows/bvt-nvhpc.yml"
_NVHPC_PACKAGES_URL = (
    "https://developer.download.nvidia.com/hpc-sdk/ubuntu/amd64/Packages"
)
_CLANG_VERSION_RE = re.compile(
    r"^([ \t]*CLANG_VERSION:[ \t]*)(\d+)[ \t]*$", re.MULTILINE
)
_NVHPC_VERSION_RE = re.compile(
    r"^([ \t]*NVHPC_VERSION:[ \t]*)(\S+)[ \t]*$", re.MULTILINE
)
_LLVM_TAG_RE = re.compile(r"^llvmorg-(\d+)\.")
_NVHPC_PKG_RE = re.compile(r"^Package:[ \t]*nvhpc-(\d+)-(\d+)[ \t]*$", re.MULTILINE)


def _log(msg: str) -> None:
    print(msg, flush=True)


def _abort(msg: str) -> NoReturn:
    print(f"error: {msg}", flush=True)
    raise SystemExit(1)


# --------------------------------------------------------------------------- #
# HTTP (stdlib only)
# --------------------------------------------------------------------------- #


def _http_get(url: str, *, accept: str | None = None) -> bytes | None:
    req = urllib.request.Request(url)
    req.add_header("User-Agent", "proxy-bump-dependencies")
    if accept is not None:
        req.add_header("Accept", accept)
    token = os.environ.get("GITHUB_TOKEN")
    if token and urllib.parse.urlparse(url).hostname in (
        "github.com",
        "api.github.com",
    ):
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            return resp.read()
    except (urllib.error.URLError, TimeoutError):
        return None


def _http_json(url: str) -> object | None:
    body = _http_get(url, accept="application/json")
    if body is None:
        return None
    try:
        return json.loads(body)
    except json.JSONDecodeError:
        return None


def _github_latest_tag(repo: str) -> str | None:
    """Latest published release tag for ``owner/repo``, or ``None`` if there is none."""
    data = _http_json(f"https://api.github.com/repos/{repo}/releases/latest")
    if isinstance(data, dict):
        name = data.get("tag_name")
        if isinstance(name, str):
            return name
    return None


# --------------------------------------------------------------------------- #
# Updaters
# --------------------------------------------------------------------------- #


def _update_registry(rel_path: str, label: str) -> None:
    """Bump each GitHub-archive entry to its latest release (downloading only on a change)."""
    _log(f"Checking {label} ({rel_path}) ...")
    path = _REPO_ROOT / rel_path
    deps = json.loads(path.read_text(encoding="utf-8"))
    changed = False
    for dep in deps:
        name, url = dep["name"], dep["url"]
        m = _ARCHIVE_URL_RE.fullmatch(url)
        if m is None:
            _abort(f"{label}/{name}: url is not a GitHub archive tarball: {url}")
        repo, current = m.group(1), m.group(2)
        latest = _github_latest_tag(repo)
        if latest is None:
            _abort(f"{label}/{name}: could not determine the latest release of {repo}")
        if latest == current:
            continue  # already current -- no download
        new_url = f"https://github.com/{repo}/archive/refs/tags/{latest}.tar.gz"
        body = _http_get(new_url)
        if body is None:
            _abort(f"{label}/{name}: could not download {new_url}")
        _log(f"  {name}: {current} -> {latest}")
        dep["url"] = new_url
        dep["sha256"] = hashlib.sha256(body).hexdigest()
        changed = True
    if changed:
        path.write_text(json.dumps(deps, indent=2) + "\n", encoding="utf-8")


def _wrap_version(wrap: Path) -> str:
    m = _WRAP_DIR_RE.search(wrap.read_text(encoding="utf-8"))
    return m.group(1).strip() if m else "?"


def _update_meson() -> None:
    _log("Updating Meson wraps ...")
    wraps = sorted((_REPO_ROOT / "subprojects").glob("*.wrap"))
    if not wraps:
        _abort("no wrap files found in subprojects/")
    if shutil.which("meson") is None:
        _abort("meson not found on PATH")
    for wrap in wraps:
        name = wrap.stem
        if _WRAPDB_RE.search(wrap.read_text(encoding="utf-8")) is None:
            _log(f"  {name}: not wrapdb-managed; skipping")
            continue
        before = _wrap_version(wrap)
        result = subprocess.run(
            ["meson", "wrap", "update", name],
            cwd=_REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            _abort(f"`meson wrap update {name}` failed: {result.stderr.strip()}")
        after = _wrap_version(wrap)
        if after != before:
            _log(f"  {name}: {before} -> {after}")


def _refresh_bazel_lock() -> None:
    _log("Refreshing MODULE.bazel.lock ...")
    if shutil.which("bazel") is None:
        _abort("bazel not found on PATH")
    result = subprocess.run(
        ["bazel", "mod", "graph", "--lockfile_mode=update"],
        cwd=_REPO_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        _abort(f"`bazel mod graph` failed: {result.stderr.strip()}")


def _replace_pin(
    rel_path: str, pattern: re.Pattern[str], latest: str, label: str
) -> None:
    """Rewrite the value (group 2) of ``pattern``'s single match in ``rel_path`` to ``latest``."""
    path = _REPO_ROOT / rel_path
    text = path.read_text(encoding="utf-8")
    m = pattern.search(text)
    if m is None:
        _abort(f"{label}: could not find the version pin in {rel_path}")
    current = m.group(2)
    if current == latest:
        return  # already current
    _log(f"  {label}: {current} -> {latest}")
    path.write_text(text[: m.start(2)] + latest + text[m.end(2) :], encoding="utf-8")


def _update_clang_version() -> None:
    """Pin CLANG_VERSION to the latest stable LLVM major (apt.llvm.org tracks per-major)."""
    _log(f"Checking CLANG_VERSION ({_BVT_CLANG}) ...")
    tag = _github_latest_tag("llvm/llvm-project")
    if tag is None:
        _abort("could not determine the latest LLVM release")
    m = _LLVM_TAG_RE.match(tag)
    if m is None:
        _abort(f"unexpected LLVM release tag format: {tag}")
    _replace_pin(_BVT_CLANG, _CLANG_VERSION_RE, m.group(1), "clang")


def _update_nvhpc_version() -> None:
    """Pin NVHPC_VERSION to the latest <major>.<minor> in the NVIDIA HPC SDK apt index."""
    _log(f"Checking NVHPC_VERSION ({_BVT_NVHPC}) ...")
    body = _http_get(_NVHPC_PACKAGES_URL)
    if body is None:
        _abort(f"could not download the NVHPC package index ({_NVHPC_PACKAGES_URL})")
    versions = {
        (int(a), int(b))
        for a, b in _NVHPC_PKG_RE.findall(body.decode("utf-8", "replace"))
    }
    if not versions:
        _abort("no nvhpc-<major>-<minor> packages found in the NVHPC index")
    major, minor = max(versions)
    _replace_pin(_BVT_NVHPC, _NVHPC_VERSION_RE, f"{major}.{minor}", "nvhpc")


if __name__ == "__main__":
    _update_registry("cmake/dependencies.json", "cmake")
    _update_registry("tools/report_generator/dependencies.json", "report_generator")
    _update_clang_version()
    _update_nvhpc_version()
    _update_meson()
    _refresh_bazel_lock()
