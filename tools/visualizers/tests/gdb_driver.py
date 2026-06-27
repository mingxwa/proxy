# Copyright (c) 2022-2026 Microsoft Corporation.
# Copyright (c) 2026-Present Next Gen C++ Foundation.
# Licensed under the MIT License.

# Sourced by gdb after stopping at proxy_visualizer_break(). Asserts that the
# pretty-printer recovers each contained type and that $pro_target can call into
# the stored object, then sets gdb's exit code (0 = all checks passed).

import gdb

_results = []


def _check(name, cond, detail=""):
    print("[%s] %s%s" % ("PASS" if cond else "FAIL", name, detail and " -- " + detail))
    _results.append(bool(cond))


def _display(expr):
    try:
        return str(gdb.parse_and_eval(expr))
    except gdb.error as err:
        return "<error: %s>" % err


def _holds(expr, sub):
    return sub in _display(expr)


def _select_main_frame():
    # The breakpoint stops inside proxy_visualizer_break(); the proxy locals live
    # in main()'s frame, so walk up to it before evaluating expressions.
    frame = gdb.newest_frame()
    while frame is not None and frame.name() != "main":
        frame = frame.older()
    if frame is not None:
        frame.select()


def _run():
    _select_main_frame()
    # Pointer-type recovery + display via the pretty-printer.
    _check("p_raw holds Cat*", _holds("p_raw", "holds Cat*"))
    _check("p_unique holds unique_ptr", _holds("p_unique", "holds std::unique_ptr<Cat"))
    _check("p_shared holds shared_ptr", _holds("p_shared", "holds std::shared_ptr<Cat"))
    # make_proxy storage is SBO-dependent; only require that *some* P is recovered.
    _check("p_made recovered", _holds("p_made", "holds "))
    _check("p_empty is empty", _holds("p_empty", "[empty]"))
    # Mechanism 2: recovery driven by direct_rtti / rtti reflectors.
    _check(
        "p_direct_rtti recovered", _holds("p_direct_rtti", "holds std::unique_ptr<Cat")
    )
    _check("p_rtti recovered", _holds("p_rtti", "holds std::unique_ptr<Cat"))
    _check("p_both recovered", _holds("p_both", "holds std::unique_ptr<Cat"))

    # $pro_type convenience function.
    _check("pro_type(p_raw)", _holds("$pro_type(p_raw)", "Cat*"))

    # $pro_ptr -> calling through the recovered pointer in the inferior.
    expected = {
        "p_raw": "Felix says meow",
        "p_unique": "Tom says meow",
        "p_shared": "Sasha says meow",
        "p_direct_rtti": "Direct says meow",
        "p_rtti": "Indirect says meow",
        "p_both": "Both says meow",
    }
    for name, want in expected.items():
        got = _display("$pro_ptr(%s)->Speak()" % name)
        _check("call %s->Speak()" % name, want in got, "want %r got %s" % (want, got))

    return _results.count(False)


try:
    _failed = _run()
except Exception as _exc:  # noqa: BLE001 - report and fail rather than hang
    print("driver exception:", _exc)
    _failed = 1

_total = len(_results)
print("\n%d/%d checks passed" % (_total - _failed, _total))
print("VISUALIZER_RESULT:", "PASS" if _total > 0 and _failed == 0 else "FAIL")
gdb.execute("set confirm off")
gdb.execute("quit %d" % (1 if _failed else 0))
