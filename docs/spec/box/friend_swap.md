# Function `swap` (`box<F>`)

```cpp
friend void swap(box& lhs, box& rhs) noexcept(std::is_nothrow_swappable_v<proxy<F>>)
    requires(std::is_swappable_v<proxy<F>>);
```

Overloads the [std::swap](https://en.cppreference.com/w/cpp/algorithm/swap) algorithm for `box`. Exchanges the state of `lhs` with that of `rhs`. Effectively calls `lhs.swap(rhs)`.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F>` is an associated class of the arguments.

## Example

```cpp
#include <iostream>
#include <numbers>
#include <string>

#include <proxy/proxy.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                                 //
                    ::add_convention<FreeToString, std::string() const> //
                    ::support_relocation<pro::constraint_level::nothrow> //
                    ::build {};

int main() {
  pro::box<Stringable> b0 = 123;
  pro::box<Stringable> b1 = std::numbers::pi;
  std::cout << ToString(b0) << "\n"; // Prints "123"
  std::cout << ToString(b1) << "\n"; // Prints "3.14..."
  std::ranges::swap(b0, b1);         // finds the hidden friend
  std::cout << ToString(b0) << "\n"; // Prints "3.14..."
  std::cout << ToString(b1) << "\n"; // Prints "123"
}
```

## See Also

- [`swap`](swap.md)
