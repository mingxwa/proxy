# Debugger visualizers for `pro::proxy`

A `pro::proxy<F>` is type-erased: the contained pointer object `P` lives in the
raw `ptr_` buffer, and the only runtime trace of its type is the `meta_` member.
These visualizers recover the pointer type `P` so that, in a debugger, you can
see what a proxy holds and call through the stored pointer with ordinary syntax.

| Debugger | File | Status |
| --- | --- | --- |
| GDB | [`proxy_gdb.py`](proxy_gdb.py) | available |
| LLDB | [`proxy_lldb.py`](proxy_lldb.py) | available |
| MSVC (natvis) | [`proxy.natvis`](proxy.natvis) | available (rtti only, see below) |

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

## LLDB

Load the formatters (e.g. from your `~/.lldbinit`, or per session):

```
(lldb) command script import /path/to/proxy/tools/visualizers/proxy_lldb.py
```

Then:

```
(lldb) frame variable p          # AnimalFacade [holds std::unique_ptr<Cat, ...>]
(lldb) proxy_type p              # std::unique_ptr<Cat, std::default_delete<Cat> >
(lldb) proxy_ptr p               # the stored pointer object P itself
(lldb) proxy_call p ->Speak()    # call through the stored pointer
```

* A summary and a synthetic `stored` child show what the proxy holds (LLDB's own
  formatters then expand `std::unique_ptr` / `std::shared_ptr`).
* `proxy_call p ->Speak()` calls through the held pointer. (LLDB's expression
  parser cannot name `std::unique_ptr` in a cast, so the command resolves the
  held object itself and calls through it.)

## MSVC (natvis)

Add [`proxy.natvis`](proxy.natvis) to your project (Visual Studio picks up a
`.natvis` in the project/solution, or `%USERPROFILE%\Documents\Visual Studio
<ver>\Visualizers\`; WinDbg/cdb load it with `.nvload`).

Unlike gdb and lldb, **natvis is declarative** — it cannot resolve symbols,
demangle, or cast to a runtime-determined type. So it can recover the contained
type only when the facade enables
[`pro::skills::direct_rtti`](https://ngcpp.github.io/proxy/spec/skills_rtti/)
(which embeds a `std::type_info`); then the proxy displays
`{ pro::proxy holds <type> }` with an expandable `[contained type]`. Without
RTTI the proxy shows a generic `{ pro::proxy }` plus its raw `meta_` / `ptr_`.
For full recovery, casting, and calls on Windows, use gdb or lldb (e.g. under
WSL or clang), or read the type from `proxy_typeid`.

## Tests

`tests/run_gdb_tests.py`, `tests/run_lldb_tests.py`, and
`tests/run_msvc_tests.ps1` compile
[`tests/test_subjects.cpp`](tests/test_subjects.cpp) and drive the respective
debugger ([`tests/gdb_driver.py`](tests/gdb_driver.py) /
[`tests/lldb_driver.py`](tests/lldb_driver.py)), asserting recovery and calls
for raw pointers, `std::unique_ptr`, `std::shared_ptr`, `make_proxy` storage,
empty proxies, and the RTTI variants:

```sh
python3 tools/visualizers/tests/run_gdb_tests.py             # uses g++
CXX=clang++-22 python3 tools/visualizers/tests/run_gdb_tests.py
CXX=clang++-22 LLDB=lldb-22 python3 tools/visualizers/tests/run_lldb_tests.py
```

`run_msvc_tests.ps1` (run from a Developer PowerShell) builds the subjects with
`cl`, validates the natvis, and drives `cdb` to confirm the contained type
surfaces for a `direct_rtti` proxy.

CI runs gdb (GCC + Clang), lldb (Clang), and the natvis/cdb check (MSVC); see
`.github/workflows/bvt-visualizers.yml`.
