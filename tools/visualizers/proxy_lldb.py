# Copyright (c) 2022-2026 Microsoft Corporation.
# Copyright (c) 2026-Present Next Gen C++ Foundation.
# Licensed under the MIT License.

"""LLDB data formatters and helpers for ``pro::proxy<F>``.

``pro::proxy<F>`` is type-erased: the contained pointer object ``P`` lives in the
raw ``ptr_`` buffer and the only runtime trace of its type is the ``meta_``
member. This script recovers the pointer type ``P`` so it can be inspected and
called through with ordinary debugger syntax. See ``proxy_gdb.py`` for the gdb
counterpart; the two share the same recovery mechanisms.

Usage::

    (lldb) command script import /path/to/proxy/tools/visualizers/proxy_lldb.py
    (lldb) frame variable my_proxy     # AnimalFacade [holds std::unique_ptr<...>]
    (lldb) proxy_type my_proxy         # the recovered type of P
    (lldb) proxy_ptr my_proxy          # the stored P itself
    (lldb) proxy_call my_proxy ->Speak()   # call through the stored pointer

Requires the program to be built with debug info (``-g``).
"""

import re
import subprocess

import lldb

_PROXY_RE = re.compile(r"^pro::v4::proxy<")


def _demangle_type(mangled):
    """Demangle an Itanium *type* mangling (e.g. ``3Cat`` -> ``Cat``)."""
    try:
        out = subprocess.run(
            ["c++filt", "-t", mangled], capture_output=True, text=True, check=False
        ).stdout.strip()
        return out if out and out != mangled else None
    except (OSError, ValueError):
        return None


def _extract_balanced(text, marker):
    """Return the ``<...>`` argument that follows ``marker`` (which ends in '<')."""
    pos = text.rfind(marker)
    if pos < 0:
        return None
    i = pos + len(marker)
    depth = 1
    start = i
    while i < len(text) and depth:
        if text[i] == "<":
            depth += 1
        elif text[i] == ">":
            depth -= 1
            if depth == 0:
                return text[start:i].strip()
        i += 1
    return None


def _find_child(val, name, depth=0):
    """Depth-first search for a (possibly inherited/nested) member named ``name``."""
    if depth > 8 or not val.IsValid():
        return None
    for i in range(val.GetNumChildren()):
        child = val.GetChildAtIndex(i)
        if child.GetName() == name:
            return child
        found = _find_child(child, name, depth + 1)
        if found is not None:
            return found
    return None


def _meta_is_indirect(meta):
    return "meta_ptr_indirect_impl" in meta.GetType().GetCanonicalType().GetName()


def _symbol_name(target, addr):
    """Resolve a load address to its (demangled) symbol name, or ``None``."""
    if not addr:
        return None
    sym = target.ResolveLoadAddress(addr).GetSymbol()
    if not sym.IsValid():
        return None
    return sym.GetName()


def _iter_reflectors(meta_storage):
    """Yield ``(is_direct, type_info_addr)`` for each embedded RTTI reflector."""

    def rec(val, depth=0):
        if depth > 8 or not val.IsValid():
            return
        for i in range(val.GetNumChildren()):
            child = val.GetChildAtIndex(i)
            tn = child.GetType().GetName()
            if "reflection_meta<" in tn and "proxy_typeid_reflector" in tn:
                is_direct = "reflection_meta<true" in tn.replace(" ", "")
                info = _find_child(child, "info")
                if info is not None and info.IsValid():
                    yield is_direct, info.GetValueAsUnsigned()
            else:
                yield from rec(child, depth + 1)

    yield from rec(meta_storage)


def _typeinfo_name(frame, info_addr):
    """Read and demangle the type name from a ``std::type_info*`` address."""
    if not info_addr:
        return None
    psz = 8
    expr = "*(const char**)(%d + %d)" % (info_addr, psz)
    val = frame.EvaluateExpression(expr)
    if not val.IsValid() or val.GetError().Fail():
        return None
    name = val.GetSummary()
    if not name:
        return None
    name = name.strip('"')
    if name and name[0] == "*":
        name = name[1:]
    return _demangle_type(name)


def _meta_storage(meta):
    """The ``composite_meta`` object: ``*meta_.ptr_`` (indirect) or the meta itself."""
    if _meta_is_indirect(meta):
        ptr = _find_child(meta, "ptr_")
        if ptr is None or ptr.GetValueAsUnsigned() == 0:
            return None
        return ptr.Dereference()
    return meta


def recover(proxy_val):
    """Return ``(state, payload)`` like the gdb script.

    state: ``"empty"`` | ``"pointer"`` (payload=P) | ``"pointee"`` (payload=T) |
    ``"unknown"``.
    """
    target = proxy_val.GetTarget()
    frame = proxy_val.GetFrame()
    meta = proxy_val.GetChildMemberWithName("meta_")

    if _meta_is_indirect(meta):
        ptr = _find_child(meta, "ptr_")
        if ptr is None or ptr.GetValueAsUnsigned() == 0:
            return "empty", None
        sym = _symbol_name(target, ptr.GetValueAsUnsigned())
        p = _extract_balanced(sym, ">::storage<") if sym else None
    else:
        invoke = _find_child(meta, "invoke")
        if invoke is None or invoke.GetValueAsUnsigned() == 0:
            return "empty", None
        sym = _symbol_name(target, invoke.GetValueAsUnsigned())
        p = _extract_balanced(sym, "in_place_type_t<") if sym else None

    if p:
        return "pointer", p

    storage = _meta_storage(meta)
    pointee = None
    if storage is not None:
        for is_direct, info_addr in _iter_reflectors(storage):
            name = _typeinfo_name(frame, info_addr)
            if name is None:
                continue
            if is_direct:
                return "pointer", name
            pointee = name
    if pointee is not None:
        return "pointee", pointee
    return "unknown", None


def _ptr_address(proxy_val):
    return proxy_val.GetChildMemberWithName("ptr_").GetLoadAddress()


def _normalize_type_name(name):
    # lldb's type lookup wants a space between consecutive closing angle brackets.
    return re.sub(r">(?=>)", "> ", name)


def _lookup_type(target, name):
    name = name.strip()
    if name.endswith("*"):
        base = target.FindFirstType(_normalize_type_name(name[:-1].strip()))
        return base.GetPointerType() if base.IsValid() else lldb.SBType()
    return target.FindFirstType(_normalize_type_name(name))


def _stored_value(proxy_val, name="stored"):
    """An ``SBValue`` for the stored pointer object (type ``P``), or ``None``."""
    state, payload = recover(proxy_val)
    if state != "pointer":
        return None
    sbtype = _lookup_type(proxy_val.GetTarget(), payload)
    if not sbtype.IsValid():
        return None
    val = proxy_val.CreateValueFromAddress(name, _ptr_address(proxy_val), sbtype)
    return val if val.IsValid() and val.GetError().Success() else None


def _first_template_arg(name):
    start = name.find("<")
    if start < 0:
        return None
    depth = 0
    end = -1
    for j in range(start, len(name)):
        if name[j] == "<":
            depth += 1
        elif name[j] == ">":
            depth -= 1
            if depth == 0:
                end = j
                break
    if end < 0:
        return None
    args = name[start + 1 : end]
    depth = 0
    for k, ch in enumerate(args):
        if ch == "<":
            depth += 1
        elif ch == ">":
            depth -= 1
        elif ch == "," and depth == 0:
            return args[:k].strip()
    return args.strip()


def _pointee_type_name(payload):
    """The pointee type ``T`` for a recovered pointer type ``P``."""
    if payload.endswith("*"):
        return payload[:-1].strip()
    return _first_template_arg(payload)


def _pointee_address(frame, proxy_val, payload):
    """Address of the pointee object held by the recovered pointer."""
    addr = _ptr_address(proxy_val)
    if "inplace_ptr<" in payload:  # object is stored inline at offset 0
        return addr
    # raw / unique_ptr / shared_ptr / allocated_ptr all keep the underlying
    # pointer as the first word of the holder.
    val = frame.EvaluateExpression("*(void**)(%d)" % addr)
    return val.GetValueAsUnsigned() if val.GetError().Success() else 0


def proxy_summary(valobj, internal_dict):
    facade = valobj.GetType().GetCanonicalType().GetName()
    state, payload = recover(valobj.GetNonSyntheticValue())
    if state == "empty":
        return "%s [empty]" % facade
    if state == "pointer":
        return "%s [holds %s]" % (facade, payload)
    if state == "pointee":
        return "%s [holds pointer to %s; pointer type unavailable]" % (facade, payload)
    return (
        "%s [non-empty; type unavailable: build with -g, keep symbols, or add skills::rtti]"
        % facade
    )


class ProxySynthetic:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj
        self.stored = None

    def update(self):
        self.stored = _stored_value(self.valobj.GetNonSyntheticValue())
        return False

    def has_children(self):
        return self.stored is not None

    def num_children(self):
        return 1 if self.stored is not None else 0

    def get_child_index(self, name):
        return 0 if name == "stored" and self.stored is not None else -1

    def get_child_at_index(self, index):
        return self.stored if index == 0 else None


def _resolve(frame, expr):
    val = frame.FindVariable(expr)
    if val.IsValid():
        return val
    return frame.EvaluateExpression(expr)


def _frame(debugger):
    return (
        debugger.GetSelectedTarget().GetProcess().GetSelectedThread().GetSelectedFrame()
    )


def cmd_proxy_type(debugger, command, result, internal_dict):
    proxy_val = _resolve(_frame(debugger), command.strip())
    _state, payload = recover(proxy_val.GetNonSyntheticValue())
    result.AppendMessage(payload if payload else "<unknown>")


def cmd_proxy_ptr(debugger, command, result, internal_dict):
    stored = _stored_value(
        _resolve(_frame(debugger), command.strip()).GetNonSyntheticValue()
    )
    if stored is None:
        result.SetError("could not recover the contained pointer type")
        return
    result.AppendMessage(str(stored))


def cmd_proxy_call(debugger, command, result, internal_dict):
    # Call through the held pointer, e.g. `proxy_call my_proxy ->Speak()`.
    var, _, tail = command.strip().partition(" ")
    frame = _frame(debugger)
    proxy_val = _resolve(frame, var).GetNonSyntheticValue()
    _state, payload = recover(proxy_val)
    tname = _pointee_type_name(payload) if payload else None
    if not tname:
        result.SetError("could not recover the contained pointer type")
        return
    expr = "((%s*)(%d))%s" % (
        tname,
        _pointee_address(frame, proxy_val, payload),
        tail.strip(),
    )
    out = frame.EvaluateExpression(expr)
    if out.GetError().Fail():
        result.SetError(out.GetError().GetCString())
        return
    result.AppendMessage(out.GetSummary() or str(out))


def __lldb_init_module(debugger, internal_dict):
    mod = __name__
    debugger.HandleCommand(
        "type summary add -x '^pro::v4::proxy<' -F %s.proxy_summary" % mod
    )
    debugger.HandleCommand(
        "type synthetic add -x '^pro::v4::proxy<' -l %s.ProxySynthetic" % mod
    )
    for cmd in ("proxy_type", "proxy_ptr", "proxy_call"):
        debugger.HandleCommand("command script add -f %s.cmd_%s %s" % (mod, cmd, cmd))
    print("proxy: lldb formatters loaded (proxy_type, proxy_ptr, proxy_call)")
