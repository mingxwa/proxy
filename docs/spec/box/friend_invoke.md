# Function template `invoke` (`box<F>`)

```cpp
template <class D, class O, class... Args>
return-type-of<O> invoke(box<F>& v, Args&&... args);
template <class D, class O, class... Args>
return-type-of<O> invoke(const box<F>& v, Args&&... args);
template <class D, class O, class... Args>
return-type-of<O> invoke(box<F>&& v, Args&&... args);
template <class D, class O, class... Args>
return-type-of<O> invoke(const box<F>&& v, Args&&... args);
```

Invokes a `box<F>` with a specified dispatch type `D`, an overload type `O`, and arguments, through an *indirect* convention. Let `R` be the return type of `O`. `return-type-of<O>` is `R`.

Equivalent to [`invoke`](../proxy_indirect_accessor/friend_invoke.md)`<D, O>(ia, std::forward<Args>(args)...)`, where `ia` is the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<F>` of the underlying [`proxy`](../proxy/README.md)`<F>` with the same cv ref-qualifiers. The behavior is undefined if `v` does not contain a value.

There shall be a convention type `Conv` defined in the convention types of `F` or of any super of `F`, reachable via `typename F::super_types` transitively, where

- `Conv::is_direct` is `false`, and
- `typename Conv::dispatch_type` is `D`, and
- [`substituted-overload`](../ProOverload.md)`<typename Conv::overload_type, F>` is `O`.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F>` is an associated class of the arguments.

To invoke a *direct* convention, call [`release`](release.md) and use [`invoke`](../proxy/friend_invoke.md) on the resulting `proxy<F>`.

## Notes

Unlike [`invoke` (`proxy<F>`)](../proxy/friend_invoke.md), which invokes a *direct* convention, `invoke` on a `box` invokes an *indirect* one. A `box` owns a value rather than a pointer, so its own conventions are the indirect ones.

It is generally not recommended to call `invoke` directly. Using an [`accessor`](../ProAccessible.md) is usually a better option with easier and more descriptive syntax.

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

- [function template `reflect` (`box<F>`)](friend_reflect.md)
- [function template `invoke` (`proxy_indirect_accessor<F>`)](../proxy_indirect_accessor/friend_invoke.md)
