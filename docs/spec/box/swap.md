# `box::swap`

```cpp
void swap(box& rhs)
    noexcept(std::is_nothrow_swappable_v<proxy<F>>)
    requires(std::is_swappable_v<proxy<F>>);
```

Exchanges the contained values of `*this` and `rhs`. Effectively swaps the underlying `proxy` objects.

## See Also

- [`swap` (non-member)](friend_swap.md)
