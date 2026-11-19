# `box_view::operator=`

```cpp
box_view& operator=(const box_view&) = default;
```

Copy assignment operator is defaulted; it assigns the underlying [`proxy_view`](../proxy_view.md)`<F, MP>` object of `rhs` to that of `*this`, so that `*this` refers to the value referred to by `rhs`, if any. Its triviality follows that of `proxy_view<F, MP>`.

## Return Value

`*this`.

## Notes

Every [constructor](constructor.md) of `box_view` other than the copy constructor is implicit. A [`box`](../box/README.md) lvalue, [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt), or a `box_view` of another facade is therefore assigned to a `box_view` by constructing a temporary `box_view` and copy-assigning it.

## Example

```cpp
#include <iostream>
#include <optional>
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
  pro::box_view<Stringable> v = b1;
  std::cout << ToString(v) << "\n"; // Prints "1"
  v = b2;
  std::cout << ToString(v) << "\n"; // Prints "2"
  v = std::nullopt;
  std::cout << std::boolalpha << v.has_value() << "\n"; // Prints "false"
}
```

## See Also

- [(constructor)](constructor.md)
