# Macro `PRO_DEF_FREE_DISPATCH`

> Header: `proxy_macros.h` and `proxy.h`

```cpp
#define PRO_DEF_FREE_DISPATCH // see below
```

Macro `PRO_DEF_FREE_DISPATCH` defines dispatch types for free function expressions with accessibility. It supports two syntaxes:

```cpp
// (1)
PRO_DEF_FREE_DISPATCH(dispatch_name, func_name);

// (2)
PRO_DEF_FREE_DISPATCH(dispatch_name, func_name, accessibility_func_name);
```

`(1)` Equivalent to `PRO_DEF_FREE_DISPATCH(dispatch_name, func_name, func_name)`.

`(2)` Defines a class named `dispatch_name` of free function call expressions of `func_name` with accessibility via free function overloads named `accessibility_func_name`. The accessibility is provided by the nested class `dispatch_name::access_type` (see [*ProAccessible* requirements](ProAccessible.md)). Effectively equivalent to:

```cpp
struct dispatch_name {
  struct access_type {
    template <class Self, class... Ds>
    struct accessor {
      accessor() = delete;
    };
    template <class Self, class... Ds>
        requires(sizeof...(Ds) > 1u && (std::is_constructible_v<accessor<Self, Ds>> && ...))
    struct accessor<Self, Ds...> : accessor<Self, Ds>... {};
    template <class Self, class D, class R, class... Args>
    struct accessor<Self, pro::proxy_operation<D, R(Args...) cv ref noex>> {
      friend R accessibility_func_name(Self cv <ref ? ref : &> self, Args... args) noex {
        return invoke<D, R(Args...) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), std::forward<Args>(args)...);
      }
    };
  };

  template <class T, class... Args>
  decltype(auto) operator()(T&& self, Args&&... args) const
      noexcept(noexcept(func_name(std::forward<T>(self), std::forward<Args>(args)...)))
      requires(requires { func_name(std::forward<T>(self), std::forward<Args>(args)...); }) {
    return func_name(std::forward<T>(self), std::forward<Args>(args)...);
  }
}
```

*Since 5.0.0*: the accessor is a member template of `dispatch_name::access_type` taking descriptors. Previously, it was a member template of `dispatch_name` taking the dispatch type and the overload types.

When headers from different major versions of the Proxy library can appear in the same translation unit (for example, Proxy 4 and Proxy 5), use the major-qualified form `PRO<major>_DEF_FREE_DISPATCH` (e.g., `PRO5_DEF_FREE_DISPATCH`).

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/proxy.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                           //
                    ::add_convention<FreeToString, std::string()> //
                    ::build {};

int main() {
  pro::proxy<Stringable> p = pro::make_proxy<Stringable>(123);
  std::cout << ToString(*p) << "\n"; // Prints "123"
}
```

## See Also

- [macro `PRO_DEF_MEM_DISPATCH`](PRO_DEF_MEM_DISPATCH.md)
- [macro `PRO_DEF_FREE_AS_MEM_DISPATCH`](PRO_DEF_FREE_AS_MEM_DISPATCH.md)
- [alias template `basic_facade_builder::add_convention`](basic_facade_builder/add_convention.md)
