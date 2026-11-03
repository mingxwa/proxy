# Class template `member_dispatch`

> Header: `proxy.h`<br />Module: `proxy`<br />Namespace: `pro::inline v5`<br />Since: 5.0.0

```cpp
template <string-literal Name>
struct member_dispatch;
```

The exposition-only type *string-literal* is the same as for [`operator_dispatch`](operator_dispatch/README.md). The primary template is undefined. [`PRO_ENABLE_MEMBER(name)`](PRO_ENABLE_MEMBER.md) defines the specialization `member_dispatch<"name">`.

The specialization is an empty, trivially copyable, nothrow default constructible type. It provides the following call operator, where `name` denotes the identifier passed to the macro:

```cpp
template <class T, class... Args>
decltype(auto) operator()(T&& self, Args&&... args) const
    noexcept(noexcept(std::forward<T>(self).name(std::forward<Args>(args)...)))
    requires(requires { std::forward<T>(self).name(std::forward<Args>(args)...); }) {
  return std::forward<T>(self).name(std::forward<Args>(args)...);
}
```

An implementation may use a static call operator when supported by the language. The call preserves the value categories of its arguments and the type and value category of its result. An invalid call expression removes the overload from overload resolution.

The dispatch does not provide an `accessor` member template. No named access is selected implicitly for it. A convention independently specifies its `access_type`, which may be [`member_access<Name>`](member_access/README.md), another access type, or `void`. Using the legacy builder with this dispatch alone provides no named accessibility.
