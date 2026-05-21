#!/usr/bin/env python3
# pyright: strict

import os
from pathlib import Path

from doc_examples import extract_cpp_code


def write_example_source(input_md: Path, output_cpp: Path) -> bool:
    cpp_code = extract_cpp_code(input_md)
    if cpp_code is None:
        return False

    output_cpp.parent.mkdir(parents=True, exist_ok=True)
    with open(output_cpp, "w", encoding="utf-8") as f:
        _ = f.write(cpp_code)
    return True


def write_example_sources(input_dir: Path, output_dir: Path) -> None:
    for root, _, files in os.walk(input_dir):
        for file in files:
            if not file.endswith(".md"):
                continue
            md_path = Path(root) / file
            rel_path = md_path.relative_to(input_dir)
            rel_base = "_".join([*rel_path.parent.parts, rel_path.stem])
            cpp_path = output_dir / f"example_{rel_base}.cpp"
            _ = write_example_source(md_path, cpp_path)


def _parse_args() -> tuple[Path, Path]:
    import argparse

    parser = argparse.ArgumentParser()
    _ = parser.add_argument(
        "input_path",
        type=Path,
        help="Path to a Markdown document or a directory of Markdown documents",
    )
    _ = parser.add_argument(
        "output_path",
        type=Path,
        help="Generated C++ output path or output directory",
    )
    args = parser.parse_args()
    return args.input_path, args.output_path


def main() -> None:
    input_path, output_path = _parse_args()

    if input_path.is_dir():
        write_example_sources(input_path, output_path)
        return

    if not write_example_source(input_path, output_path):
        raise ValueError(
            f"File '{input_path}' does not contain an '## Example' C++ code block."
        )


if __name__ == "__main__":
    main()
