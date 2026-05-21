#!/usr/bin/env python3
# pyright: strict

from pathlib import Path
from typing import Optional

import re

EXAMPLE_PATTERN = re.compile(r"## Example\r?\n\r?\n```cpp\r?\n(.*?)\r?\n```", re.DOTALL)


def _wrap_format_example(cpp_code: str) -> str:
    if "pro::skills::format" in cpp_code:
        return f"""
#include <proxy/proxy.h>
#ifdef PRO4D_HAS_FORMAT
{cpp_code}
#else
int main() {{
  // std::format not available
  return 77;
}}
#endif
""".strip()
    return cpp_code


def extract_cpp_code(md_path: Path) -> Optional[str]:
    with open(md_path, "r", encoding="utf-8") as f:
        content = f.read()

    code_blocks: list[str] = re.findall(EXAMPLE_PATTERN, content)

    if len(code_blocks) == 0:
        return None
    if len(code_blocks) > 1:
        msg = f"File '{md_path}' contains more than one '## Example' C++ code block."
        raise ValueError(msg)

    cpp_code = code_blocks[0]
    header = f"""
// This file was auto-generated from:
// {md_path}

""".lstrip()

    return header + _wrap_format_example(cpp_code)


def render_cpp_code(md_path: Path, *, allow_stub: bool = False) -> str:
    cpp_code = extract_cpp_code(md_path)
    if cpp_code is not None:
        return cpp_code

    if not allow_stub:
        raise ValueError(f"File '{md_path}' does not contain an '## Example' C++ code block.")

    return f"""// This file was auto-generated from:
// {md_path}

int main() {{
  return 0;
}}
"""