#!/usr/bin/env python3
# pyright: strict

"""Bump every in-scope dependency of the proxy repo to its latest version.

This is the engine behind the weekly ``pipeline-bump-dependencies.yml`` workflow. It
updates each dependency family in place and prints (and optionally writes) a Markdown
summary of what changed. It never touches git; the workflow decides whether anything
changed and opens the pull request.

Domains handled (each can be turned off with ``--skip``):

  actions       GitHub Action pins in .github/workflows/*.yml (and the bazelisk binary)
  precommit     hook ``rev``s and local-hook tool pins in .pre-commit-config.yaml
  mkdocs        pinned PyPI packages in mkdocs/requirements.txt
  cpp           fmt / googletest / benchmark in dependencies.json (the CMake registry)
  meson         subprojects/*.wrap via ``meson wrap update``
  bazel         bazel_dep versions in MODULE.bazel (from the Bazel Central Registry)
  proxy_deps    http_archive pins in proxy_deps.bzl (legacy WORKSPACE shim)
  bazelversion  .bazelversion (latest bazelbuild/bazel release)

Run from the repository root:
  python3 tools/bump_dependencies.py [--dry-run] [--skip meson,bazel] \
      [--summary-file dep-bump-summary.md]

When a bazel-side declaration changes, the script also refreshes MODULE.bazel.lock with
`bazel mod graph --lockfile_mode=update` (registry metadata only -- no source downloads,
no build); the PR's CI performs the actual build.

Only the Python standard library is used. Set ``GITHUB_TOKEN`` to lift the GitHub API
rate limit (the workflow passes the built-in token automatically).
"""

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import urllib.error
import urllib.request
from dataclasses import dataclass, field
from pathlib import Path

_REPO_ROOT = Path(__file__).resolve().parent.parent
_ALL_DOMAINS = (
    "actions",
    "precommit",
    "mkdocs",
    "cpp",
    "meson",
    "bazel",
    "proxy_deps",
    "bazelversion",
)


@dataclass(frozen=True)
class Change:
    """A single old -> new version bump, used to build the summary."""

    domain: str  # one of _ALL_DOMAINS; groups the summary
    name: str  # human-readable dependency name
    old: str
    new: str


def _log(msg: str) -> None:
    print(msg, file=sys.stderr, flush=True)


# --------------------------------------------------------------------------- #
# HTTP helpers (stdlib only)
# --------------------------------------------------------------------------- #


def _http_get(
    url: str, *, accept: str | None = None, auth: bool = False
) -> bytes | None:
    """GET a URL, returning the body or None on error (errors are logged, not raised)."""
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
    """True if ``new`` is a strict version upgrade over ``old`` (guards downgrades)."""
    nt = _version_tuple(new)
    if nt is None:
        return False
    ot = _version_tuple(old)
    if ot is None:
        return True
    return nt > ot


_github_tag_cache: dict[str, str | None] = {}


def _github_latest_tag(repo: str) -> str | None:
    """Latest stable release tag for ``owner/repo`` (cached), with a tags fallback."""
    if repo in _github_tag_cache:
        return _github_tag_cache[repo]
    tag: str | None = None
    data = _http_json(f"https://api.github.com/repos/{repo}/releases/latest", auth=True)
    if isinstance(data, dict):
        raw = data.get("tag_name")
        if isinstance(raw, str):
            tag = raw
    if tag is None:
        # No published "latest" release: pick the highest clean-semver tag instead.
        tags = _http_json(
            f"https://api.github.com/repos/{repo}/tags?per_page=100", auth=True
        )
        best: tuple[int, int, int] | None = None
        if isinstance(tags, list):
            for entry in tags:
                if not isinstance(entry, dict):
                    continue
                name = entry.get("name")
                if not isinstance(name, str):
                    continue
                parsed = _parse_semver(name)
                if parsed is not None and (best is None or parsed > best):
                    best, tag = parsed, name
    _github_tag_cache[repo] = tag
    return tag


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
        return None  # skip pre-releases
    return version


# --------------------------------------------------------------------------- #
# Updater
# --------------------------------------------------------------------------- #


@dataclass
class Updater:
    dry_run: bool
    skip: set[str]
    changes: list[Change] = field(default_factory=list)

    def _record(self, domain: str, name: str, old: str, new: str) -> None:
        self.changes.append(Change(domain, name, old, new))
        _log(f"  - {name}: {old} -> {new}")

    def _write(self, path: Path, content: str) -> None:
        if self.dry_run:
            return
        path.write_text(content, encoding="utf-8")

    def _sha256_of_url(self, url: str) -> str | None:
        body = _http_get(url)
        if body is None:
            return None
        return hashlib.sha256(body).hexdigest()

    # ----- GitHub Actions + bazelisk ---------------------------------------- #

    def update_actions(self) -> None:
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
                return None  # already current, or would be a downgrade
            return new_ref

        for wf in workflows:
            text = wf.read_text(encoding="utf-8")

            def repl(m: re.Match[str]) -> str:
                prefix, repo_path, ref = m.group(1), m.group(2), m.group(3)
                new_ref = resolve(repo_path, ref)
                if new_ref is None or new_ref == ref:
                    return m.group(0)
                self._record("actions", repo_path, ref, new_ref)
                return f"{prefix}{repo_path}@{new_ref}"

            new_text = uses_re.sub(repl, text)
            new_text = self._bump_bazelisk(new_text)
            if new_text != text:
                self._write(wf, new_text)

    def _bump_bazelisk(self, text: str) -> str:
        m = re.search(r"bazelisk/releases/download/(v[0-9][^/]*)/", text)
        if m is None:
            return text
        old = m.group(1)
        tag = _github_latest_tag("bazelbuild/bazelisk")
        if not tag or not _is_newer(old, tag):
            return text
        self._record("actions", "bazelbuild/bazelisk", old, tag)
        return text.replace(
            f"bazelisk/releases/download/{old}/",
            f"bazelisk/releases/download/{tag}/",
        )

    # ----- pre-commit ------------------------------------------------------- #

    def update_precommit(self) -> None:
        _log("Checking pre-commit hooks ...")
        path = _REPO_ROOT / ".pre-commit-config.yaml"
        text = path.read_text(encoding="utf-8")

        # Bump each `- repo: <github-url>` block's `rev:` to the latest tag.
        block_re = re.compile(
            r"(- repo:\s*https://github\.com/(\S+?)/?\n\s*rev:\s*)(\S+)",
        )

        def repl_rev(m: re.Match[str]) -> str:
            # Pre-commit pins an exact tag (any format, e.g. buildifier's 8.5.1.1), so
            # take the latest tag verbatim rather than reformatting it.
            head, repo, rev = m.group(1), m.group(2), m.group(3)
            tag = _github_latest_tag(repo)
            if tag is None or not _is_newer(rev, tag):
                return m.group(0)
            self._record("precommit", repo, rev, tag)
            return f"{head}{tag}"

        text = block_re.sub(repl_rev, text)

        # Bump local-hook `additional_dependencies` PyPI pins ("name==version").
        text = self._bump_pypi_pins(text, "precommit")
        if text != path.read_text(encoding="utf-8"):
            self._write(path, text)

    def _bump_pypi_pins(self, text: str, domain: str) -> str:
        pin_re = re.compile(r"([A-Za-z0-9_.-]+)==([0-9][A-Za-z0-9_.+!-]*)")

        def repl(m: re.Match[str]) -> str:
            name, ver = m.group(1), m.group(2)
            latest = _pypi_latest(name)
            if latest is None or latest == ver:
                return m.group(0)
            self._record(domain, name, ver, latest)
            return f"{name}=={latest}"

        return pin_re.sub(repl, text)

    # ----- mkdocs ----------------------------------------------------------- #

    def update_mkdocs(self) -> None:
        _log("Checking mkdocs requirements ...")
        path = _REPO_ROOT / "mkdocs" / "requirements.txt"
        text = path.read_text(encoding="utf-8")
        new_text = self._bump_pypi_pins(text, "mkdocs")
        if new_text != text:
            self._write(path, new_text)

    # ----- C++ libraries (dependencies.json) -------------------------------- #

    def update_cpp(self) -> None:
        _log("Checking C++ libraries (dependencies.json) ...")
        path = _REPO_ROOT / "dependencies.json"
        deps = json.loads(path.read_text(encoding="utf-8"))
        # The owner/repo and the current tag are both encoded in each entry's GitHub
        # tarball URL, so name + url + sha256 is all the metadata an entry needs.
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
            old_version, new_version = (
                re.sub(r"^v", "", old_tag),
                re.sub(r"^v", "", tag),
            )
            if not _is_newer(old_version, new_version):
                continue
            new_url = f"https://github.com/{repo}/archive/refs/tags/{tag}.tar.gz"
            new_sha = self._sha256_of_url(new_url)
            if new_sha is None:
                _log(
                    f"  ! could not download {new_url}; leaving {dep['name']} unchanged"
                )
                continue
            self._record("cpp", dep["name"], old_version, new_version)
            dep["url"], dep["sha256"] = new_url, new_sha
            changed = True
        if changed:
            self._write(path, json.dumps(deps, indent=2) + "\n")

    # ----- Meson wraps ------------------------------------------------------ #

    def update_meson(self) -> None:
        _log("Checking Meson wraps ...")
        subprojects = _REPO_ROOT / "subprojects"
        wraps = sorted(subprojects.glob("*.wrap"))
        if not wraps:
            return
        if shutil.which("meson") is None:
            _log("  ! meson not found on PATH; skipping wraps")
            return
        for wrap in wraps:
            name = wrap.stem
            before = _wrap_version(wrap)
            if self.dry_run:
                _log(
                    f"  - {name}: would run `meson wrap update {name}` (current {before})"
                )
                continue
            result = subprocess.run(
                ["meson", "wrap", "update", name],
                cwd=_REPO_ROOT,
                capture_output=True,
                text=True,
                check=False,
            )
            if result.returncode != 0:
                _log(f"  ! `meson wrap update {name}` failed: {result.stderr.strip()}")
                continue
            after = _wrap_version(wrap)
            if after != before:
                self._record("meson", name, before, after)

    # ----- Bazel modules (MODULE.bazel) ------------------------------------- #

    def update_bazel(self) -> None:
        _log("Checking Bazel modules (MODULE.bazel) ...")
        path = _REPO_ROOT / "MODULE.bazel"
        text = path.read_text(encoding="utf-8")
        dep_re = re.compile(r'(bazel_dep\(name = "([^"]+)", version = ")([^"]+)(")')

        def repl(m: re.Match[str]) -> str:
            head, name, version, tail = m.group(1), m.group(2), m.group(3), m.group(4)
            latest = _bcr_latest_version(name)
            if latest is None or latest == version:
                return m.group(0)
            self._record("bazel", name, version, latest)
            return f"{head}{latest}{tail}"

        new_text = dep_re.sub(repl, text)
        if new_text != text:
            self._write(path, new_text)

    # ----- proxy_deps.bzl (WORKSPACE shim) ---------------------------------- #

    def update_proxy_deps(self) -> None:
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
            base = (
                asset[: asset.index(f"-{old_ver}")] if f"-{old_ver}" in asset else None
            )
            new_url = (
                f"https://github.com/{repo}/releases/download/{tag}/"
                f"{asset.replace(old_ver, tag)}"
            )
            new_sha = self._sha256_of_url(new_url)
            if new_sha is None:
                _log(f"  ! could not download {new_url}; leaving {repo} unchanged")
                continue
            # The http_archive's sha256 is the last one declared before its url block.
            old_sha: str | None = None
            for sm in re.finditer(r'sha256 = "([0-9a-f]{64})"', original[: m.start()]):
                old_sha = sm.group(1)
            text = text.replace(
                f"/releases/download/{old_ver}/", f"/releases/download/{tag}/"
            )
            if base is not None:
                text = text.replace(f"{base}-{old_ver}", f"{base}-{tag}")
            if old_sha is not None:
                text = text.replace(old_sha, new_sha)
            self._record("proxy_deps", repo, old_ver, tag)
        if text != original:
            self._write(path, text)

    # ----- .bazelversion ---------------------------------------------------- #

    def update_bazelversion(self) -> None:
        _log("Checking .bazelversion ...")
        path = _REPO_ROOT / ".bazelversion"
        current = path.read_text(encoding="utf-8").strip()
        tag = _github_latest_tag("bazelbuild/bazel")
        if tag is None:
            return
        latest = re.sub(r"^v", "", tag)
        if _is_newer(current, latest):
            self._record("bazelversion", "bazel", current, latest)
            self._write(path, latest + "\n")

    # ----- orchestration ---------------------------------------------------- #

    def run(self) -> None:
        dispatch = {
            "actions": self.update_actions,
            "precommit": self.update_precommit,
            "mkdocs": self.update_mkdocs,
            "cpp": self.update_cpp,
            "meson": self.update_meson,
            "bazel": self.update_bazel,
            "proxy_deps": self.update_proxy_deps,
            "bazelversion": self.update_bazelversion,
        }
        for domain in _ALL_DOMAINS:
            if domain in self.skip:
                _log(f"Skipping {domain}")
                continue
            dispatch[domain]()
        self._refresh_bazel_lock()

    def _refresh_bazel_lock(self) -> None:
        # Bumping any bazel-side declaration (MODULE.bazel / proxy_deps.bzl /
        # .bazelversion) invalidates MODULE.bazel.lock. Refresh it from registry metadata
        # only -- no source downloads and no build; the PR's CI does the actual build.
        if not any(
            c.domain in {"bazel", "proxy_deps", "bazelversion"} for c in self.changes
        ):
            return
        _log("Refreshing MODULE.bazel.lock ...")
        if self.dry_run:
            _log("  - would run `bazel mod graph --lockfile_mode=update`")
            return
        if shutil.which("bazel") is None:
            _log("  ! bazel not found on PATH; skipping lockfile refresh")
            return
        result = subprocess.run(
            ["bazel", "mod", "graph", "--lockfile_mode=update"],
            cwd=_REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            _log(f"  ! `bazel mod graph` failed: {result.stderr.strip()}")

    def summary(self) -> str:
        titles = {
            "actions": "GitHub Actions",
            "precommit": "Pre-commit hooks",
            "mkdocs": "mkdocs (PyPI)",
            "cpp": "C++ libraries (CMake / Meson / Bazel)",
            "meson": "Meson wraps",
            "bazel": "Bazel modules",
            "proxy_deps": "Bazel WORKSPACE shim (proxy_deps.bzl)",
            "bazelversion": "Bazel version",
        }
        lines = ["## Dependency bump summary", ""]
        if not self.changes:
            lines.append("No dependency updates were available.")
            return "\n".join(lines) + "\n"
        for domain in _ALL_DOMAINS:
            seen: set[tuple[str, str, str]] = set()
            group: list[Change] = []
            for c in self.changes:
                key = (c.name, c.old, c.new)
                if c.domain == domain and key not in seen:
                    seen.add(key)
                    group.append(c)
            if not group:
                continue
            lines.append(f"### {titles[domain]}")
            for c in group:
                lines.append(f"- `{c.name}`: {c.old} → {c.new}")
            lines.append("")
        return "\n".join(lines).rstrip() + "\n"


def _format_pin(old_ref: str, latest: tuple[int, int, int]) -> str | None:
    """Format ``latest`` to match the precision/prefix of ``old_ref`` (e.g. v4 -> v6)."""
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
    """Return a wrap file's ``directory = ...`` value (encodes the resolved version)."""
    m = re.search(r"^directory\s*=\s*(.+)$", wrap.read_text(encoding="utf-8"), re.M)
    return m.group(1).strip() if m else "?"


def _bazel_version_key(version: str) -> tuple[tuple[int, ...], int] | None:
    """Sort key for a Bazel module version, understanding the ``.bcr.N`` suffix."""
    m = re.match(r"^(\d+(?:\.\d+)*)(?:\.bcr\.(\d+))?$", version)
    if m is None:
        return None
    release = tuple(int(p) for p in m.group(1).split("."))
    return (release, int(m.group(2) or 0))


def _bcr_latest_version(module: str) -> str | None:
    """Highest non-yanked version of a module in the Bazel Central Registry."""
    url = (
        "https://raw.githubusercontent.com/bazelbuild/bazel-central-registry/"
        f"main/modules/{module}/metadata.json"
    )
    data = _http_json(url)
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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="compute and report bumps without modifying any files",
    )
    parser.add_argument(
        "--skip",
        default="",
        help=f"comma-separated domains to skip (any of: {', '.join(_ALL_DOMAINS)})",
    )
    parser.add_argument(
        "--summary-file",
        type=Path,
        help="also write the Markdown summary to this path",
    )
    args = parser.parse_args()

    skip = {s.strip() for s in args.skip.split(",") if s.strip()}
    unknown = skip - set(_ALL_DOMAINS)
    if unknown:
        parser.error(f"unknown --skip domains: {', '.join(sorted(unknown))}")

    updater = Updater(dry_run=args.dry_run, skip=skip)
    updater.run()

    summary = updater.summary()
    print(summary)
    if args.summary_file is not None:
        args.summary_file.write_text(summary, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
