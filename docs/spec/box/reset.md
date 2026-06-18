# `box::reset`

```cpp
void reset()
    noexcept(std::is_nothrow_destructible_v<proxy<F>>)
    requires(std::is_destructible_v<proxy<F>>);
```

Destroys the contained value if it exists. After the call, `*this` does not contain a value.

## See Also

- [`operator=`](assignment.md)
