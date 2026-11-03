# Class template `free_dispatch`

> Header: `proxy.h`<br />Module: `proxy`<br />Namespace: `pro::inline v5`<br />Since: 5.0.0

```cpp
template <string-literal Name>
struct free_dispatch;
```

The exposition-only type *string-literal* is the same as for [`operator_dispatch`](operator_dispatch/README.md). The primary template is undefined. [`PRO_ENABLE_FREE(name)`](PRO_ENABLE_FREE.md) defines the specialization `free_dispatch<"name">`.

The specialization is an empty, trivially copyable, nothrow default constructible type. It provides the following call operator, where `name` denotes the identifier passed to the macro:

```cpp
template <class T, class... Args>
decltype(auto) operator()(T&& self, Args&&... args) const
    noexcept(noexcept(name(std::forward<T>(self), std::forward<Args>(args)...)))
    requires(requires { name(std::forward<T>(self), std::forward<Args>(args)...); }) {
  return name(std::forward<T>(self), std::forward<Args>(args)...);
}
```

An implementation may use a static call operator when supported by the language. The call preserves the value categories of its arguments and the type and value category of its result. An invalid call expression removes the overload from overload resolution.

The dispatch does not provide an `accessor` member template. No named access is selected implicitly for it. A convention independently specifies its `access_type`, which may be [`free_access<Name>`](free_access/README.md), another access type, or `void`. Using the legacy builder with this dispatch alone provides no named accessibility.

The call is unqualified. Ordinary unqualified lookup is performed in the context of the macro expansion, with argument-dependent lookup at instantiation. Declare functions needed by ordinary lookup before enabling the name. Functions found by argument-dependent lookup may be declared later.
