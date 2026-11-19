# `box_view::operator bool`<br />`box_view::has_value`

```cpp
explicit operator bool() const noexcept;
bool has_value() const noexcept;
```

Checks whether `*this` refers to a value.

## Return Value

`true` if `*this` refers to a value, or `false` otherwise.

## Notes

A `box_view` that refers to a value continues to report `true` after the referenced value is destroyed. `has_value` does not detect an invalidated `box_view` (see the [notes of `box_view`](README.md#notes)).

## Example

```cpp
#include <iostream>

#include <proxy/box.h>

struct Viewable : pro::facade_builder               //
                  ::add_skill<pro::skills::as_view> //
                  ::build {};

int main() {
  pro::box<Viewable> b;
  pro::box_view<Viewable> v = b;
  std::cout << std::boolalpha << v.has_value() << "\n"; // Prints "false"
  b = 123;
  v = b;
  std::cout << static_cast<bool>(v) << "\n"; // Prints "true"
}
```

## See Also

- [`operator==`](friend_operator_equality.md)
