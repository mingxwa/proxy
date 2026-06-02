"""Minimal WORKSPACE-mode dependency loading for Bazel 5.1+ consumers.

MODULE.bazel is the supported path for development and targets Bazel 7+. This
file exists as a minimal compatibility shim so Bazel 5.1+ consumers using
legacy WORKSPACE mode can still depend on @proxy//:proxy. Tests, benchmarks,
and docs are NOT supported through this path.
"""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def proxy_deps():
    """Declare @proxy//:proxy's runtime dependencies for WORKSPACE-mode consumers.

    Each dep is only declared if the consumer has not declared it already,
    so consumers can pin different versions if needed.
    """
    if not native.existing_rule("bazel_skylib"):
        http_archive(
            name = "bazel_skylib",
            sha256 = "bc283cdfcd526a52c3201279cda4bc298652efa898b10b4db0837dc51652756f",
            urls = [
                "https://github.com/bazelbuild/bazel-skylib/releases/download/1.7.1/bazel-skylib-1.7.1.tar.gz",
            ],
        )
    if not native.existing_rule("rules_cc"):
        http_archive(
            name = "rules_cc",
            sha256 = "2037875b9a4456dce4a79d112a8ae885bbc4aad968e6587dca6e64f3a0900cdf",
            strip_prefix = "rules_cc-0.0.9",
            urls = [
                "https://github.com/bazelbuild/rules_cc/releases/download/0.0.9/rules_cc-0.0.9.tar.gz",
            ],
        )
