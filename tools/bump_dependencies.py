#!/usr/bin/env python3
# pyright: strict

"""Bump every in-scope dependency of the proxy repo to its latest version.

This is the engine behind the weekly ``pipeline-bump-dependencies.yml`` workflow. It
updates each dependency family in place, printing each change to stdout (``- name: old →
new``) as it goes. It never touches git; the workflow decides whether anything changed
and opens the pull request.

Domains handled:

  actions       GitHub Action pins in .github/workflows/*.yml (and the bazelisk binary)
  precommit     hook ``rev``s and local-hook tool pins in .pre-commit-config.yaml
  mkdocs        pinned PyPI packages in mkdocs/requirements.txt
  cpp           fmt / googletest / benchmark in cmake/dependencies.json
  meson         subprojects/*.wrap via ``meson wrap update``
  bazel         bazel_dep versions in MODULE.bazel (from the Bazel Central Registry)
  proxy_deps    http_archive pins in proxy_deps.bzl (legacy WORKSPACE shim)
  bazelversion  .bazelversion (latest bazelbuild/bazel release)

Run from the repository root:
  python3 tools/bump_dependencies.py

When done, MODULE.bazel.lock is refreshed with ``bazel mod graph --lockfile_mode=update``
(registry metadata only -- no source downloads, no build); the PR's CI does the actual
build.

Actionable warnings (a dependency left unbumped, a step skipped) are emitted as GitHub
Actions ``::warning::`` workflow commands so they surface as annotations on the run and
the PR (and are inert plain text anywhere else).

Only the Python standard library is used. Set ``GITHUB_TOKEN`` to lift the GitHub API
rate limit (the workflow passes the built-in token automatically).
"""

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import urllib.error
import urllib.request
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parent.parent


def _log(msg: str) -> None:
    print(msg, file=sys.stderr, flush=True)


def _warn(msg: str) -> None:
    """Emit a human-readable warning to stderr and a ``::warning::`` annotation to stdout.

    The annotation surfaces on the GitHub Actions run page and the PR; it is inert plain
    text outside Actions, so it is emitted unconditionally.
    """
    _log(f"  ! {msg}")
    print(f"::warning::{msg.replace(chr(10), ' ')}", flush=True)


def _record(name: str, old: str, new: str) -> None:
    print(f"- {name}: {old} → {new}", flush=True)


# --------------------------------------------------------------------------- #
# HTTP helpers (stdlib only)
# --------------------------------------------------------------------------- #


def _http_get(
    url: str, *, accept: str | None = None, auth: bool = False
) -> bytes | None:
    req = urllib.request.Request(url)
    req.add_header("User-Agent", "proxy-bump-dependencies")
    if accept is not None:
        req.add_header("Accept", accept)
    if auth:
        token = os.environ.get("GITHUB_TOKEN")
        if token:
            req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            return resp.read()
    except (urllib.error.URLError, TimeoutError) as exc:
        _log(f"  ! request failed for {url}: {exc}")
        return None


def _http_json(url: str, *, auth: bool = False) -> object | None:
    body = _http_get(url, accept="application/json", auth=auth)
    if body is None:
        return None
    try:
        return json.loads(body)
    except json.JSONDecodeError as exc:
        _log(f"  ! bad JSON from {url}: {exc}")
        return None


# --------------------------------------------------------------------------- #
# Version helpers
# --------------------------------------------------------------------------- #

_SEMVER_RE = re.compile(r"^v?(\d+)(?:\.(\d+))?(?:\.(\d+))?$")


def _parse_semver(tag: str) -> tuple[int, int, int] | None:
    """Parse a clean ``[v]MAJOR[.MINOR[.PATCH]]`` tag; reject anything with a suffix."""
    m = _SEMVER_RE.match(tag.strip())
    if m is None:
        return None
    return (int(m.group(1)), int(m.group(2) or 0), int(m.group(3) or 0))


def _version_tuple(tag: str) -> tuple[int, ...] | None:
    """Numeric tuple of a dotted version (any arity, optional ``v``); None if impure."""
    s = tag.strip().lstrip("v")
    if not re.fullmatch(r"\d+(?:\.\d+)*", s):
        return None
    return tuple(int(p) for p in s.split("."))


def _is_newer(old: str, new: str) -> bool:
    """True if ``new`` is a strict upgrade over ``old`` (guards against downgrades)."""
    nt = _version_tuple(new)
    if nt is None:
        return False
    ot = _version_tuple(old)
    if ot is None:
        return True
    return nt > ot


def _github_latest_tag(repo: str) -> str | None:
    """Latest stable release tag for ``owner/repo``.

    Tries ``releases/latest`` first (excludes pre-releases and drafts by definition).
    Falls back to the newest tag for repos that publish no GitHub Releases.
    """
    data = _http_json(f"https://api.github.com/repos/{repo}/releases/latest", auth=True)
    if data is not None:
        if not isinstance(data, dict):
            _warn(
                f"{repo}/releases/latest: unexpected response type {type(data).__name__}"
            )
        else:
            name = data.get("tag_name")
            if not isinstance(name, str):
                _warn(f"{repo}/releases/latest: missing tag_name in response")
            else:
                return name

    data = _http_json(f"https://api.github.com/repos/{repo}/tags?per_page=1", auth=True)
    if data is None:
        return None
    if not isinstance(data, list) or len(data) == 0:
        _warn(f"{repo}/tags: unexpected response")
        return None
    entry = data[0]
    if not isinstance(entry, dict):
        _warn(f"{repo}/tags: malformed entry")
        return None
    name = entry.get("name")
    if not isinstance(name, str) or _version_tuple(name) is None:
        _warn(f"{repo}/tags: latest tag {name!r} is not a version tag")
        return None
    return name


def _pypi_latest(package: str) -> str | None:
    """Latest non-prerelease version of a PyPI package."""
    data = _http_json(f"https://pypi.org/pypi/{package}/json")
    if not isinstance(data, dict):
        return None
    info = data.get("info")
    if not isinstance(info, dict):
        return None
    version = info.get("version")
    if not isinstance(version, str):
        return None
    if re.search(r"(a|b|rc|dev|alpha|beta)\d", version):
        return None
    return version


def _sha256_of_url(url: str) -> str | None:
    body = _http_get(url)
    return hashlib.sha256(body).hexdigest() if body is not None else None


def _bump_pypi_pins(text: str, label: str) -> str:
    pin_re = re.compile(r"([A-Za-z0-9_.-]+)==([0-9][A-Za-z0-9_.+!-]*)")

    def repl(m: re.Match[str]) -> str:
        name, ver = m.group(1), m.group(2)
        latest = _pypi_latest(name)
        if latest is None or latest == ver:
            return m.group(0)
        _record(f"{label}/{name}", ver, latest)
        return f"{name}=={latest}"

    return pin_re.sub(repl, text)


def _format_pin(old_ref: str, latest: tuple[int, int, int]) -> str | None:
    """Format ``latest`` to match the precision/prefix of ``old_ref`` (e.g. v4 → v6)."""
    m = re.match(r"^(v?)(\d+)(\.\d+)?(\.\d+)?$", old_ref)
    if m is None:
        return None
    prefix = m.group(1)
    major, minor, patch = latest
    if m.group(4) is not None:
        return f"{prefix}{major}.{minor}.{patch}"
    if m.group(3) is not None:
        return f"{prefix}{major}.{minor}"
    return f"{prefix}{major}"


def _wrap_version(wrap: Path) -> str:
    m = re.search(r"^directory\s*=\s*(.+)$", wrap.read_text(encoding="utf-8"), re.M)
    return m.group(1).strip() if m else "?"


def _bazel_version_key(version: str) -> tuple[tuple[int, ...], int] | None:
    """Sort key for a Bazel module version, understanding the ``.bcr.N`` suffix."""
    m = re.match(r"^(\d+(?:\.\d+)*)(?:\.bcr\.(\d+))?$", version)
    if m is None:
        return None
    return (tuple(int(p) for p in m.group(1).split(".")), int(m.group(2) or 0))


def _bcr_latest_version(module: str) -> str | None:
    """Highest non-yanked version of a module in the Bazel Central Registry."""
    data = _http_json(
        "https://raw.githubusercontent.com/bazelbuild/bazel-central-registry/"
        f"main/modules/{module}/metadata.json"
    )
    if not isinstance(data, dict):
        return None
    versions = data.get("versions")
    yanked = data.get("yanked_versions") or {}
    if not isinstance(versions, list):
        return None
    best: tuple[tuple[int, ...], int] | None = None
    best_str: str | None = None
    for v in versions:
        if not isinstance(v, str) or (isinstance(yanked, dict) and v in yanked):
            continue
        key = _bazel_version_key(v)
        if key is not None and (best is None or key > best):
            best, best_str = key, v
    return best_str


# --------------------------------------------------------------------------- #
# Updaters
# --------------------------------------------------------------------------- #


def update_actions() -> None:
    _log("Checking GitHub Action pins ...")
    workflows = sorted((_REPO_ROOT / ".github" / "workflows").glob("*.yml"))
    uses_re = re.compile(r"(uses:\s*)([^@\s/]+/[^@\s]+?)@(\S+)")

    def resolve(repo_path: str, ref: str) -> str | None:
        if _parse_semver(ref) is None:
            return None  # SHA pin or branch ref: leave alone
        repo = "/".join(repo_path.split("/")[:2])
        tag = _github_latest_tag(repo)
        parsed = _parse_semver(tag) if tag else None
        if parsed is None:
            return None
        new_ref = _format_pin(ref, parsed)
        if new_ref is None or not _is_newer(ref, new_ref):
            return None
        return new_ref

    for wf in workflows:
        text = wf.read_text(encoding="utf-8")

        def repl(m: re.Match[str]) -> str:
            prefix, repo_path, ref = m.group(1), m.group(2), m.group(3)
            new_ref = resolve(repo_path, ref)
            if new_ref is None or new_ref == ref:
                return m.group(0)
            _record(repo_path, ref, new_ref)
            return f"{prefix}{repo_path}@{new_ref}"

        new_text = uses_re.sub(repl, text)

        # Bump the bazelisk binary pin in the same pass.
        bz = re.search(r"bazelisk/releases/download/(v[0-9][^/]*)/", new_text)
        if bz:
            old = bz.group(1)
            tag = _github_latest_tag("bazelbuild/bazelisk")
            if tag and _is_newer(old, tag):
                _record("bazelbuild/bazelisk", old, tag)
                new_text = new_text.replace(
                    f"bazelisk/releases/download/{old}/",
                    f"bazelisk/releases/download/{tag}/",
                )

        if new_text != text:
            wf.write_text(new_text, encoding="utf-8")


def update_precommit() -> None:
    _log("Checking pre-commit hooks ...")
    path = _REPO_ROOT / ".pre-commit-config.yaml"
    text = path.read_text(encoding="utf-8")

    block_re = re.compile(r"(- repo:\s*https://github\.com/(\S+?)/?\n\s*rev:\s*)(\S+)")

    def repl_rev(m: re.Match[str]) -> str:
        head, repo, rev = m.group(1), m.group(2), m.group(3)
        tag = _github_latest_tag(repo)
        if tag is None or not _is_newer(rev, tag):
            return m.group(0)
        _record(repo, rev, tag)
        return f"{head}{tag}"

    text = block_re.sub(repl_rev, text)
    text = _bump_pypi_pins(text, "precommit")
    if text != path.read_text(encoding="utf-8"):
        path.write_text(text, encoding="utf-8")


def update_mkdocs() -> None:
    _log("Checking mkdocs requirements ...")
    path = _REPO_ROOT / "mkdocs" / "requirements.txt"
    text = path.read_text(encoding="utf-8")
    new_text = _bump_pypi_pins(text, "mkdocs")
    if new_text != text:
        path.write_text(new_text, encoding="utf-8")


def update_cpp() -> None:
    _log("Checking C++ libraries (cmake/dependencies.json) ...")
    path = _REPO_ROOT / "cmake" / "dependencies.json"
    deps = json.loads(path.read_text(encoding="utf-8"))
    url_re = re.compile(
        r"https://github\.com/([^/]+/[^/]+)/archive/refs/tags/(.+)\.tar\.gz"
    )
    changed = False
    for dep in deps:
        m = url_re.fullmatch(dep["url"])
        if m is None:
            continue
        repo, old_tag = m.group(1), m.group(2)
        tag = _github_latest_tag(repo)
        if tag is None:
            continue
        old_version, new_version = re.sub(r"^v", "", old_tag), re.sub(r"^v", "", tag)
        if not _is_newer(old_version, new_version):
            continue
        new_url = f"https://github.com/{repo}/archive/refs/tags/{tag}.tar.gz"
        new_sha = _sha256_of_url(new_url)
        if new_sha is None:
            _warn(f"could not download {new_url}; leaving {dep['name']} unchanged")
            continue
        _record(dep["name"], old_version, new_version)
        dep["url"], dep["sha256"] = new_url, new_sha
        changed = True
    if changed:
        path.write_text(json.dumps(deps, indent=2) + "\n", encoding="utf-8")


def update_meson() -> None:
    _log("Checking Meson wraps ...")
    wraps = sorted((_REPO_ROOT / "subprojects").glob("*.wrap"))
    if not wraps:
        return
    if shutil.which("meson") is None:
        _warn("meson not found on PATH; skipping wraps")
        return
    for wrap in wraps:
        name = wrap.stem
        if not re.search(
            r"^wrapdb_version\s*=", wrap.read_text(encoding="utf-8"), re.MULTILINE
        ):
            _log(f"  - {name}: not wrapdb-managed; skipping")
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
            _warn(f"`meson wrap update {name}` failed; left unchanged")
            continue
        after = _wrap_version(wrap)
        if after != before:
            _record(name, before, after)


def update_bazel() -> None:
    _log("Checking Bazel modules (MODULE.bazel) ...")
    path = _REPO_ROOT / "MODULE.bazel"
    text = path.read_text(encoding="utf-8")
    dep_re = re.compile(r'(bazel_dep\(name = "([^"]+)", version = ")([^"]+)(")')

    def repl(m: re.Match[str]) -> str:
        head, name, version, tail = m.group(1), m.group(2), m.group(3), m.group(4)
        latest = _bcr_latest_version(name)
        if latest is None or latest == version:
            return m.group(0)
        _record(name, version, latest)
        return f"{head}{latest}{tail}"

    new_text = dep_re.sub(repl, text)
    if new_text != text:
        path.write_text(new_text, encoding="utf-8")


def update_proxy_deps() -> None:
    _log("Checking proxy_deps.bzl http_archive pins ...")
    path = _REPO_ROOT / "proxy_deps.bzl"
    original = path.read_text(encoding="utf-8")
    text = original
    url_re = re.compile(
        r'https://github\.com/([^/]+/[^/]+)/releases/download/([^/]+)/([^"]+?\.tar\.gz)'
    )
    for m in url_re.finditer(original):
        repo, old_ver, asset = m.group(1), m.group(2), m.group(3)
        tag = _github_latest_tag(repo)
        if tag is None or tag == old_ver:
            continue
        new_url = (
            f"https://github.com/{repo}/releases/download/{tag}/"
            f"{asset.replace(old_ver, tag)}"
        )
        new_sha = _sha256_of_url(new_url)
        if new_sha is None:
            _warn(f"could not download {new_url}; leaving {repo} unchanged")
            continue
        old_sha: str | None = None
        for sm in re.finditer(r'sha256 = "([0-9a-f]{64})"', original[: m.start()]):
            old_sha = sm.group(1)
        text = text.replace(
            f"/releases/download/{old_ver}/", f"/releases/download/{tag}/"
        )
        base = asset[: asset.index(f"-{old_ver}")] if f"-{old_ver}" in asset else None
        if base is not None:
            text = text.replace(f"{base}-{old_ver}", f"{base}-{tag}")
        if old_sha is not None:
            text = text.replace(old_sha, new_sha)
        _record(repo, old_ver, tag)
    if text != original:
        path.write_text(text, encoding="utf-8")


def update_bazelversion() -> None:
    _log("Checking .bazelversion ...")
    path = _REPO_ROOT / ".bazelversion"
    current = path.read_text(encoding="utf-8").strip()
    tag = _github_latest_tag("bazelbuild/bazel")
    if tag is None:
        return
    latest = re.sub(r"^v", "", tag)
    if _is_newer(current, latest):
        _record("bazel", current, latest)
        path.write_text(latest + "\n", encoding="utf-8")


def refresh_bazel_lock() -> None:
    _log("Refreshing MODULE.bazel.lock ...")
    if shutil.which("bazel") is None:
        _warn("bazel not found on PATH; MODULE.bazel.lock not refreshed")
        return
    result = subprocess.run(
        ["bazel", "mod", "graph", "--lockfile_mode=update"],
        cwd=_REPO_ROOT,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        _warn(
            f"`bazel mod graph` failed; MODULE.bazel.lock may be stale: {result.stderr.strip()}"
        )


if __name__ == "__main__":
    update_actions()
    update_precommit()
    update_mkdocs()
    update_cpp()
    update_meson()
    update_bazel()
    update_proxy_deps()
    update_bazelversion()
    refresh_bazel_lock()
