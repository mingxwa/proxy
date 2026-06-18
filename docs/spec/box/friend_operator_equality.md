# Function `operator==` (`box<F>`)

```cpp
friend bool operator==(const box& lhs, std::nullopt_t) noexcept;
```

Checks whether `lhs` contains a value by comparing it with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt). A `box` that does not contain a value compares equal to `std::nullopt`; otherwise, it compares non-equal.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F>` is an associated class of the arguments.

The `!=` operator is [synthesized](https://en.cppreference.com/w/cpp/language/default_comparisons) from `operator==`.

## Return Value

`!lhs.has_value()`.

## Example

```cpp
#include <iostream>
#include <optional>

#include <proxy/proxy.h>

struct AnyMovable : pro::facade_builder::build {};

int main() {
  pro::box<AnyMovable> b;
  std::cout << std::boolalpha << (b == std::nullopt) << "\n"; // Prints "true"
  std::cout << (b != std::nullopt) << "\n";                   // Prints "false"
  b = 123;
  std::cout << (b == std::nullopt) << "\n"; // Prints "false"
  std::cout << (b != std::nullopt) << "\n"; // Prints "true"
}
```

## See Also

- [`has_value`](operator_bool.md)
