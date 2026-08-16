# `box::operator bool`<br />`box::has_value`

```cpp
explicit operator bool() const noexcept;
bool has_value() const noexcept;
```

Checks whether `*this` contains a value.

## Return Value

`true` if `*this` contains a value, or `false` otherwise.

## Example

```cpp
#include <iostream>
#include <optional>

#include <proxy/box.h>

struct Any : pro::facade_builder::build {};

int main() {
  pro::box<Any> b;
  std::cout << std::boolalpha << b.has_value() << "\n"; // Prints "false"
  b = 123;
  std::cout << b.has_value() << "\n"; // Prints "true"
  b = std::nullopt;
  std::cout << static_cast<bool>(b) << "\n"; // Prints "false"
}
```

## See Also

- [`operator==`](friend_operator_equality.md)
