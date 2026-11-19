# Function template `reflect` (`box_view<F, MP>`)

```cpp
template <class M>
const M& reflect(const box_view<F, MP>& v) noexcept;
```

Acquires reflection information of the type of the value referred to by a `box_view<F, MP>`, through an *indirect* reflection.

Equivalent to [`reflect`](../proxy_indirect_accessor/friend_reflect.md)`<M>(ia)`, where `ia` is the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<view_facade<F>, MP>` of the underlying [`proxy_view`](../proxy_view.md)`<F, MP>`. The behavior is undefined if `v` does not refer to a value.

There shall be a reflection type `Refl` defined in the reflection types of `F` or of any super of `F`, reachable via `typename F::super_types` transitively, where

- `Refl::is_direct` is `false`, and
- `typename Refl::metadata_type` is `M`.

The reference obtained from `reflect()` may be invalidated if `v` is subsequently modified.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box_view<F, MP>` is an associated class of the arguments.

## Example

```cpp
#include <iostream>
#include <typeinfo>

#include <proxy/box.h>

struct RttiAware : pro::facade_builder               //
                   ::add_skill<pro::skills::rtti>    //
                   ::add_skill<pro::skills::as_view> //
                   ::build {};

int main() {
  pro::box<RttiAware> b = 123;
  pro::box_view<RttiAware> v = b;
  std::cout << std::boolalpha << (proxy_typeid(v) == typeid(int))
            << "\n"; // Prints "true"
  proxy_cast<int&>(v) = 456;
  std::cout << proxy_cast<int>(b) << "\n"; // Prints "456"
}
```

## See Also

- [function template `invoke` (`box_view<F, MP>`)](friend_invoke.md)
- [function template `reflect` (`box<F, MP>`)](../box/friend_reflect.md)
