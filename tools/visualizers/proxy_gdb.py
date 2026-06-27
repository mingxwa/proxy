# Copyright (c) 2022-2026 Microsoft Corporation.
# Copyright (c) 2026-Present Next Gen C++ Foundation.
# Licensed under the MIT License.

"""GDB pretty-printer and helpers for ``pro::proxy<F>``.

``pro::proxy<F>`` is type-erased: the contained pointer object ``P`` lives in the
raw ``ptr_`` buffer and the only runtime trace of ``P`` is the ``meta_`` member.
This script recovers the pointer type ``P`` and exposes the stored pointer object
so it can be inspected and called into with ordinary debugger syntax.

Type recovery (in order):

* **Mechanism 1 (primary):** resolve a per-``P`` symbol reachable from ``meta_``.
  - *indirect* meta -> ``meta_.ptr_`` points at ``meta_ptr_indirect_impl<M>::storage<P>``
  - *direct*   meta -> first ``conv_meta::invoke`` targets ``...conv_meta<P>(in_place_type_t<P>)...``
  Requires an unstripped symbol table (or DWARF that names these entities).
* **Mechanism 2 (fallback, strip-tolerant):** read ``&typeid(P)`` embedded by
  ``skills::direct_rtti`` (``reflection_meta<true, proxy_typeid_reflector>``),
  distinguished from ``skills::rtti`` / ``indirect_rtti``'s ``&typeid(T)``
  (``reflection_meta<false, ...>``, the pointee, used only for display when ``P``
  cannot be recovered).

Usage::

    (gdb) source tools/visualizers/proxy_gdb.py
    (gdb) print my_proxy              # [holds P] + the stored pointer object
    (gdb) print $pro_ptr(my_proxy)    # the stored P itself
    (gdb) print $pro_ptr(my_proxy)->SomeMethod()   # call through P

Requires the program to be built with debug info (``-g``).
"""

import re
import subprocess

import gdb

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
    """Return the ``<...>`` argument that follows ``marker`` (which ends in '<').

    ``marker`` is matched at its last occurrence so that nested template names do
    not shadow the one we want.
    """
    pos = text.rfind(marker)
    if pos < 0:
        return None
    i = pos + len(marker)
    depth = 1
    start = i
    while i < len(text) and depth:
        c = text[i]
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
            if depth == 0:
                return text[start:i].strip()
        i += 1
    return None


def _info_symbol(addr):
    """Resolve an address to its symbol name, or ``None`` if unavailable."""
    try:
        out = gdb.execute("info symbol 0x%x" % (addr & ((1 << 64) - 1)), to_string=True)
    except gdb.error:
        return None
    out = out.strip()
    if not out or out.startswith("No symbol"):
        return None
    name = out.split(" in section ", 1)[0]
    # Drop a trailing " + <offset>" if present.
    return re.sub(r"\s*\+\s*\d+$", "", name).strip()


def _meta_is_indirect(meta_val):
    return "meta_ptr_indirect_impl" in str(meta_val.type.strip_typedefs())


def _first_field(val, name):
    """Depth-first search for a (possibly inherited) member named ``name``."""
    try:
        fields = val.type.strip_typedefs().fields()
    except (gdb.error, TypeError):
        return None
    for f in fields:
        if f.is_base_class:
            found = _first_field(val[f], name)
            if found is not None:
                return found
        elif f.name == name:
            return val[f]
    return None


def _iter_reflectors(meta_val):
    """Yield ``(is_direct, type_info_ptr)`` for each embedded RTTI reflector."""

    def rec(val):
        try:
            fields = val.type.strip_typedefs().fields()
        except (gdb.error, TypeError):
            return
        for f in fields:
            if not f.is_base_class:
                continue
            tn = str(f.type)
            if "reflection_meta<" in tn and "proxy_typeid_reflector" in tn:
                is_direct = "reflection_meta<true" in tn.replace(" ", "")
                info = _first_field(val[f], "info")
                if info is not None:
                    yield is_direct, info
            else:
                yield from rec(val[f])

    yield from rec(meta_val)


def _typeinfo_name(info_ptr):
    """Read and demangle the type name from a ``std::type_info*`` value."""
    psz = gdb.lookup_type("char").pointer().sizeof
    try:
        name = gdb.parse_and_eval(
            "*(const char**)(%d + %d)" % (int(info_ptr), psz)
        ).string()
    except gdb.error:
        return None
    if name and name[0] == "*":  # cross-module uniqueness marker
        name = name[1:]
    return _demangle_type(name)


def _meta_storage_value(meta_val):
    """The ``composite_meta`` object: ``*meta_.ptr_`` (indirect) or the meta itself."""
    if _meta_is_indirect(meta_val):
        ptr = _first_field(meta_val, "ptr_")
        if ptr is None or int(ptr) == 0:
            return None
        return ptr.dereference()
    return meta_val


def recover(proxy_val):
    """Return ``(state, payload)``.

    state is one of: ``"empty"``, ``"pointer"`` (payload = P type name),
    ``"pointee"`` (payload = T type name, P unknown), ``"unknown"`` (payload=None).
    """
    meta = proxy_val["meta_"]
    indirect = _meta_is_indirect(meta)

    # Emptiness + the address to resolve for Mechanism 1.
    if indirect:
        ptr = _first_field(meta, "ptr_")
        if ptr is None or int(ptr) == 0:
            return "empty", None
        sym = _info_symbol(int(ptr))
        p = _extract_balanced(sym, ">::storage<") if sym else None
    else:
        invoke = _first_field(meta, "invoke")
        if invoke is None or int(invoke) == 0:
            return "empty", None
        sym = _info_symbol(int(invoke))
        p = _extract_balanced(sym, "in_place_type_t<") if sym else None

    if p:
        return "pointer", p

    # Mechanism 2 fallback (strip-tolerant): typeid(P) via direct_rtti.
    storage = _meta_storage_value(meta)
    pointee = None
    if storage is not None:
        for is_direct, info in _iter_reflectors(storage):
            name = _typeinfo_name(info)
            if name is None:
                continue
            if is_direct:
                return "pointer", name
            pointee = name
    if pointee is not None:
        return "pointee", pointee
    return "unknown", None


def _stored_pointer(proxy_val):
    """A ``gdb.Value`` for the stored pointer object ``*(P*)&ptr_``, or ``None``."""
    state, payload = recover(proxy_val)
    if state != "pointer":
        return None
    addr = int(proxy_val["ptr_"].address)
    try:
        return gdb.parse_and_eval("*(%s*)(%d)" % (payload, addr))
    except gdb.error:
        return None


class ProxyPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        facade = str(self.val.type.strip_typedefs())
        state, payload = recover(self.val)
        if state == "empty":
            return "%s [empty]" % facade
        if state == "pointer":
            return "%s [holds %s]" % (facade, payload)
        if state == "pointee":
            return "%s [holds pointer to %s; pointer type unavailable]" % (
                facade,
                payload,
            )
        return (
            "%s [non-empty; type unavailable: build with -g, keep symbols, or add skills::rtti]"
            % facade
        )

    def children(self):
        stored = _stored_pointer(self.val)
        if stored is not None:
            yield "stored", stored


def _lookup(val):
    t = val.type
    if t.code == gdb.TYPE_CODE_REF:
        t = t.target()
    t = t.strip_typedefs()
    if t.code == gdb.TYPE_CODE_STRUCT and _PROXY_RE.match(str(t)):
        return ProxyPrinter(val)
    return None


class _ProPtr(gdb.Function):
    """$pro_ptr(p): the stored pointer object P (inspect it or call through it)."""

    def __init__(self):
        super().__init__("pro_ptr")

    def invoke(self, proxy_val):
        stored = _stored_pointer(proxy_val)
        if stored is None:
            raise gdb.error("pro_ptr: could not recover the contained pointer type")
        return stored


class _ProType(gdb.Function):
    """$pro_type(p): the recovered contained type as a string."""

    def __init__(self):
        super().__init__("pro_type")

    def invoke(self, proxy_val):
        _state, payload = recover(proxy_val)
        return payload if payload is not None else "<unknown>"


def register(objfile=None):
    target = objfile if objfile is not None else gdb
    target.pretty_printers.append(_lookup)


_ProPtr()
_ProType()
register()
print("proxy: gdb pretty-printer loaded ($pro_ptr, $pro_type available)")
