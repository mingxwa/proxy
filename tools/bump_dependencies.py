#!/usr/bin/env python3
# pyright: strict

"""Renovate post-upgrade hook: regenerate the artifacts Renovate cannot compute itself.

Renovate (see renovate.json) discovers every version bump in this repo -- GitHub Actions,
pre-commit hooks, the mkdocs PyPI pins, MODULE.bazel (Bazel Central Registry), .bazelversion,
and the CMake FetchContent registry (via a customManager). It then runs this script as a
``postUpgradeTasks`` command, on the same branch, to regenerate the three derived artifacts a
version bump invalidates but Renovate has no way to produce:

  cmake   the SHA256 in cmake/dependencies.json + the report generator's registry, recomputed
          to match the tarball URL Renovate just bumped.
  meson   subprojects/*.wrap, refreshed via ``meson wrap update`` (wrapdb has no Renovate
          manager, and a wrap's source/patch versions and hashes must move together).
  bazel   MODULE.bazel.lock, refreshed to match the bazel_dep versions Renovate just bumped.

It only rewrites; it does not pick versions. Running it by hand simply makes those artifacts
consistent with the versions already on disk.

Output channels:
  * ``_log``    progress -> stderr.
  * ``_warn``   actionable problems (a download failed, a tool missing) -> stderr only.
                Renovate captures this task's streams, so the workflow redirects stderr to a
                file and re-emits each line as a ``::warning::`` annotation.
  * ``_record`` a regenerated artifact -> stdout as a markdown bullet for the PR/job summary.

Only the Python standard library is used. Set ``GITHUB_TOKEN`` to lift the GitHub download
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


# A Meson wrap's resolved version, read from its "directory = <name>-<version>" line.
_WRAP_DIR_RE = re.compile(r"^directory\s*=\s*(.+)$", re.MULTILINE)
# Marks a wrapdb-managed wrap. `meson wrap update` only handles these; wraps provided by the
# upstream project (fmt, which uses "3rdparty_wrapdb_version") are not updatable this way.
_WRAPDB_RE = re.compile(r"^wrapdb_version\s*=", re.MULTILINE)


def _log(msg: str) -> None:
    print(msg, flush=True)


def _warn(msg: str) -> None:
    """Print an actionable problem to stderr.

    The workflow redirects this task's stderr to a file and re-emits each line as a
    ``::warning::`` annotation -- Renovate captures the task's streams, so a printed
    ``::warning::`` here would be swallowed. Keep stderr warnings-only: progress goes to stdout.
    """
    print(msg.replace("\n", " "), file=sys.stderr, flush=True)


def _record(name: str, old: str, new: str) -> None:
    print(f"- {name}: {old} → {new}", flush=True)


def _download(url: str) -> bytes | None:
    """GET ``url`` (with the GitHub token, for rate limits); ``None`` on any failure."""
    req = urllib.request.Request(url)
    req.add_header("User-Agent", "proxy-bump-dependencies")
    token = os.environ.get("GITHUB_TOKEN")
    if token:
        req.add_header("Authorization", f"Bearer {token}")
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            return resp.read()
    except (urllib.error.URLError, TimeoutError):
        return None


def refresh_registry_hashes(rel_path: str, label: str) -> None:
    """Recompute each entry's SHA256 to match its (Renovate-bumped) URL."""
    _log(f"Refreshing hashes for {label} ({rel_path}) ...")
    path = _REPO_ROOT / rel_path
    deps = json.loads(path.read_text(encoding="utf-8"))
    changed = False
    for dep in deps:
        name, url = dep["name"], dep["url"]
        body = _download(url)
        if body is None:
            _warn(f"{label}/{name}: could not download {url}; sha256 left unchanged")
            continue
        digest = hashlib.sha256(body).hexdigest()
        if digest != dep.get("sha256"):
            _record(f"{label}/{name} sha256", str(dep.get("sha256"))[:12], digest[:12])
            dep["sha256"] = digest
            changed = True
    if changed:
        path.write_text(json.dumps(deps, indent=2) + "\n", encoding="utf-8")


def _wrap_version(wrap: Path) -> str:
    m = _WRAP_DIR_RE.search(wrap.read_text(encoding="utf-8"))
    return m.group(1).strip() if m else "?"


def update_meson() -> None:
    _log("Updating Meson wraps ...")
    wraps = sorted((_REPO_ROOT / "subprojects").glob("*.wrap"))
    if not wraps:
        _warn("no wrap files found in subprojects/")
        return
    if shutil.which("meson") is None:
        _warn("meson not found on PATH; wraps not updated")
        return
    for wrap in wraps:
        name = wrap.stem
        if _WRAPDB_RE.search(wrap.read_text(encoding="utf-8")) is None:
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
            f"`bazel mod graph` failed; MODULE.bazel.lock may be stale: "
            f"{result.stderr.strip()}"
        )


if __name__ == "__main__":
    refresh_registry_hashes("cmake/dependencies.json", "cmake")
    refresh_registry_hashes(
        "tools/report_generator/dependencies.json", "report_generator"
    )
    update_meson()
    refresh_bazel_lock()
