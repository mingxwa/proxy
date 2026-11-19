# `box_view::box_view`

```cpp
// (1)
box_view() noexcept = default;
box_view(std::nullopt_t) noexcept;

// (2)
box_view(const box_view&) = default;

// (3)
template <facade F2>
box_view(box<F2, MP>& b) noexcept
    requires(std::is_convertible_v<proxy<F2, MP>&, proxy_view<F, MP>>);

// (4)
template <facade F2>
box_view(const box_view<F2, MP>& rhs) noexcept
    requires(!std::is_same_v<F, F2> &&
        std::is_convertible_v<const proxy_view<F2, MP>&, proxy_view<F, MP>>);
```

Creates a new `box_view`. Let `p` be the underlying [`proxy_view`](../proxy_view.md)`<F, MP>` object of the constructed `box_view`.

- `(1)` Default constructor and the constructor taking [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt) construct a `box_view` that does not refer to a value.
- `(2)` Copy constructor is defaulted; it copies `p` from that of `rhs`, so the constructed `box_view` refers to the value referred to by `rhs`, if any. Its triviality follows that of `proxy_view<F, MP>`.
- `(3)` Initializes `p` from the underlying [`proxy`](../proxy/README.md)`<F2, MP>` object of `b`, as an lvalue. If `b` contains a value, the constructed `box_view` refers to that value; otherwise, it does not refer to a value. The conversion is available when [`skills::as_view`](../skills_as_view.md) is applied to `F2` or to a super of `F2`, and `F` is `F2` or a super of `F2`, reachable via `typename F2::super_types` transitively.
- `(4)` Initializes `p` from the underlying `proxy_view<F2, MP>` object of `rhs`, so the constructed `box_view` refers to the value referred to by `rhs`, if any. The conversion is available when `F` is a super of `F2`, reachable via `typename F2::super_types` transitively (see [`proxy_view`](../proxy_view.md)).

## Notes

`(3)` takes only a non-`const` lvalue `box`, for the reasons given in the [notes of `box_view`](README.md#notes).

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/box.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                                 //
                    ::add_convention<FreeToString, std::string() const> //
                    ::build {};

struct Number : pro::facade_builder                                       //
                ::add_facade<Stringable>                                  //
                ::add_convention<pro::operator_dispatch<"+=">, void(int)> //
                ::add_skill<pro::skills::as_view>                         //
                ::build {};

int main() {
  pro::box<Number> b = 123;
  pro::box_view<Number> v1 = b;      // (3)
  pro::box_view<Stringable> v2 = b;  // (3), a view of a super
  pro::box_view<Stringable> v3 = v1; // (4)
  pro::box_view<Number> v4 = v1;     // (2)
  v4 += 1;
  std::cout << ToString(v2) << ", " << ToString(v3)
            << "\n"; // Prints "124, 124"

  pro::box<Number> empty;
  pro::box_view<Number> v5 = empty;
  std::cout << std::boolalpha << v5.has_value() << "\n"; // Prints "false"
}
```

## See Also

- [`box::box`](../box/constructor.md)
- [alias template `skills::as_view`](../skills_as_view.md)
