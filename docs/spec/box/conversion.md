# `box::operator proxy_indirect_accessor`

```cpp
// (1)
operator proxy_indirect_accessor<F>&() & noexcept;
operator const proxy_indirect_accessor<F>&() const& noexcept;

// (2)
operator proxy_indirect_accessor<F>&&() && noexcept;
operator const proxy_indirect_accessor<F>&&() const&& noexcept;
```

Converts `*this` to a reference of the [`proxy_indirect_accessor<F>`](../proxy_indirect_accessor.md) that provides accessibility to the indirect conventions and reflections of `F`. The behavior is undefined if `*this` does not contain a value.

- `(1)` Returns an lvalue reference to the indirect accessor, with the same constness as `*this`.
- `(2)` Returns an rvalue reference to the indirect accessor, with the same constness as `*this`.

These conversions allow a `box` to be passed where a `proxy_indirect_accessor<F>` reference is expected, for example, to APIs written against the indirect accessor type. Because the indirect accessors are also inherited by `box` directly, ordinary usage does not require an explicit conversion.

## See Also

- [class template `proxy_indirect_accessor`](../proxy_indirect_accessor.md)
