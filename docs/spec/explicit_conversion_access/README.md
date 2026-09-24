# Class `explicit_conversion_access`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: `pro::inline v5`  
> Since: 5.0.0

```cpp
struct explicit_conversion_access;
```

Class `explicit_conversion_access` is an access type that provides accessibility to [`proxy`](../proxy/README.md) via explicit type conversion expressions. It meets the [*ProAccessible* requirements](../ProAccessible.md) of applicable types.

`explicit_conversion_access` is the `access_type` of [`explicit_conversion_dispatch`](../explicit_conversion_dispatch/README.md). It can also be the `access_type` of any other [dispatch](../ProDispatch.md) type whose conventions convert a `proxy` to the return types of their overloads. The direct, or the indirect, conventions whose access type is `explicit_conversion_access` share an accessor (see [*ProAccessible* requirements](../ProAccessible.md)), so the conversions of all of them are available together.

## Member Types

| Name                      | Description                       |
| ------------------------- | --------------------------------- |
| [`accessor`](accessor.md) | provides accessibility to `proxy` |

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/proxy.h>

struct ToStringDispatch {
  using access_type = pro::explicit_conversion_access;

  template <class T>
  std::string operator()(const T& self) const {
    return std::to_string(self);
  }
};

struct Stringable : pro::facade_builder                                     //
                    ::add_convention<ToStringDispatch, std::string() const> //
                    ::build {};

int main() {
  pro::proxy<Stringable> p = pro::make_proxy<Stringable>(123);
  std::cout << static_cast<std::string>(*p) << "\n"; // Prints "123"
}
```

## See Also

- [class `explicit_conversion_dispatch`](../explicit_conversion_dispatch/README.md)
- [class `implicit_conversion_access`](../implicit_conversion_access/README.md)
- [class template `operator_access`](../operator_access/README.md)
