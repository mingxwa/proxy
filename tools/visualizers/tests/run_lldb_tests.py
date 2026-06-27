#!/usr/bin/env python3
# Copyright (c) 2022-2026 Microsoft Corporation.
# Copyright (c) 2026-Present Next Gen C++ Foundation.
# Licensed under the MIT License.

"""Build the visualizer test subjects and assert the lldb formatters on them.

Honors the environment variables ``CXX`` (default ``clang++``), ``CXXSTD``
(default ``c++23``), ``CXXFLAGS`` (extra compile flags), and ``LLDB`` (default
``lldb``). Exits non-zero if compilation fails or any assertion in
``lldb_driver.py`` fails.
"""

import os
import subprocess
import sys
import tempfile

_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.abspath(os.path.join(_HERE, "..", "..", ".."))
_INCLUDE = os.path.join(_ROOT, "include")
_PRINTER = os.path.join(_ROOT, "tools", "visualizers", "proxy_lldb.py")
_SUBJECTS = os.path.join(_HERE, "test_subjects.cpp")
_DRIVER = os.path.join(_HERE, "lldb_driver.py")


def main():
    cxx = os.environ.get("CXX", "clang++")
    std = os.environ.get("CXXSTD", "c++23")
    lldb_bin = os.environ.get("LLDB", "lldb")
    extra = os.environ.get("CXXFLAGS", "").split()

    with tempfile.TemporaryDirectory() as tmp:
        exe = os.path.join(tmp, "test_subjects")
        compile_cmd = [
            cxx,
            "-std=" + std,
            "-g",
            "-O0",
            "-I",
            _INCLUDE,
            _SUBJECTS,
            "-o",
            exe,
            *extra,
        ]
        print("$", " ".join(compile_cmd), flush=True)
        subprocess.run(compile_cmd, check=True)

        lldb_cmd = [
            lldb_bin,
            "-b",
            "-o",
            "command script import " + _PRINTER,
            "-o",
            "breakpoint set -n proxy_visualizer_break",
            "-o",
            "run",
            "-o",
            "command script import " + _DRIVER,
            exe,
        ]
        print("$", " ".join(lldb_cmd), flush=True)
        proc = subprocess.run(lldb_cmd, check=False, capture_output=True, text=True)
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        return 0 if "VISUALIZER_RESULT: PASS" in proc.stdout else 1


if __name__ == "__main__":
    sys.exit(main())
