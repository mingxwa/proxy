# Proxy 3 Specifications

Proxy is a C++20 template library for modern runtime polymorphism based on pointer-semantics. It makes runtime abstraction easier in C++: not only saves engineering effort in managing lifetime of different types of objects (like other languages with GC, e.g., Java, C#), but also supports flexible architecture design without requiring inheritance (like Go or Rust, and even better). Most importantly, it generates high quality code with equal or higher performance than an equivalent implementation with virtual functions or existing polymorphic wrappers (including `std::function`, `std::move_only_function`, `std::any`, etc.) in C++ today.

This document is intended to be a quick reference for people to learn and use this library. If you have any questions or feedback, please feel free to file an issue following [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).

## Concepts

| Name                                                      | Description                                                  |
| --------------------------------------------------------- | ------------------------------------------------------------ |
| [`facade`](facade.md)                                     | Specifies that a type models a "facade" of `proxy`           |
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

