# Function template `invoke` (`box_view<F, MP>`)

```cpp
template <class D, class S, class... Args>
return-type-of<S> invoke(box_view<F, MP>& v, Args&&... args);
template <class D, class S, class... Args>
return-type-of<S> invoke(const box_view<F, MP>& v, Args&&... args);
template <class D, class S, class... Args>
return-type-of<S> invoke(box_view<F, MP>&& v, Args&&... args);
template <class D, class S, class... Args>
return-type-of<S> invoke(const box_view<F, MP>&& v, Args&&... args);
```

Invokes a `box_view<F, MP>` with a specified dispatch type `D`, a signature type `S`, and arguments, through an *indirect* operation. Let `R` be the return type of `S`. `return-type-of<S>` is `R`.

Equivalent to [`invoke`](../proxy_indirect_accessor/friend_invoke.md)`<D, S>(ia, std::forward<Args>(args)...)`, where `ia` is the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<view_facade<F>, MP>` of the underlying [`proxy_view`](../proxy_view.md)`<F, MP>` with the same cv ref-qualifiers. The behavior is undefined if `v` does not refer to a value.

There shall be an operation type `Op` defined in the operation types of `F` or of any super of `F`, reachable via `typename F::super_types` transitively, where

- `Op::is_direct` is `false`, and
- `typename Op::dispatch_type` is `D`, and
- [`substituted-signature`](../ProxySignature.md)`<typename Op::signature_type, view_facade<F>, MP>` is `S`.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box_view<F, MP>` is an associated class of the arguments.

## Notes

It is generally not recommended to call `invoke` directly. Using an [`accessor`](../ProxyAccess.md) is usually a better option with easier and more descriptive syntax.

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/box.h>

PRO_DEF_MEM_DISPATCH(MemLength, length);

struct Sized : pro::facade_builder                              //
               ::add_convention<MemLength, std::size_t() const> //
               ::add_skill<pro::skills::as_view>                //
               ::build {};

int main() {
  pro::box<Sized> b{std::in_place_type<std::string>, "hello"};
  pro::box_view<Sized> v = b;
  std::cout << v.length() << "\n"; // Invokes with accessor, prints "5"
  std::cout << invoke<MemLength, std::size_t() const>(v)
            << "\n"; // Invokes with the non-member invoke, also prints "5"
}
```

## See Also

- [function template `reflect` (`box_view<F, MP>`)](friend_reflect.md)
- [function template `invoke` (`box<F, MP>`)](../box/friend_invoke.md)
