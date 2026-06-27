#!/usr/bin/env python3
# Copyright (c) 2022-2026 Microsoft Corporation.
# Copyright (c) 2026-Present Next Gen C++ Foundation.
# Licensed under the MIT License.

"""Build the visualizer test subjects and assert the gdb pretty-printer output.

Honors the environment variables ``CXX`` (default ``g++``), ``CXXSTD`` (default
``c++23``), ``CXXFLAGS`` (extra compile flags), and ``GDB`` (default ``gdb``).
Exits non-zero if compilation fails or any assertion in ``gdb_driver.py`` fails.
"""

import os
import subprocess
import sys
import tempfile

_HERE = os.path.dirname(os.path.abspath(__file__))
_ROOT = os.path.abspath(os.path.join(_HERE, "..", "..", ".."))
_INCLUDE = os.path.join(_ROOT, "include")
_PRINTER = os.path.join(_ROOT, "tools", "visualizers", "proxy_gdb.py")
_SUBJECTS = os.path.join(_HERE, "test_subjects.cpp")
_DRIVER = os.path.join(_HERE, "gdb_driver.py")


def main():
    cxx = os.environ.get("CXX", "g++")
    std = os.environ.get("CXXSTD", "c++23")
    gdb_bin = os.environ.get("GDB", "gdb")
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

        gdb_cmd = [
            gdb_bin,
            "-q",
            "-batch",
            "-nx",
            "-ex",
            "source " + _PRINTER,
            "-ex",
            "break proxy_visualizer_break",
            "-ex",
            "run",
            "-ex",
            "source " + _DRIVER,
            exe,
        ]
        print("$", " ".join(gdb_cmd), flush=True)
        proc = subprocess.run(gdb_cmd, check=False, capture_output=True, text=True)
        sys.stdout.write(proc.stdout)
        sys.stderr.write(proc.stderr)
        # Trust the printed sentinel rather than gdb's exit code, which an active
        # inferior can mask.
        return 0 if "VISUALIZER_RESULT: PASS" in proc.stdout else 1


if __name__ == "__main__":
    sys.exit(main())
