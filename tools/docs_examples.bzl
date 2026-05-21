load("@rules_cc//cc:defs.bzl", "cc_binary")

def _example_base(md_path):
    if md_path.endswith(".md"):
        md_path = md_path[:-3]
    return md_path.replace("/", "_")


def _doc_example_source_impl(ctx):
    out = ctx.outputs.out
    ctx.actions.run(
        executable = ctx.executable._extractor,
        arguments = [ctx.file.src.path, out.path],
        inputs = [ctx.file.src],
        outputs = [out],
        tools = [ctx.executable._extractor],
        mnemonic = "ExtractDocExample",
        progress_message = "Generating example source from %s" % ctx.file.src.short_path,
    )
    return DefaultInfo(files = depset([out]))


_doc_example_source = rule(
    implementation = _doc_example_source_impl,
    attrs = {
        "src": attr.label(allow_single_file = [".md"]),
        "out": attr.output(mandatory = True),
        "_extractor": attr.label(
            default = "//tools:extract_example_code_from_docs",
            executable = True,
            cfg = "exec",
        ),
    },
)


def doc_examples(name, docs, copts, deps = []):
    example_targets = []
    for doc in docs:
        base = _example_base(doc)
        source_name = "generate_" + base
        example_targets.append(":" + base)
        _doc_example_source(
            name = source_name,
            src = doc,
            out = "example_%s.cpp" % base,
        )
        cc_binary(
            name = base,
            srcs = [":" + source_name],
            copts = copts,
            deps = ["//:proxy", "@fmt//:fmt"] + deps,
        )

    native.filegroup(
        name = name,
        srcs = example_targets,
    )