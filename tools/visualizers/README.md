# Debugger visualizers for `pro::proxy`

A `pro::proxy<F>` is type-erased: the contained pointer object `P` lives in the
raw `ptr_` buffer, and the only runtime trace of its type is the `meta_` member.
These visualizers recover the pointer type `P` so that, in a debugger, you can
see what a proxy holds and call through the stored pointer with ordinary syntax.

| Debugger | File | Status |
| --- | --- | --- |
| GDB | [`proxy_gdb.py`](proxy_gdb.py) | available |
| LLDB | _planned_ | — |
| MSVC (natvis) | _planned_ | — |

Build your program with debug info (`-g`) for any of these to work.

## How type recovery works

1. **Mechanism 1 (primary).** A per-`P` symbol is reachable from `meta_`:
   * *indirect* meta — `meta_.ptr_` points at
     `detail::meta_ptr_indirect_impl<M>::storage<P>`;
   * *direct* meta — the first `conv_meta::invoke` targets a per-`P` thunk named
     `...conv_meta<P>(in_place_type_t<P>)...`.

   The symbol name carries `P`. Needs an unstripped symbol table (or DWARF that
   names these entities).
2. **Mechanism 2 (fallback, survives `strip`).** When a facade opts into
   [`skills::direct_rtti`](https://ngcpp.github.io/proxy/spec/skills_rtti/), the
   `meta_` embeds `&typeid(P)`; the visualizer reads and demangles it. This is
   distinguished from `skills::rtti` / `skills::indirect_rtti`, which embed
   `&typeid(T)` for the pointee `T` (used only for display when `P` cannot be
   recovered).

Neither mechanism changes the layout or ABI of `proxy`.

## GDB

Load the printer (e.g. from your `~/.gdbinit`, or per session):

```
(gdb) source /path/to/proxy/tools/visualizers/proxy_gdb.py
```

Then:

```
(gdb) print p                       # AnimalFacade [holds std::unique_ptr<Cat, ...>] = {stored = ...}
(gdb) print $pro_type(p)            # "std::unique_ptr<Cat, std::default_delete<Cat> >"
(gdb) print $pro_ptr(p)             # the stored pointer object P itself
(gdb) print $pro_ptr(p)->Speak()    # call through the stored pointer
```

* `$pro_ptr(p)` returns the stored pointer object `P`. Inspect it, or call
  through it — `->` resolves natively for raw pointers and via gdb's xmethods for
  `std::unique_ptr` / `std::shared_ptr` (libstdc++).
* `$pro_type(p)` returns the recovered type of `P` as a string.

## Tests

[`tests/run_gdb_tests.py`](tests/run_gdb_tests.py) compiles
[`tests/test_subjects.cpp`](tests/test_subjects.cpp) and drives gdb with
[`tests/gdb_driver.py`](tests/gdb_driver.py), asserting recovery and calls for
raw pointers, `std::unique_ptr`, `std::shared_ptr`, `make_proxy` storage, empty
proxies, and the RTTI variants:

```sh
python3 tools/visualizers/tests/run_gdb_tests.py            # uses g++
CXX=clang++-22 python3 tools/visualizers/tests/run_gdb_tests.py
```

CI runs this for both GCC and Clang (see `.github/workflows/bvt-visualizers.yml`).
