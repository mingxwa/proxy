# Macro `PRO_DEF_MEM_DISPATCH`

> Header: `proxy_macros.h` and `proxy.h`

```cpp
#define PRO_DEF_MEM_DISPATCH // see below
```

Macro `PRO_DEF_MEM_DISPATCH` defines dispatch types for member function expressions with accessibility. It supports two syntaxes:

```cpp
// (1)
PRO_DEF_MEM_DISPATCH(dispatch_name, func_name);

// (2)
PRO_DEF_MEM_DISPATCH(dispatch_name, func_name, accessibility_func_name);
```

`(1)` Equivalent to `PRO_DEF_MEM_DISPATCH(dispatch_name, func_name, func_name)`.

`(2)` Defines a class named `dispatch_name` of member function call expressions of `func_name` with accessibility via member function overloads named `accessibility_func_name`. The accessibility is provided by the nested class `dispatch_name::access_type` (see [*ProAccessible* requirements](ProAccessible.md)). Effectively equivalent to:

```cpp
struct dispatch_name {
  struct access_type {
    template <class Self, class... Ds>
    struct accessor {
      accessor() = delete;
    };
    template <class Self, class... Ds>
        requires(sizeof...(Ds) > 1u && (std::is_constructible_v<accessor<Self, Ds>> && ...))
    struct accessor<Self, Ds...> : accessor<Self, Ds>... {
      using accessor<Self, Ds>::accessibility_func_name ...;
    };
    template <class Self, class D, class R, class... Args>
    struct accessor<Self, pro::proxy_operation<D, R(Args...) cv ref noex>> {
      R accessibility_func_name(Args... args) cv ref noex {
        return invoke<D, R(Args...) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(*this), std::forward<Args>(args)...);
      }
    };
  };

  template <class T, class... Args>
  decltype(auto) operator()(T&& self, Args&&... args) const
      noexcept(noexcept(std::forward<T>(self).func_name(std::forward<Args>(args)...)))
      requires(requires { std::forward<T>(self).func_name(std::forward<Args>(args)...); }) {
    return std::forward<T>(self).func_name(std::forward<Args>(args)...);
  }
}
```

The accessor of `dispatch_name::access_type` for multiple descriptors makes the `accessibility_func_name` of every descriptor visible. Therefore, the overloads of the conventions of `dispatch_name`, and of any other dispatch type sharing this access type such as [`weak_dispatch<dispatch_name>`](weak_dispatch/README.md), are all candidate functions when `accessibility_func_name` is called.

*Since 5.0.0*: the accessor is a member template of `dispatch_name::access_type` taking descriptors. Previously, it was a member template of `dispatch_name` taking the dispatch type and the overload types.

When headers from different major versions of the Proxy library can appear in the same translation unit (for example, Proxy 4 and Proxy 5), use the major-qualified form `PRO<major>_DEF_MEM_DISPATCH` (e.g., `PRO5_DEF_MEM_DISPATCH`).

## Example

```cpp
#include <iostream>
#include <string>
#include <vector>

#include <proxy/proxy.h>

PRO_DEF_MEM_DISPATCH(MemAt, at);

struct Dictionary : pro::facade_builder                                   //
                    ::add_convention<MemAt, std::string(int index) const> //
                    ::build {};

int main() {
  std::vector<const char*> v{"hello", "world"};
  pro::proxy<Dictionary> p = &v;
  std::cout << p->at(1) << "\n"; // Prints "world"
}
```

## See Also

- [macro `PRO_DEF_FREE_DISPATCH`](PRO_DEF_FREE_DISPATCH.md)
- [alias template `basic_facade_builder::add_convention`](basic_facade_builder/add_convention.md)
