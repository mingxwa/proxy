#!/usr/bin/env python3
# pyright: strict

"""Format C++ throughout the repo.

Usage:
  format_cpp.py            Reformat every C++ file (and Markdown example) in place.
  format_cpp.py --check    Don't change anything. Exit non-zero if any file
                           is not already formatted.

Requires `clang-format` on PATH. The pre-commit hook supplies one via the
`clang-format` PyPI wheel. Manual invocations can use any system install.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

from extract_example_code import try_extract_example_code

REPO_ROOT = Path(__file__).resolve().parent.parent


def _git_ls(*patterns: str) -> list[Path]:
    result = subprocess.run(
        [
            "git",
            "ls-files",
            "--cached",
            "--others",
            "--exclude-standard",
            "-z",
            "--",
            *patterns,
        ],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        check=True,
    )
    return sorted(REPO_ROOT / p for p in result.stdout.split("\0") if p)


def _format_stdin(code: str) -> str:
    result = subprocess.run(
        ["clang-format", "--assume-filename=example.cpp"],
        input=code,
        text=True,
        capture_output=True,
        check=True,
    )
    return result.stdout


def _format_static(files: list[Path], *, check: bool) -> bool:
    if not files:
        return True
    args = [str(f) for f in files]
    cmd = (
        ["clang-format", "--dry-run", "--Werror", *args]
        if check
        else ["clang-format", "-i", *args]
    )
    return subprocess.run(cmd, cwd=REPO_ROOT).returncode == 0


def _format_markdown(md: Path, *, check: bool) -> bool:
    content = md.read_text(encoding="utf-8")
    try:
        m = try_extract_example_code(content)
    except ValueError as e:
        raise ValueError(f"'{md.relative_to(REPO_ROOT)}': {e}") from None
    if m is None:
        return True

    original = m.group("code")
    formatted = _format_stdin(original).rstrip("\n")
    if formatted == original:
        return True

    if check:
        rel = md.relative_to(REPO_ROOT).as_posix()
        print(f"{rel}: '## Example' block would be reformatted", file=sys.stderr)
        return False

    new_content = content[: m.start("code")] + formatted + content[m.end("code") :]
    md.write_text(new_content, encoding="utf-8")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="dry-run; exit non-zero if anything would change",
    )
    args = parser.parse_args()

    ok = _format_static(_git_ls("*.h", "*.cpp", "*.ixx"), check=args.check)
    for md in _git_ls("*.md"):
        ok = _format_markdown(md, check=args.check) and ok
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
