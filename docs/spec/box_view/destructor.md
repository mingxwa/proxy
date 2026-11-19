# `box_view::~box_view`

```cpp
~box_view() = default;
```

Destroys the `box_view` object. The value referred to by the `box_view`, if any, is not affected.

The destructor is defaulted; it destroys the underlying [`proxy_view`](../proxy_view.md)`<F, MP>` object, and its triviality follows that of `proxy_view<F, MP>`.

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
  pro::box<Stringable> b = 123;
  {
    pro::box_view<Stringable> v = b;
  } // Destroying v does not destroy the value contained by b
  std::cout << ToString(b) << "\n"; // Prints "123"
}
```

## See Also

- [`reset`](reset.md)
