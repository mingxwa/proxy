# `box::release`

```cpp
proxy<F> release() noexcept(std::is_nothrow_move_constructible_v<proxy<F>>);
```

Releases the ownership of the contained value and returns it as a [`proxy<F>`](../proxy/README.md). After the call, `*this` does not contain a value.

## Return Value

A `proxy<F>` that owns the value previously contained in `*this`, or a `proxy<F>` that does not contain a value if `*this` did not contain a value.

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/proxy.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                                 //
                    ::add_convention<FreeToString, std::string() const> //
                    ::build {};

int main() {
  pro::box<Stringable> b = 123;
  pro::proxy<Stringable> p = b.release();
  std::cout << std::boolalpha << b.has_value() << "\n"; // Prints "false"
  std::cout << ToString(*p) << "\n";                    // Prints "123"
}
```

## See Also

- [class template `proxy`](../proxy/README.md)
