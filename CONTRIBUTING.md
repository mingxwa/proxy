# Contributing

Thanks for contributing! This document covers the local lint setup so your changes match what CI enforces.

## Lint matrix

CI runs the `bvt-lint` job on every PR. The workflow installs `pre-commit`, runs `pre-commit run --all-files`, and then builds the docs with `mkdocs build --strict`. The full check set, file scopes, and pinned tool versions all live in [`.pre-commit-config.yaml`](.pre-commit-config.yaml). That file is the single source of truth.

| Hook                  | What it checks |
|-----------------------|----------------|
| `pre-commit-hooks`    | Basic hygiene. Line endings, trailing whitespace, final newline, merge-conflict markers, case-conflicting paths, Windows-illegal names, files over 1024 KB. |
| `ruff-format`         | Python scripts (`*.py`). |
| `clang-format`        | Every tracked C++ source file (`*.h`, `*.cpp`, `*.ixx`) and the `## Example` cpp blocks inside any Markdown file. |
| `gersemi`             | CMake build files (`CMakeLists.txt` and `*.cmake`) outside `subprojects/`. |
| `meson-format`        | Meson build files (`meson.build`, `meson.options`, `meson_options.txt`). |
| `buildifier`          | Bazel build files (`BUILD.bazel`, `MODULE.bazel`, `WORKSPACE.bazel`, `*.bzl`). |
| `actionlint`          | GitHub Actions workflows (`.github/workflows/*.yml`). |

These hooks run both locally and in CI. On top of them, CI also builds the docs with `mkdocs build --strict` to confirm `docs/` builds cleanly.

The `clang-format` hook is backed by [`tools/format_cpp.py`](tools/format_cpp.py), which you can also run directly.

```sh
python3 tools/format_cpp.py             # apply fixes in place
python3 tools/format_cpp.py --check     # exits non-zero if any file would change
```

## Running lint locally

Install [`pre-commit`](https://pre-commit.com/), then run `pre-commit install` so the hooks run on every `git commit`. The framework manages each formatter at its pinned version for you. See the [pre-commit docs](https://pre-commit.com/) for everyday usage such as running a single hook or all files.

## Upgrading a formatter

All versions are pinned in [`.pre-commit-config.yaml`](.pre-commit-config.yaml), which is the only file you change to upgrade a formatter:

- For most hooks, bump the `rev:` field.
- For local hooks, bump the version pin in `additional_dependencies:` (for example `meson==X.Y.Z` or `clang-format==X.Y.Z`).

CI reinstalls from the new pin on the next run.
