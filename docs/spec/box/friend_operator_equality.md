# Function `operator==` (`box<F>`)

```cpp
friend bool operator==(const box& lhs, std::nullopt_t) noexcept;
```

Checks whether `lhs` contains a value by comparing it with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt). A `box` that does not contain a value compares equal to `std::nullopt`; otherwise, it compares non-equal.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F>` is an associated class of the arguments.

The `!=` operator is [synthesized](https://en.cppreference.com/w/cpp/language/default_comparisons) from `operator==`.

## Return Value

`!lhs.has_value()`.

## Notes

A `box` is compared with `std::nullopt` rather than with `nullptr`, because it holds a value rather than a pointer. This mirrors [`std::optional`](https://en.cppreference.com/w/cpp/utility/optional/operator_cmp) and differs from [`proxy`](../proxy/friend_operator_equality.md).

## Example

```cpp
#include <iostream>
#include <optional>

#include <proxy/box.h>

struct Any : pro::facade_builder::build {};

int main() {
  pro::box<Any> b;
  std::cout << std::boolalpha << (b == std::nullopt) << "\n"; // Prints "true"
  std::cout << (b != std::nullopt) << "\n";                   // Prints "false"
  b = 123;
  std::cout << (b == std::nullopt) << "\n"; // Prints "false"
  std::cout << (b != std::nullopt) << "\n"; // Prints "true"
}
```

## See Also

- [`has_value`](operator_bool.md)
