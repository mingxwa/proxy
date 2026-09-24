# Class template `proxy_reflection`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: `pro::inline v5`  
> Since: 5.0.0

```cpp
template <class M>
struct proxy_reflection {};
```

Class template `proxy_reflection` is a *descriptor* of a reflection of a [`proxy`](proxy/README.md) whose reflector type is `M`. Descriptors are the template arguments following `Self` of the member template `accessor` of an access type (see [*ProAccessible* requirements](ProAccessible.md)), telling the access type which operations and reflections its accessor provides accessibility for.

For a reflection type `R` of a [facade](facade.md) type `F` or of any super of `F`, the descriptor of `R` in [`proxy<F, MP>`](proxy/README.md) or [`proxy_indirect_accessor<F, MP>`](proxy_indirect_accessor/README.md) is `proxy_reflection<typename R::reflector_type>`. Within an accessor, the described reflection is acquired by calling [`reflect<M>`](proxy/friend_reflect.md) with the `Self` object.

## Example

```cpp
#include <iostream>
#include <typeinfo>

#include <proxy/proxy.h>

struct TypeNameAccess {
  template <class Self, class... Ds>
  struct accessor {
    accessor() = delete;
  };
  template <class Self, class M>
  struct accessor<Self, pro::proxy_reflection<M>> {
    friend const char* TypeName(const Self& self) noexcept {
      return reflect<M>(self).Info->name();
    }
  };
};

struct TypeInfoReflector {
  using access_type = TypeNameAccess;

  TypeInfoReflector() = default;
  template <class T>
  constexpr explicit TypeInfoReflector(std::in_place_type_t<T>) noexcept
      : Info(&typeid(T)) {}

  const std::type_info* Info;
};

struct TypeNameAware : pro::facade_builder                          //
                       ::add_direct_reflection<TypeInfoReflector>   //
                       ::add_indirect_reflection<TypeInfoReflector> //
                       ::build {};

int main() {
  int a = 123;
  pro::proxy<TypeNameAware> p = &a;
  std::cout << TypeName(p) << "\n";  // Prints the name of int*
  std::cout << TypeName(*p) << "\n"; // Prints the name of int
}
```

## See Also

- [class template `proxy_operation`](proxy_operation.md)
- [named requirements *ProAccessible*](ProAccessible.md)
- [named requirements *ProBasicReflection*](ProBasicReflection.md)
