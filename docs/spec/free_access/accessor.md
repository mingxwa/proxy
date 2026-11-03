# Class template `free_access::accessor`

```cpp
template <class Self, class... Ds>
struct accessor {
  accessor() = delete;
};

template <class Self, class D, class R, class... Args>
struct accessor<Self, proxy_operation<D, R(Args...) cv ref noex>> {
  friend R name(Self cv <ref ? ref : &> self, Args... args) noex;
};
```

Here `cv ref noex` ranges over the qualifiers supported by [*ProOverload*](../ProOverload.md). The function is equivalent to `return invoke<D, R(Args...) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), std::forward<Args>(args)...)`. The dispatch `D` is taken from the descriptor and need not be `free_dispatch<Name>`.

For more than one descriptor, if each `accessor<Self, Ds>` is default constructible, the aggregate accessor inherits those individual accessors. The generated functions are hidden friends found by argument-dependent lookup on `Self`.

Otherwise the primary template applies. Unsupported descriptors make the accessor nonconstructible. Accessor eligibility and descriptor deduplication follow *ProAccess* and the specification of `proxy`. Descriptors whose generated functions have conflicting declarations do not form a valid overload set.

## Reflection descriptor

```cpp
template <class Self, class M>
struct accessor<Self, proxy_reflection<M>> {
  friend const M& name(const Self& self) noexcept;
};
```

For a reflection descriptor, the function returns `reflect<M>(self)`. The lifetime of the returned reference is the lifetime specified by [`reflect`](../proxy/friend_reflect.md). Reflection descriptors participate in accessor aggregation in the same way as operation descriptors.
