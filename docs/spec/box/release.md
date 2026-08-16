# `box::release`

```cpp
proxy<F> release()
    noexcept(std::is_nothrow_move_constructible_v<proxy<F>> &&
        std::is_nothrow_destructible_v<proxy<F>>)
    requires(std::is_move_constructible_v<proxy<F>> &&
        std::is_destructible_v<proxy<F>>);
```

Releases the ownership of the contained value to a [`proxy`](../proxy/README.md)`<F>` object, constructed from the underlying `proxy<F>` object of `*this`.

After the call, `*this` does not contain a value.

## Return Value

A `proxy<F>` object that contains the value contained by `*this` before the call, if any.

## Notes

The *direct* accessors of `F` describe the contained pointer, and are therefore provided by `proxy<F>` rather than by `box<F>`. `release` is the way to reach them from a `box`.

When `F::copyability` is `constraint_level::trivial`, `proxy<F>` has no move constructor, and the ownership is transferred by its trivial copy constructor. `*this` is then emptied explicitly, so that the postcondition holds for every facade type.

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/box.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                           //
                    ::add_convention<FreeToString, std::string()> //
                    ::build {};

int main() {
  pro::box<Stringable> b = 123;
  std::cout << std::boolalpha << b.has_value() << "\n"; // Prints "true"

  pro::proxy<Stringable> p = b.release();
  std::cout << b.has_value() << "\n"; // Prints "false"
  std::cout << ToString(*p) << "\n";  // Prints "123"
}
```

## See Also

- [class template `proxy`](../proxy/README.md)
- [`reset`](reset.md)
