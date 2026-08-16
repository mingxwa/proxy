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
    noexcept(std::is_nothrow_convertible_v<const proxy<F2>&, proxy<F>>)
    requires(std::is_convertible_v<const proxy<F2>&, proxy<F>>);

// (4)
template <facade F2>
box(box<F2>&& rhs)
    noexcept(std::is_nothrow_convertible_v<proxy<F2>, proxy<F>>)
    requires(std::is_convertible_v<proxy<F2>, proxy<F>>);

// (5)
template <class T>
box(T&& val) requires(std::is_constructible_v<std::decay_t<T>, T>);

// (6)
template <class T, class... Args>
explicit box(std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>);

// (7)
template <class T, class U, class... Args>
explicit box(std::in_place_type_t<T>, std::initializer_list<U> il,
        Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>);

// (8)
template <class Alloc, class T>
explicit box(std::allocator_arg_t, const Alloc& alloc, T&& val)
    requires(std::is_constructible_v<std::decay_t<T>, T>);

// (9)
template <class Alloc, class T, class... Args>
explicit box(std::allocator_arg_t, const Alloc& alloc,
        std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>);

// (10)
template <class Alloc, class T, class U, class... Args>
explicit box(std::allocator_arg_t, const Alloc& alloc,
        std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>);
```

Creates a new `box`. Let `p` be the underlying [`proxy`](../proxy/README.md)`<F>` object of the constructed `box`.

- `(1)` Default constructor and the constructor taking [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt) construct a `box` that does not contain a value.
- `(2)` Copy constructor and move constructor are defaulted; they copy or move `p` from that of `rhs`. Their availability and triviality therefore follow those of [`proxy<F>`](../proxy/constructor.md) `(2-3)`.
- `(3)` Converting copy constructor initializes `p` from the underlying `proxy<F2>` of `rhs`. If `rhs` does not contain a value, the constructed `box` does not contain a value.
- `(4)` Converting move constructor initializes `p` from the underlying `proxy<F2>` of `rhs`. If `rhs` does not contain a value, the constructed `box` does not contain a value. `rhs` does not contain a value after the conversion.
- `(5)` Initializes `p` as if by [`make_proxy`](../make_proxy.md)`<F>(std::forward<T>(val))`, so that the contained value is an object of type `std::decay_t<T>` direct-non-list-initialized with `std::forward<T>(val)`. Participates in overload resolution only if `std::decay_t<T>` is neither a specialization of `box` nor a specialization of [`std::in_place_type_t`](https://en.cppreference.com/w/cpp/utility/in_place).
- `(6)` Initializes `p` as if by [`make_proxy`](../make_proxy.md)`<F, T>(std::forward<Args>(args)...)`.
- `(7)` Initializes `p` as if by [`make_proxy`](../make_proxy.md)`<F, T>(il, std::forward<Args>(args)...)`.
- `(8)` Initializes `p` as if by [`allocate_proxy`](../allocate_proxy.md)`<F>(alloc, std::forward<T>(val))`. Participates in overload resolution only if `std::decay_t<T>` is not a specialization of [`std::in_place_type_t`](https://en.cppreference.com/w/cpp/utility/in_place).
- `(9)` Initializes `p` as if by [`allocate_proxy`](../allocate_proxy.md)`<F, T>(alloc, std::forward<Args>(args)...)`.
- `(10)` Initializes `p` as if by [`allocate_proxy`](../allocate_proxy.md)`<F, T>(alloc, il, std::forward<Args>(args)...)`.

For `(3-4)`, the conversion between the underlying `proxy` objects is well-formed when `F` is `F2` or a super of `F2`, reachable via `typename F2::super_types` transitively (see [`proxy::proxy`](../proxy/constructor.md) `(4-5)`).

For `(5-10)`, the diagnostics of the corresponding [`make_proxy`](../make_proxy.md) or [`allocate_proxy`](../allocate_proxy.md) call apply; in particular, if the selected pointer type is not [proxiable](../proxiable.md) for `F`, the program is ill-formed and diagnostic messages are generated.

## Exceptions

`(5-10)` throw any exception thrown by allocation or by the constructor of the contained value.

## Notes

`(6-10)` construct the contained value from `T` rather than from `std::decay_t<T>`, which keeps the semantics simpler than those of [`std::any`](https://en.cppreference.com/w/cpp/utility/any/any).

## Example

```cpp
#include <iostream>
#include <memory_resource>
#include <vector>

#include <proxy/box.h>

PRO_DEF_MEM_DISPATCH(MemSize, size);

struct BasicContainer
    : pro::facade_builder                                       //
      ::add_convention<MemSize, std::size_t() const & noexcept> //
      ::support_copy<pro::constraint_level::nontrivial>         //
      ::build {};

int main() {
  pro::box<BasicContainer> b0;
  std::cout << std::boolalpha << b0.has_value() << "\n"; // Prints "false"

  // From a value
  pro::box<BasicContainer> b1 = std::vector<int>{1, 2, 3};
  std::cout << b1.has_value() << ", " << b1.size() << "\n"; // Prints "true, 3"

  // In-place construction
  pro::box<BasicContainer> b2{std::in_place_type<std::vector<int>>, 10u, 0};
  std::cout << b2.size() << "\n"; // Prints "10"

  // In-place construction from an initializer list
  pro::box<BasicContainer> b3{std::in_place_type<std::vector<int>>,
                              {1, 2, 3, 4}};
  std::cout << b3.size() << "\n"; // Prints "4"

  // With a custom allocator
  std::pmr::unsynchronized_pool_resource pool;
  pro::box<BasicContainer> b4{std::allocator_arg,
                              std::pmr::polymorphic_allocator<>{&pool},
                              std::in_place_type<std::vector<int>>, 5u, 0};
  std::cout << b4.size() << "\n"; // Prints "5"

  // Copy construction; the contained std::vector is copied
  pro::box<BasicContainer> b5 = b1;
  std::cout << b5.size() << "\n"; // Prints "3"
}
```

## See Also

- [function template `make_proxy`](../make_proxy.md)
- [function template `allocate_proxy`](../allocate_proxy.md)
