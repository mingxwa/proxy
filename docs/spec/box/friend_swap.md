# Function `swap` (`box<F>`)

```cpp
friend void swap(box& lhs, box& rhs)
    noexcept(std::is_nothrow_swappable_v<proxy<F>>)
    requires(std::is_swappable_v<proxy<F>>);
```

Overloads the [std::swap](https://en.cppreference.com/w/cpp/algorithm/swap) algorithm for `box`. Exchanges the state of `lhs` with that of `rhs`. Effectively calls `lhs.swap(rhs)`.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F>` is an associated class of the arguments.

## Example

```cpp
#include <iostream>
#include <string>
#include <utility>

#include <proxy/box.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                           //
                    ::add_convention<FreeToString, std::string()> //
                    ::build {};

int main() {
  pro::box<Stringable> b0 = 123;
  pro::box<Stringable> b1 = 456;
  std::ranges::swap(b0, b1);         // Finds the hidden friend
  std::cout << ToString(b0) << "\n"; // Prints "456"
  std::cout << ToString(b1) << "\n"; // Prints "123"
}
```

## See Also

- [`swap`](swap.md)
