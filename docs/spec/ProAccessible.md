# Named requirements: *ProAccessible*

A *descriptor* is a specialization of either [`proxy_operation`](proxy_operation.md) or [`proxy_reflection`](proxy_reflection.md). A type `A` meets the *ProAccessible* requirements of a type `Self` and descriptor types `Ds...`, where `Self` is a specialization of either [`proxy`](proxy/README.md) or [`proxy_indirect_accessor`](proxy_indirect_accessor/README.md), if the following expressions are well-formed and have the specified semantics.

| Expressions                                  | Semantics                                                    |
| -------------------------------------------- | ------------------------------------------------------------ |
| `typename A::template accessor<Self, Ds...>` | A type that provides accessibility to `Self` for the operations and reflections described by `Ds...`. It shall be a *nothrow-default-constructible*, *trivially-copyable* type, and shall not be [final](https://en.cppreference.com/w/cpp/language/final). |

*Since 5.0.0*: an accessor is provided by an access type for any number of operations and reflections, each described by a descriptor. Previously, accessibility was provided by the dispatch type `D` of a convention as `typename D::template accessor<P, D, Os...>`, and by the reflector type `R` of a reflection as `typename R::template accessor<P, R>`.

## Notes

The *access type* of a convention or a reflection is its member type `access_type`, or `void` if it has none (see [*ProBasicConvention*](ProBasicConvention.md) and [*ProBasicReflection*](ProBasicReflection.md)). For each access type `A`, `proxy` inherits a single accessor `typename A::template accessor<Self, Ds...>` formed from the descriptors of all the direct conventions and reflections sharing `A`, and `proxy_indirect_accessor` inherits one formed from the descriptors of all the indirect conventions and reflections sharing `A`, provided that `A` meets the *ProAccessible* requirements of `Self` and `Ds...` (see [`proxy`](proxy/README.md) and [`proxy_indirect_accessor`](proxy_indirect_accessor/README.md)). Therefore, a member function of the accessor can be called on `Self`, and a friend function defined in the accessor can be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `Self` is an associated class of the arguments.

In the accessor, `*this` can be converted to a reference to `Self`, through which an operation described by `proxy_operation<D, O>` is performed via [`invoke<D, O>`](proxy/friend_invoke.md), and a reflection described by `proxy_reflection<M>` is acquired via [`reflect<M>`](proxy/friend_reflect.md). When `Ds...` contains more than one descriptor, an access type usually inherits the accessors of the individual descriptors and, for member functions, makes them visible together with `using`-declarations, so that overload resolution is performed among all of them. [`operator_access`](operator_access/README.md), [`explicit_conversion_access`](explicit_conversion_access/README.md), [`implicit_conversion_access`](implicit_conversion_access/README.md), and the access types defined by [`PRO_DEF_MEM_DISPATCH`](PRO_DEF_MEM_DISPATCH.md), [`PRO_DEF_FREE_DISPATCH`](PRO_DEF_FREE_DISPATCH.md) and [`PRO_DEF_FREE_AS_MEM_DISPATCH`](PRO_DEF_FREE_AS_MEM_DISPATCH.md) follow this pattern.

## See Also

- [class template `proxy`](proxy/README.md)
- [class template `proxy_indirect_accessor`](proxy_indirect_accessor/README.md)
- [class template `proxy_operation`](proxy_operation.md)
- [class template `proxy_reflection`](proxy_reflection.md)
