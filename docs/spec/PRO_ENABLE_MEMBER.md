# Macro `PRO_ENABLE_MEMBER`

> Header: `proxy_macros.h` and `proxy.h`<br />Since: 5.0.0

```cpp
PRO_ENABLE_MEMBER(name);
```

After including `proxy.h`, invoke this macro at global namespace scope before using the specializations it defines. `name` shall be a single unqualified function identifier. The macro defines separate explicit specializations of [`member_dispatch<"name">`](member_dispatch.md) and [`member_access<"name">`](member_access/README.md). The string is the spelling of the identifier after macro argument expansion. The same name shall be enabled at most once per translation unit for this macro. Put the invocation in a guarded header when sharing the specializations across translation units. The member and free macros may enable the same name independently.

The macro does not register a default access type or couple the dispatch to the access type. A convention selects both explicitly. To expose a free call as a member, for example, combine `free_dispatch<"name">` with `member_access<"name">` after enabling both. A different access name can be selected by enabling that name separately. Qualified function names, member data access, and function template argument lists are not accepted as macro arguments.

Use `PRO5_ENABLE_MEMBER` when headers from multiple major versions may appear in one translation unit. This is the version-qualified form of the same macro.

## Example

```cpp
#include <iostream>
#include <proxy/proxy.h>
#include <tuple>

struct Buffer {
  int size() const noexcept { return 42; }
};

PRO_ENABLE_MEMBER(size);

struct SizeConvention {
  static constexpr bool is_direct = false;
  using dispatch_type = pro::member_dispatch<"size">;
  using overload_type = int() const noexcept;
  using access_type = pro::member_access<"size">;
};

struct Sized : pro::facade_builder::build {
  using convention_types = std::tuple<SizeConvention>;
};

int main() {
  Buffer buffer;
  pro::proxy<Sized> p = &buffer;
  std::cout << p->size() << "\n";
}
```

## Modules

When using `import proxy.v5`, include `proxy/proxy_macros.h` and the standard library headers needed by the macro in the global module fragment. Place the invocation after the import in an `extern "C++"` block at global namespace scope, as in the [module examples](../modules_support.md). Macro definitions are not exported by the module.
