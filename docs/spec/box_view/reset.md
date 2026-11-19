# `box_view::reset`

```cpp
void reset() noexcept;
```

Makes `*this` stop referring to a value. The value previously referred to, if any, is not affected. After the call, `*this` does not refer to a value.

## Example

```cpp
#include <iostream>

#include <proxy/box.h>

struct Viewable : pro::facade_builder               //
                  ::add_skill<pro::skills::as_view> //
                  ::build {};

int main() {
  pro::box<Viewable> b = 123;
  pro::box_view<Viewable> v = b;
  v.reset();
  std::cout << std::boolalpha << v.has_value() << "\n"; // Prints "false"
  std::cout << b.has_value() << "\n";                   // Prints "true"
}
```

## See Also

- [`operator=`](assignment.md)
