# `box_view::swap`

```cpp
void swap(box_view& rhs) noexcept;
```

Exchanges the values referred to by `*this` and `rhs`. Equivalent to [swapping](../proxy/swap.md) the underlying [`proxy_view`](../proxy_view.md)`<F, MP>` objects. The referenced values themselves are not affected.

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

int main() {
  pro::box<Stringable> b1 = 1;
  pro::box<Stringable> b2 = 2;
  pro::box_view<Stringable> v1 = b1;
  pro::box_view<Stringable> v2 = b2;
  v1.swap(v2);
  std::cout << ToString(v1) << ", " << ToString(v2) << "\n"; // Prints "2, 1"
  std::cout << ToString(b1) << ", " << ToString(b2) << "\n"; // Prints "1, 2"
}
```
