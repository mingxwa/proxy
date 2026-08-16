# `box::swap`

```cpp
void swap(box& rhs) noexcept(std::is_nothrow_swappable_v<proxy<F>>)
    requires(std::is_swappable_v<proxy<F>>);
```

Exchanges the contained values of `*this` and `rhs`. Equivalent to [swapping](../proxy/swap.md) the underlying [`proxy`](../proxy/README.md)`<F>` objects, so no contained value is relocated when `F::relocatability` or `F::copyability` is `constraint_level::trivial`.

## See Also

- [`swap` (non-member)](friend_swap.md)
