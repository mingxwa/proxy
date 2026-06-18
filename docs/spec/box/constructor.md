# `box<F>::box`

```cpp
// (1)
box() noexcept = default;
box(std::nullopt_t) noexcept;

// (2)
box(const box&) = default;
box(box&&) = default;

// (3)
template <facade F2>
box(const box<F2>& rhs)
    noexcept(std::is_nothrow_convertible_v<proxy<F>, const proxy<F2>&>)
    requires(std::is_convertible_v<proxy<F>, const proxy<F2>&>);
template <facade F2>
box(box<F2>&& rhs)
    noexcept(std::is_nothrow_convertible_v<proxy<F>, proxy<F2>>)
    requires(std::is_convertible_v<proxy<F>, proxy<F2>>);

// (4)
template <class T>
box(T&& val)
    requires(std::is_constructible_v<std::decay_t<T>, T>);

// (5)
template <class T, class... Args>
explicit box(std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>);

// (6)
template <class T, class U, class... Args>
explicit box(std::in_place_type_t<T>, std::initializer_list<U> il,
        Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>);

// (7)
template <class Alloc, class T>
explicit box(std::allocator_arg_t, const Alloc& alloc, T&& val)
    requires(std::is_constructible_v<std::decay_t<T>, T>);

// (8)
template <class Alloc, class T, class... Args>
explicit box(std::allocator_arg_t, const Alloc& alloc,
        std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>);

// (9)
template <class Alloc, class T, class U, class... Args>
explicit box(std::allocator_arg_t, const Alloc& alloc,
        std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>);
```

Creates a new `box`.

- `(1)` Default constructor and the constructor taking [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt) construct a `box` that does not contain a value.
- `(2)` Copy constructor and move constructor are defaulted. The copy constructor performs a deep copy of the contained value of `rhs` (if any); it is defined as deleted when the underlying `proxy<F>` is not copyable. After move construction, `rhs` does not contain a value.
- `(3)` Converting constructors from a `box<F2>` of a different facade type. Participates in overload resolution only if `proxy<F>` is convertible from the corresponding `proxy<F2>`. The contained value of `rhs` (if any) is transferred to `*this` following the conversion semantics of the underlying `proxy`.
- `(4)` Let `VT` be `std::decay_t<T>`. Constructs a `box` whose contained value is of type `VT`, owned as if by [`make_proxy<F>(std::forward<T>(val))`](../make_proxy.md). Participates in overload resolution only if `VT` is not a specialization of `box` or [`std::in_place_type_t`](https://en.cppreference.com/w/cpp/utility/in_place).
- `(5)` Constructs a `box` whose contained value is of type `T`, owned as if by [`make_proxy<F, T>(std::forward<Args>(args)...)`](../make_proxy.md).
- `(6)` Constructs a `box` whose contained value is of type `T`, owned as if by [`make_proxy<F, T>(il, std::forward<Args>(args)...)`](../make_proxy.md).
- `(7)` Let `VT` be `std::decay_t<T>`. Constructs a `box` whose contained value is of type `VT`, owned as if by [`allocate_proxy<F>(alloc, std::forward<T>(val))`](../allocate_proxy.md). Participates in overload resolution only if `VT` is not a specialization of [`std::in_place_type_t`](https://en.cppreference.com/w/cpp/utility/in_place).
- `(8)` Constructs a `box` whose contained value is of type `T`, owned as if by [`allocate_proxy<F, T>(alloc, std::forward<Args>(args)...)`](../allocate_proxy.md).
- `(9)` Constructs a `box` whose contained value is of type `T`, owned as if by [`allocate_proxy<F, T>(alloc, il, std::forward<Args>(args)...)`](../allocate_proxy.md).

For `(4-9)`, if [`proxiable_target<std::decay_t<T>, F>`](../proxiable_target.md) is `false`, the program is ill-formed and diagnostic messages are generated.

## Exceptions

Throws any exception thrown by allocation or the constructor of the contained value type.

## Example

```cpp
#include <array>
#include <iostream>
#include <string>

#include <proxy/proxy.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                                 //
                    ::add_convention<FreeToString, std::string() const> //
                    ::build {};

int main() {
  pro::box<Stringable> b0; // Does not contain a value
  std::cout << std::boolalpha << b0.has_value() << "\n"; // Prints "false"

  // From a value
  pro::box<Stringable> b1 = 123;
  std::cout << ToString(b1) << "\n"; // Prints "123"

  // In-place construction
  pro::box<Stringable> b2{std::in_place_type<double>, 3.14};
  std::cout << ToString(b2) << "\n"; // Prints "3.140000"

  // With an allocator (the large target is heap-allocated)
  pro::box<Stringable> b3{std::allocator_arg, std::allocator<void>{},
                          std::in_place_type<long long>, 456};
  std::cout << ToString(b3) << "\n"; // Prints "456"
}
```

## See Also

- [function template `make_proxy`](../make_proxy.md)
- [function template `allocate_proxy`](../allocate_proxy.md)
