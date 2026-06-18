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
template <facade F2>
box& operator=(box<F2>&& rhs)
    noexcept(std::is_nothrow_assignable_v<proxy<F>, proxy<F2>>)
    requires(std::is_assignable_v<proxy<F>, proxy<F2>>);

// (4)
template <class T>
box& operator=(T&& val)
    requires(std::is_constructible_v<std::decay_t<T>, T> &&
        std::is_destructible_v<proxy<F>>);
```

Assigns a new value to `box` or destroys the contained value.

- `(1)` Destroys the current contained value if it exists. After the call, `*this` does not contain a value.
- `(2)` Copy assignment operator and move assignment operator are defaulted. The copy assignment operator performs a deep copy; it is defined as deleted when the underlying `proxy<F>` is not copy-assignable. After move assignment, `rhs` does not contain a value.
- `(3)` Assigns from a `box<F2>` of a different facade type, following the assignment semantics of the underlying `proxy`. Participates in overload resolution only if `proxy<F>` is assignable from the corresponding `proxy<F2>`.
- `(4)` Let `VT` be `std::decay_t<T>`. Sets the contained value to an object of type `VT` as if by `*this = box(std::forward<T>(val))`. Participates in overload resolution only if `VT` is not a specialization of `box`.

## Return Value

`*this`.

## See Also

- [(constructor)](constructor.md)
- [`reset`](reset.md)
