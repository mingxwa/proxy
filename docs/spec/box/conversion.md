# `box::operator proxy_indirect_accessor&`

```cpp
operator proxy_indirect_accessor<F, MP>&() & noexcept;
operator const proxy_indirect_accessor<F, MP>&() const& noexcept;
operator proxy_indirect_accessor<F, MP>&&() && noexcept;
operator const proxy_indirect_accessor<F, MP>&&() const&& noexcept;
```

Converts a `box<F, MP>` to a reference to the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<F, MP>` of the underlying [`proxy`](../proxy/README.md)`<F, MP>`, with matching cv ref-qualifiers. Equivalent to `*p`, where `p` is the underlying `proxy<F, MP>` object of `*this` with the same qualifiers.

The behavior is undefined if `*this` does not contain a value.

## Notes

The accessors of the indirect operations and reflections of `F` are inherited by `box<F, MP>` itself, so this conversion is not needed to call them. It exists so that a `box<F, MP>` can be passed to code written against `proxy_indirect_accessor<F, MP>`, for example to an operation whose [dependent signature](../proxy_dependent_signature.md) takes a `const proxy_indirect_accessor<F, MP>&`.

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
