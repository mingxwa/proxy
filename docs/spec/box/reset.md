# `box::reset`

```cpp
void reset() noexcept(std::is_nothrow_destructible_v<proxy<F>>)
    requires(std::is_destructible_v<proxy<F>>);
```

Destroys the contained value if it exists, and deallocates any storage allocated for it. After the call, `*this` does not contain a value.

## Example

```cpp
#include <iostream>

#include <proxy/box.h>

struct Any : pro::facade_builder::build {};

int main() {
  pro::box<Any> b = 123;
  std::cout << std::boolalpha << b.has_value() << "\n"; // Prints "true"
  b.reset();
  std::cout << b.has_value() << "\n"; // Prints "false"
}
```

## See Also

- [`operator=`](assignment.md)
- [`release`](release.md)
