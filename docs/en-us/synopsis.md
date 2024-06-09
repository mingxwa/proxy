# Proxy 3 Specifications

If you have any questions or feedback, please feel free to file an issue following [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

## Concepts

| Name                                                      | Description                                                  |
| --------------------------------------------------------- | ------------------------------------------------------------ |
| [`facade`](facade.md)                                     | Specifies that a type models a "facade" for runtime polymorphism |
| [`proxiable`](proxiable.md)                               | Specifies that a pointer type can instantiate a `proxy`      |
| [`inplace_proxiable_target`](inplace_proxiable_target.md) | Specifies that a value type can instantiate a `proxy` with direct storage |

## Classes

| Name                                  | Description                                                |
| ------------------------------------- | ---------------------------------------------------------- |
| [`facade_builder`](facade_builder.md) | Provides capability to build a facade type at compile-time |
| [`proxy`](proxy.md)                   | Wraps a pointer object matches specified facade            |

## Functions

| Name                                          | Description                                                  |
| --------------------------------------------- | ------------------------------------------------------------ |
| [`proxy_invoke`](proxy_invoke.md)             | Invokes a `proxy` with a specified dispatch                  |
| [`proxy_reflect`](proxy_reflect.md)           | Acquires compile-time reflection information of the underlying pointer type |
| [`make_proxy`](make_proxy.md)                 | Creates a `proxy` object potentially with heap allocation    |
| [`make_proxy_inplace`](make_proxy_inplace.md) | Create a `proxy` object with strong no-allocation guarantee  |
| [`allocate_proxy`](allocate_proxy.md)         | Creates a `proxy` object with an allocator                   |

## Macros

| Name                                                         | Description                                   |
| ------------------------------------------------------------ | --------------------------------------------- |
| [`PRO_DEF_MEM_DISPATCH`](PRO_DEF_MEM_DISPATCH.md)            | Defines a dispatch type to a member function  |
| [`PRO_DEF_FREE_DISPATCH`](PRO_DEF_FREE_DISPATCH.md)          | Defines a dispatch type to a free function    |
| [`PRO_DEF_OPERATOR_DISPATCH`](PRO_DEF_OPERATOR_DISPATCH.md)  | Defines a dispatch type to an operator        |
| [`PRO_DEF_PREFIX_OPERATOR_DISPATCH`](PRO_DEF_PREFIX_OPERATOR_DISPATCH.md) | Defines a dispatch type to a prefix operator  |
| [`PRO_DEF_POSTFIX_OPERATOR_DISPATCH`](PRO_DEF_POSTFIX_OPERATOR_DISPATCH.md) | Defines a dispatch type to a postfix operator |
| [`PRO_DEF_CONVERSION_DISPATCH`](PRO_DEF_CONVERSION_DISPATCH.md) | Defines a dispatch type to a type conversion  |

## Named Requirements

| Name                              | Description                                                  |
| --------------------------------- | ------------------------------------------------------------ |
| [`Facade`](req_facade.md)         | Specifies that a type models a "facade" for runtime polymorphism |
| [`Convention`](req_convention.md) | Specifies that a type models a "convention" for runtime polymorphism |
| [`Dispatch`](req_dispatch.md)     | Specifies that a type models a "dispatch" for runtime polymorphism |
| [`Reflection`](req_reflection.md) | Specifies that a type models a "reflection" for runtime polymorphism |
| [`Accessor`](req_accessor.md)     | Specifies that a type models an "accessor" for `proxy`       |

