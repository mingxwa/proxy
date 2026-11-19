# Function template `invoke` (`box<F, MP>`)

```cpp
template <class D, class S, class... Args>
return-type-of<S> invoke(box<F, MP>& b, Args&&... args);
template <class D, class S, class... Args>
return-type-of<S> invoke(const box<F, MP>& b, Args&&... args);
template <class D, class S, class... Args>
return-type-of<S> invoke(box<F, MP>&& b, Args&&... args);
template <class D, class S, class... Args>
return-type-of<S> invoke(const box<F, MP>&& b, Args&&... args);
```

Invokes a `box<F, MP>` with a specified dispatch type `D`, a signature type `S`, and arguments, through an *indirect* operation. Let `R` be the return type of `S`. `return-type-of<S>` is `R`.

Equivalent to [`invoke`](../proxy_indirect_accessor/friend_invoke.md)`<D, S>(ia, std::forward<Args>(args)...)`, where `ia` is the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<F, MP>` of the underlying [`proxy`](../proxy/README.md)`<F, MP>` with the same cv ref-qualifiers. The behavior is undefined if `b` does not contain a value.

There shall be an operation type `Op` defined in the operation types of `F` or of any super of `F`, reachable via `typename F::super_types` transitively, where

- `Op::is_direct` is `false`, and
- `typename Op::dispatch_type` is `D`, and
- [`substituted-signature`](../ProxySignature.md)`<typename Op::signature_type, F, MP>` is `S`.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F, MP>` is an associated class of the arguments.

To invoke a *direct* operation, call [`release`](release.md) and use [`invoke`](../proxy/friend_invoke.md) on the resulting `proxy<F, MP>`.

## Notes

Unlike [`invoke` (`proxy<F, MP>`)](../proxy/friend_invoke.md), which invokes a *direct* operation, `invoke` on a `box` invokes an *indirect* one. A `box` owns a value rather than a pointer, so its own operations are the indirect ones.

It is generally not recommended to call `invoke` directly. Using an [`accessor`](../ProxyAccess.md) is usually a better option with easier and more descriptive syntax.

## Example

```cpp
#include <iostream>
#include <string>
#include <utility>

#include <proxy/box.h>

PRO_DEF_MEM_DISPATCH(MemLength, length);

struct Sized : pro::facade_builder                              //
               ::add_convention<MemLength, std::size_t() const> //
               ::build {};

int main() {
  pro::box<Sized> b{std::in_place_type<std::string>, "hello"};
  std::cout << b.length() << "\n"; // Invokes with accessor, prints "5"
  std::cout << invoke<MemLength, std::size_t() const>(b)
            << "\n"; // Invokes with the non-member invoke, also prints "5"
}
```

## See Also

- [function template `reflect` (`box<F, MP>`)](friend_reflect.md)
- [function template `invoke` (`proxy_indirect_accessor<F, MP>`)](../proxy_indirect_accessor/friend_invoke.md)
