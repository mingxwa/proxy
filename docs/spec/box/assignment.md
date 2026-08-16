# `box::operator=`

```cpp
// (1)
box& operator=(std::nullopt_t)
    noexcept(std::is_nothrow_assignable_v<proxy<F>, std::nullptr_t>)
    requires(std::is_assignable_v<proxy<F>, std::nullptr_t>);

// (2)
box& operator=(const box&) = default;
box& operator=(box&&) = default;

// (3)
template <facade F2>
box& operator=(const box<F2>& rhs)
    noexcept(std::is_nothrow_assignable_v<proxy<F>, const proxy<F2>&>)
    requires(std::is_assignable_v<proxy<F>, const proxy<F2>&>);

// (4)
template <facade F2>
box& operator=(box<F2>&& rhs)
    noexcept(std::is_nothrow_assignable_v<proxy<F>, proxy<F2>>)
    requires(std::is_assignable_v<proxy<F>, proxy<F2>>);

// (5)
template <class T>
box& operator=(T&& val)
    requires(std::is_constructible_v<std::decay_t<T>, T> &&
        std::is_assignable_v<proxy<F>, proxy<F>>);
```

Assigns a new value to `box` or destroys the contained value. Let `p` be the underlying [`proxy`](../proxy/README.md)`<F>` object of `*this`.

- `(1)` Destroys the current contained value if it exists. After the call, `*this` does not contain a value.
- `(2)` Copy assignment operator and move assignment operator are defaulted; they assign `p` from that of `rhs`. Their availability and triviality therefore follow those of [`proxy<F>`](../proxy/assignment.md) `(2-3)`.
- `(3)` Converting copy assignment operator assigns the underlying `proxy<F2>` of `rhs` to `p`. If `rhs` does not contain a value, it destroys the contained value of `*this` (if any).
- `(4)` Converting move assignment operator assigns the underlying `proxy<F2>` of `rhs` to `p`. If `rhs` does not contain a value, it destroys the contained value of `*this` (if any). After the assignment, `rhs` does not contain a value.
- `(5)` Equivalent to `p = `[`make_proxy`](../make_proxy.md)`<F>(std::forward<T>(val))`, so that the contained value becomes an object of type `std::decay_t<T>` direct-non-list-initialized with `std::forward<T>(val)`. Participates in overload resolution only if `std::decay_t<T>` is not a specialization of `box`. The diagnostics of the corresponding `make_proxy` call apply.

For `(3-4)`, the assignment between the underlying `proxy` objects is well-formed when `F` is `F2` or a super of `F2`, reachable via `typename F2::super_types` transitively (see [`proxy::operator=`](../proxy/assignment.md) `(4-5)`).

## Return Value

`*this`.

## Exceptions

`(5)` throws any exception thrown by allocation or by the constructor of the contained value. If an exception is thrown, `*this` is unmodified.

## Notes

`(5)` accepts a value rather than a pointer, which is the essential difference from [`proxy::operator=`](../proxy/assignment.md) `(6)`. To replace the contained value without creating a temporary, use [`emplace`](emplace.md).

## Example

```cpp
#include <iostream>
#include <optional>
#include <string>

#include <proxy/box.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                           //
                    ::add_convention<FreeToString, std::string()> //
                    ::build {};

int main() {
  pro::box<Stringable> b = 123;
  std::cout << ToString(b) << "\n"; // Prints "123"

  // Assigning a value replaces the contained object
  b = 3.5;
  std::cout << ToString(b) << "\n"; // Prints "3.500000"

  b = std::nullopt;
  std::cout << std::boolalpha << b.has_value() << "\n"; // Prints "false"
}
```

## See Also

- [(constructor)](constructor.md)
- [`emplace`](emplace.md)
