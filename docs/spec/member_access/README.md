# Class template `member_access`

> Header: `proxy.h`<br />Module: `proxy`<br />Namespace: `pro::inline v5`<br />Since: 5.0.0

```cpp
template <string-literal Name>
struct member_access;
```

The exposition-only type *string-literal* is the same as for [`operator_dispatch`](../operator_dispatch/README.md). The primary template is undefined. [`PRO_ENABLE_MEMBER(name)`](../PRO_ENABLE_MEMBER.md) defines the specialization `member_access<"name">`, an access type meeting the [*ProAccess* requirements](../ProAccess.md) for operation and reflection descriptors. A convention or reflection selects it independently of its dispatch or reflector type.

## Member Types

| Name | Description |
| ---- | ----------- |
| [`accessor`](accessor.md) | Provides named access to operations and reflections |
