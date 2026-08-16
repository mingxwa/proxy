# `box::operator proxy_indirect_accessor&`

```cpp
operator proxy_indirect_accessor<F>&() & noexcept;
operator const proxy_indirect_accessor<F>&() const& noexcept;
operator proxy_indirect_accessor<F>&&() && noexcept;
operator const proxy_indirect_accessor<F>&&() const&& noexcept;
```

Converts a `box<F>` to a reference to the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<F>` of the underlying [`proxy`](../proxy/README.md)`<F>`, with matching cv ref-qualifiers. Equivalent to `*p`, where `p` is the underlying `proxy<F>` object of `*this` with the same qualifiers.

The behavior is undefined if `*this` does not contain a value.

## Notes

The accessors of the indirect conventions and reflections of `F` are inherited by `box<F>` itself, so this conversion is not needed to call them. It exists so that a `box<F>` can be passed to code written against `proxy_indirect_accessor<F>` — for example, a [facade-aware overload](../facade_aware_overload_t.md) whose parameter type is `const proxy_indirect_accessor<F>&`.

These conversion functions do not check whether the `box` contains a value. To check, call [`has_value()`](operator_bool.md) or compare with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt).

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/box.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                                 //
                    ::add_convention<FreeToString, std::string() const> //
                    ::build {};

void Print(const pro::proxy_indirect_accessor<Stringable>& ia) {
  std::cout << ToString(ia) << "\n";
}

int main() {
  pro::box<Stringable> b = 123;
  Print(b);                         // Implicit conversion; prints "123"
  std::cout << ToString(b) << "\n"; // Inherited accessor; also prints "123"
}
```

## See Also

- [class template `proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)
- [`release`](release.md)
