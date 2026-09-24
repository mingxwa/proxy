# Class template `operator_access`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: `pro::inline v5`  
> Since: 5.0.0

The definition of `operator_access` makes use of an exposition-only type *string-literal*, which is constructible from a `char` array at compile-time and can be used as a non-type template argument.

```cpp
template <string-literal Sign, bool Rhs = false>
struct operator_access;
```

Class template `operator_access` is an access type that provides accessibility to [`proxy`](../proxy/README.md) via operator expressions. It meets the [*ProAccessible* requirements](../ProAccessible.md) of applicable types. `Sign` represents the sign of operator (SOP) as a string literal (e.g., `"+"` for operator `+`). `Rhs` specifies whether the `proxy` operand is on the right-hand side of a binary operator.

`operator_access<Sign, Rhs>` is the `access_type` of [`operator_dispatch<Sign, Rhs>`](../operator_dispatch/README.md), and is defined for the same SOPs. It can also be the `access_type` of any other [dispatch](../ProDispatch.md) type. The direct, or the indirect, conventions whose access type is `operator_access<Sign, Rhs>` share an accessor (see [*ProAccessible* requirements](../ProAccessible.md)). Therefore, the conventions of `operator_dispatch<Sign, Rhs>` and of other dispatch types with this access type participate in overload resolution together when the operator expression is evaluated.

## Supported SOPs

`operator_access` supports the same 37 SOPs as [`operator_dispatch`](../operator_dispatch/README.md#supported-sops). The expressions available for each specialization of `operator_access` are listed in the [specializations of `operator_dispatch`](../operator_dispatch/README.md#specializations).

## Member Types

| Name                      | Description                       |
| ------------------------- | --------------------------------- |
| [`accessor`](accessor.md) | provides accessibility to `proxy` |

## Example

```cpp
#include <iostream>

#include <proxy/proxy.h>

struct RunDispatch {
  using access_type = pro::operator_access<"()">;

  template <class T>
  void operator()(T& self) const {
    self.Run();
  }
};

struct Runnable : pro::facade_builder                                       //
                  ::add_convention<pro::operator_dispatch<"()">, void(int)> //
                  ::add_convention<RunDispatch, void()>                     //
                  ::build {};

struct Job {
  void operator()(int n) { std::cout << "Called with " << n << "\n"; }
  void Run() { std::cout << "Run\n"; }
};

int main() {
  Job job;
  pro::proxy<Runnable> p = &job;
  (*p)(123); // Prints "Called with 123"
  (*p)();    // Prints "Run"
}
```

## See Also

- [class template `operator_dispatch`](../operator_dispatch/README.md)
- [class `explicit_conversion_access`](../explicit_conversion_access/README.md)
- [class `implicit_conversion_access`](../implicit_conversion_access/README.md)
