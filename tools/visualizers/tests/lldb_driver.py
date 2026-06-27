# Copyright (c) 2022-2026 Microsoft Corporation.
# Copyright (c) 2026-Present Next Gen C++ Foundation.
# Licensed under the MIT License.

# Imported by lldb after stopping at proxy_visualizer_break(). Asserts that the
# summary recovers each contained pointer type and that `proxy_call` can call
# through the stored pointer, then prints a VISUALIZER_RESULT sentinel.

import os
import sys

import lldb

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import proxy_lldb  # noqa: E402

_results = []


def _check(name, cond, detail=""):
    print("[%s] %s%s" % ("PASS" if cond else "FAIL", name, detail and " -- " + detail))
    _results.append(bool(cond))


def _run(debugger):
    target = debugger.GetSelectedTarget()
    thread = target.GetProcess().GetSelectedThread()
    main_idx = 0
    for i, fr in enumerate(thread.frames):
        if (fr.GetFunctionName() or "").startswith("main"):
            main_idx = i
            break
    thread.SetSelectedFrame(main_idx)
    frame = thread.GetSelectedFrame()

    def summary(name):
        return frame.FindVariable(name).GetSummary() or ""

    def call(name):
        ret = lldb.SBCommandReturnObject()
        debugger.GetCommandInterpreter().HandleCommand(
            "proxy_call %s ->Speak()" % name, ret
        )
        return (ret.GetOutput() or "") + (ret.GetError() or "")

    _check("p_raw holds Cat*", "holds Cat*" in summary("p_raw"))
    _check(
        "p_unique holds unique_ptr", "holds std::unique_ptr<Cat" in summary("p_unique")
    )
    _check(
        "p_shared holds shared_ptr", "holds std::shared_ptr<Cat" in summary("p_shared")
    )
    _check("p_made recovered", "holds " in summary("p_made"))
    _check("p_empty is empty", "[empty]" in summary("p_empty"))
    _check(
        "p_direct_rtti recovered",
        "holds std::unique_ptr<Cat" in summary("p_direct_rtti"),
    )
    _check("p_rtti recovered", "holds std::unique_ptr<Cat" in summary("p_rtti"))
    _check("p_both recovered", "holds std::unique_ptr<Cat" in summary("p_both"))

    expected = {
        "p_raw": "Felix says meow",
        "p_unique": "Tom says meow",
        "p_shared": "Sasha says meow",
        "p_made": "Milo says meow",
        "p_direct_rtti": "Direct says meow",
        "p_rtti": "Indirect says meow",
        "p_both": "Both says meow",
    }
    for name, want in expected.items():
        got = call(name)
        _check(
            "call %s ->Speak()" % name,
            want in got,
            "want %r got %s" % (want, got.strip()),
        )


def __lldb_init_module(debugger, internal_dict):
    debugger.SetAsync(False)
    try:
        _run(debugger)
        failed = _results.count(False)
    except Exception as exc:  # noqa: BLE001 - report and fail rather than hang
        import traceback

        traceback.print_exc()
        print("driver exception:", exc)
        failed = 1
    total = len(_results)
    print("\n%d/%d checks passed" % (total - failed, total))
    print("VISUALIZER_RESULT:", "PASS" if total > 0 and failed == 0 else "FAIL")
