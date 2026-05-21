PROXY_CXX_COPTS = select({
    "@bazel_tools//src/conditions:windows": ["/std:c++20", "/utf-8"],
    "//conditions:default": ["-std=c++20"],
})

PROXY_WARNING_COPTS = select({
    "//:msvc_compiler": ["/W4"],
    "//:clang_compiler": ["-Wall", "-Wextra", "-Wpedantic", "-Wno-c++2b-extensions"],
    "//conditions:default": ["-Wall", "-Wextra", "-Wpedantic"],
})

PROXY_STRICT_WARNING_COPTS = select({
    "//:msvc_compiler": ["/WX"],
    "//conditions:default": ["-Werror"],
})

PROXY_COMMON_COPTS = PROXY_CXX_COPTS + PROXY_WARNING_COPTS + PROXY_STRICT_WARNING_COPTS