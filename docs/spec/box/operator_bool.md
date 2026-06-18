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

#include <proxy/proxy.h>

struct AnyCopyable : pro::facade_builder                               //
                     ::support_copy<pro::constraint_level::nontrivial> //
                     ::build {};

int main() {
  pro::box<AnyCopyable> b;
  std::cout << std::boolalpha << b.has_value() << "\n"; // Prints "false"
  b = 123;
  std::cout << static_cast<bool>(b) << "\n"; // Prints "true"
  b = std::nullopt;
  std::cout << b.has_value() << "\n"; // Prints "false"
}
```

## See Also

- [`operator==`](friend_operator_equality.md)
