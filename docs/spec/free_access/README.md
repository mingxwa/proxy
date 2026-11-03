# Class template `free_access`

> Header: `proxy.h`<br />Module: `proxy`<br />Namespace: `pro::inline v5`<br />Since: 5.0.0

```cpp
template <string-literal Name>
struct free_access;
```

The exposition-only type *string-literal* is the same as for [`operator_dispatch`](../operator_dispatch/README.md). The primary template is undefined. [`PRO_ENABLE_FREE(name)`](../PRO_ENABLE_FREE.md) defines the specialization `free_access<"name">`, an access type meeting the [*ProAccess* requirements](../ProAccess.md) for operation and reflection descriptors. A convention or reflection selects it independently of its dispatch or reflector type.

## Member Types

| Name | Description |
| ---- | ----------- |
| [`accessor`](accessor.md) | Provides named access to operations and reflections |
