# Function `operator==` (`box_view<F, MP>`)

```cpp
friend bool operator==(const box_view& lhs, std::nullopt_t) noexcept;
```

Checks whether `lhs` refers to a value by comparing it with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt). A `box_view` that does not refer to a value compares equal to `std::nullopt`; otherwise, it compares non-equal.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box_view<F, MP>` is an associated class of the arguments.

The `!=` operator is [synthesized](https://en.cppreference.com/w/cpp/language/default_comparisons) from `operator==`.

## Return Value

`!lhs.has_value()`.

## Example

```cpp
#include <iostream>
#include <optional>

#include <proxy/box.h>

struct Viewable : pro::facade_builder               //
                  ::add_skill<pro::skills::as_view> //
                  ::build {};

int main() {
  pro::box<Viewable> b;
  pro::box_view<Viewable> v = b;
  std::cout << std::boolalpha << (v == std::nullopt) << "\n"; // Prints "true"
  b = 123;
  v = b;
  std::cout << (v != std::nullopt) << "\n"; // Prints "true"
}
```

## See Also

- [`has_value`](operator_bool.md)
