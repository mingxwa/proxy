# `box<F>::~box`

```cpp
~box() = default;
```

Destroys the `box` object. If the `box` contains a value, the contained value is also destroyed. The destructor is trivial when the destructor of the underlying `proxy<F>` is trivial.

## See Also

- [(constructor)](constructor.md)
- [`reset`](reset.md)
