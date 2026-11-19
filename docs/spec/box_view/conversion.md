# `box_view::operator proxy_indirect_accessor&`

```cpp
operator proxy_indirect_accessor<view_facade<F>, MP>&() & noexcept;
operator const proxy_indirect_accessor<view_facade<F>, MP>&() const& noexcept;
operator proxy_indirect_accessor<view_facade<F>, MP>&&() && noexcept;
operator const proxy_indirect_accessor<view_facade<F>, MP>&&() const&& noexcept;
```

Converts a `box_view<F, MP>` to a reference to the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<view_facade<F>, MP>` of the underlying [`proxy_view`](../proxy_view.md)`<F, MP>`, with matching cv ref-qualifiers. Equivalent to `*p`, where `p` is the underlying `proxy_view<F, MP>` object of `*this` with the same qualifiers.

The behavior is undefined if `*this` does not refer to a value.

## Notes

The accessors of the indirect operations and reflections of `F` are inherited by `box_view<F, MP>` itself, so this conversion is not needed to call them. It exists so that a `box_view<F, MP>` can be passed to code written against `proxy_indirect_accessor<view_facade<F>, MP>`, which is what dereferencing a `proxy_view<F, MP>` yields.

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/box.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                                 //
                    ::add_convention<FreeToString, std::string() const> //
                    ::add_skill<pro::skills::as_view>                   //
                    ::build {};

void Print(
    const pro::proxy_indirect_accessor<pro::view_facade<Stringable>>& ia) {
  std::cout << ToString(ia) << "\n";
}

int main() {
  pro::box<Stringable> b = 123;
  pro::box_view<Stringable> v = b;
  Print(v);                         // Implicit conversion; prints "123"
  std::cout << ToString(v) << "\n"; // Inherited accessor; also prints "123"
}
```

## See Also

- [class template `proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)
