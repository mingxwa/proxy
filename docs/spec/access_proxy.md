// TODO: Remove this file

# Function template `access_proxy`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: `pro::inline v4`

```cpp
template <facade F, class A>
proxy<F>& access_proxy(A& a) noexcept;

template <facade F, class A>
const proxy<F>& access_proxy(const A& a) noexcept;

template <facade F, class A>
proxy<F>&& access_proxy(A&& a) noexcept;

template <facade F, class A>
const proxy<F>&& access_proxy(const A&& a) noexcept;
```

Accesses a `proxy` object from an [accessor](ProAccessible.md) instantiated from the `proxy`. As per `facade<F>`, `typename F::convention_types` shall be a [tuple-like](https://en.cppreference.com/w/cpp/utility/tuple/tuple-like) type containing distinct types `Cs`. There shall be a type `C` in `Cs` where `A` is the same type as `typename C::template accessor<F>`. The behavior is undefined if `a` is not instantiated from a `proxy`.

## Return Value

A reference to the `proxy` that has instantiated `a`.

## Notes

Similar to [`proxy_invoke`](proxy_invoke.md), this function can be used to implement the accessibility of `proxy`. If the facade type `F` is defined with the recommended facilities, it has full accessibility support. Specifically, when:

- the underlying dispatch type `typename C::dispatch_type` is defined via [macro `PRO_DEF_MEM_DISPATCH`](PRO_DEF_MEM_DISPATCH.md), [macro `PRO_DEF_FREE_DISPATCH`](PRO_DEF_FREE_DISPATCH.md), or is a specialization of either [`operator_dispatch`](operator_dispatch/README.md) or [`conversion_dispatch`](explicit_conversion_dispatch/README.md), and
- the convention is defined via [`facade_builder`](basic_facade_builder/README.md).

## See Also

- [named requirements *ProAccessible*](ProAccessible.md)
- [function template `proxy_invoke`](proxy_invoke.md)
