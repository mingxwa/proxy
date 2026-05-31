PROXY_BUILD_COPTS = select({
    "//:msvc_like": ["/W4", "/utf-8"],
    "//conditions:default": ["-Wall", "-Wextra", "-Wpedantic"],
}) + select({
    "@rules_cc//cc/compiler:clang": ["-Wno-c++2b-extensions"],
    "//conditions:default": [],
})

PROXY_STRICT_BUILD_COPTS = PROXY_BUILD_COPTS + select({
    "//:msvc_like": ["/WX"],
    "//conditions:default": ["-Werror"],
})
