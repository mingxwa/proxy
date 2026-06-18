# `box::emplace`<br />`box::emplace_alloc`

```cpp
// (1)
template <class T, class... Args>
T& emplace(Args&&... args)
    requires(std::is_constructible_v<T, Args...> &&
        std::is_destructible_v<proxy<F>>);

// (2)
template <class T, class U, class... Args>
T& emplace(std::initializer_list<U> il, Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
        std::is_destructible_v<proxy<F>>);

// (3)
template <class T, class Alloc, class... Args>
T& emplace_alloc(const Alloc& alloc, Args&&... args)
    requires(std::is_constructible_v<T, Args...> &&
        std::is_destructible_v<proxy<F>>);

// (4)
template <class T, class Alloc, class U, class... Args>
T& emplace_alloc(const Alloc& alloc, std::initializer_list<U> il,
        Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
        std::is_destructible_v<proxy<F>>);
```

The `emplace` and `emplace_alloc` function templates change the contained value to an object of type `T` constructed from the arguments.

First, the current contained value (if any) is destroyed as if by calling [`reset()`](reset.md). Then:

- `(1)` Sets the contained value to an object of type `T`, owned as if by [`make_proxy<F, T>(std::forward<Args>(args)...)`](../make_proxy.md).
- `(2)` Sets the contained value to an object of type `T`, owned as if by [`make_proxy<F, T>(il, std::forward<Args>(args)...)`](../make_proxy.md).
- `(3)` Sets the contained value to an object of type `T`, owned with the allocator `alloc` as if by [`allocate_proxy<F, T>(alloc, std::forward<Args>(args)...)`](../allocate_proxy.md).
- `(4)` Sets the contained value to an object of type `T`, owned with the allocator `alloc` as if by [`allocate_proxy<F, T>(alloc, il, std::forward<Args>(args)...)`](../allocate_proxy.md).

For `(1-4)`, if [`proxiable_target<T, F>`](../proxiable_target.md) is `false`, the program is ill-formed and diagnostic messages are generated.

## Return Value

A reference to the newly created contained value.

## Exceptions

Throws any exception thrown by allocation or `T`'s constructor. If an exception is thrown, `*this` does not contain a value.

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
  pro::box<Stringable> b;
  b.emplace<int>(123);
  std::cout << ToString(b) << "\n"; // Prints "123"

  // emplace_alloc uses the given allocator for any required allocation.
  b.emplace_alloc<double>(std::allocator<void>{}, 3.14);
  std::cout << ToString(b) << "\n"; // Prints "3.140000"
}
```

## See Also

- [(constructor)](constructor.md)
- [`reset`](reset.md)
