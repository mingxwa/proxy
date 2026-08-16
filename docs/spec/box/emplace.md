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

These function templates change the contained value to an object of type `T` constructed from the arguments.

First, the current contained value (if any) is destroyed as if by calling [`reset()`](reset.md). Then:

- `(1)` Sets the contained value to an object of type `T`, direct-non-list-initialized with `std::forward<Args>(args)...`, selecting the storage as [`make_proxy`](../make_proxy.md)`<F, T>(std::forward<Args>(args)...)` does.
- `(2)` Sets the contained value to an object of type `T`, direct-non-list-initialized with `il, std::forward<Args>(args)...`, selecting the storage as [`make_proxy`](../make_proxy.md)`<F, T>(il, std::forward<Args>(args)...)` does.
- `(3)` Sets the contained value to an object of type `T`, direct-non-list-initialized with `std::forward<Args>(args)...`, allocating the storage as [`allocate_proxy`](../allocate_proxy.md)`<F, T>(alloc, std::forward<Args>(args)...)` does.
- `(4)` Sets the contained value to an object of type `T`, direct-non-list-initialized with `il, std::forward<Args>(args)...`, allocating the storage as [`allocate_proxy`](../allocate_proxy.md)`<F, T>(alloc, il, std::forward<Args>(args)...)` does.

For `(1-4)`, if the selected pointer type is not [proxiable](../proxiable.md) for `F`, the program is ill-formed and diagnostic messages are generated.

## Return Value

A reference to the newly created contained value.

## Exceptions

Throws any exception thrown by allocation or by `T`'s constructor. If an exception is thrown, the previously contained value (if any) is destroyed, and `*this` does not contain a value.

## Notes

`Alloc` is deduced from the first function argument of `emplace_alloc`, so only `T` is specified explicitly, as in `b.emplace_alloc<T>(alloc, args...)`.

Unlike [`std::any::emplace`](https://en.cppreference.com/w/cpp/utility/any/emplace), the contained value is of type `T` rather than `std::decay_t<T>`.

## Example

```cpp
#include <iostream>
#include <memory_resource>
#include <string>

#include <proxy/box.h>

PRO_DEF_MEM_DISPATCH(MemSize, size);

struct BasicContainer
    : pro::facade_builder                                       //
      ::add_convention<MemSize, std::size_t() const & noexcept> //
      ::build {};

int main() {
  // The memory resource shall outlive every box that allocates from it
  std::pmr::unsynchronized_pool_resource pool;
  pro::box<BasicContainer> b;

  // Constructs a std::string in-place
  std::string& s = b.emplace<std::string>(5u, 'x');
  std::cout << s << ", " << b.size() << "\n"; // Prints "xxxxx, 5"

  // Replaces the contained value, taking the storage from a custom allocator
  b.emplace_alloc<std::string>(std::pmr::polymorphic_allocator<>{&pool},
                               {'a', 'b', 'c'});
  std::cout << b.size() << "\n"; // Prints "3"
}
```

## See Also

- [(constructor)](constructor.md)
- [`reset`](reset.md)
