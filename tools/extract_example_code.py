#!/usr/bin/env python3
# pyright: strict

"""Extract the C++ example from a Markdown documentation file.

Usage: extract_example_code.py INPUT.md OUTPUT.cpp
"""

import re
import sys
from pathlib import Path
from typing import Optional

_EXAMPLE_PATTERN = re.compile(
    r"(?P<prefix>## Example\r?\n\r?\n```cpp\r?\n)"
    r"(?P<code>.*?)"
    r"(?P<suffix>\r?\n```)",
    re.DOTALL,
)


def try_extract_example_code(content: str) -> Optional["re.Match[str]"]:
    """Return the sole ## Example cpp block match in *content*, or None.

    Raises ValueError if more than one such block exists.
    """
    matches = list(_EXAMPLE_PATTERN.finditer(content))
    if len(matches) > 1:
        raise ValueError("more than one '## Example' C++ block")
    return matches[0] if matches else None


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} INPUT.md OUTPUT.cpp", file=sys.stderr)
        sys.exit(1)
    md_path = Path(sys.argv[1])
    try:
        m = try_extract_example_code(md_path.read_text(encoding="utf-8"))
    except ValueError as e:
        raise ValueError(f"'{md_path}': {e}") from None
    if m is not None:
        code = (
            f"// This file was auto-generated from:\n// {md_path}\n\n{m.group('code')}"
        )
        Path(sys.argv[2]).write_text(code, encoding="utf-8")
