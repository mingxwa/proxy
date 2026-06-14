#!/usr/bin/env python3
# pyright: strict

"""Post-upgrade hook for the weekly Renovate bump: handle what Renovate has no manager for.

Renovate (renovate.json) bumps everything with a standard manager -- GitHub Actions,
pre-commit hooks, the mkdocs PyPI pins, MODULE.bazel (Bazel Central Registry) and
.bazelversion. It then runs this script (a postUpgradeTasks command) on the same branch to
handle, self-containedly, the three families it has no manager for:

  cmake   fmt / googletest / benchmark in cmake/dependencies.json and nlohmann_json in
          tools/report_generator/dependencies.json. For each, query the repo's latest release
          and -- only when it differs from the pinned tag -- download the tarball and rewrite
          its url + sha256 (the sha256 is the part Renovate cannot compute).
  meson   subprojects/*.wrap, refreshed via `meson wrap update`.
  bazel   MODULE.bazel.lock, refreshed to match the bazel_dep versions Renovate bumped.

The script is self-contained: it discovers versions and computes hashes itself rather than
relying on Renovate to pre-bump anything. Any problem -- a lookup or download that fails, a
missing tool, a failed subprocess -- is a hard error: the script exits non-zero, which
Renovate surfaces as a failed ``renovate/artifacts`` check on the pull request.

Progress and errors go to stdout. Only the Python standard library is used; GITHUB_TOKEN, if
present, lifts the GitHub rate limit.
"""

import hashlib
import json
import os
import re
import shutil
import subprocess
import urllib.error
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
    if token:
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


if __name__ == "__main__":
    _update_registry("cmake/dependencies.json", "cmake")
    _update_registry("tools/report_generator/dependencies.json", "report_generator")
    _update_meson()
    _refresh_bazel_lock()
