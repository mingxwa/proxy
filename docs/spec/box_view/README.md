# Class template `box_view`

> Header: `box.h`  
> Module: `proxy`  
> Namespace: `pro::inline v5`  
> Since: 5.0.0

```cpp
template <facade F, class MP = compact_metadata>
class box_view;  // freestanding-deleted
```

Class template `box_view` is a non-owning view of the value contained by a [`box`](../box/README.md). `box_view` is to `box` what [`proxy_view`](../proxy_view.md) is to [`proxy`](../proxy/README.md): a `box_view<F, MP>` wraps a `proxy_view<F, MP>` and never owns the lifetime of the value it refers to.

Any instance of `box_view<F, MP>` at any given point in time either *refers to a value* or *does not refer to a value*. A `box_view` that refers to a value is obtained from a `box<F2, MP>` lvalue when the underlying `proxy<F2, MP>` converts to `proxy_view<F, MP>`, for example when `F2` or one of its supers is built with [`skills::as_view`](../skills_as_view.md) and `F` is `F2` or a super of `F2` (see [(constructor)](constructor.md)).

Let `VF` be [`view_facade<F>`](../proxy_view.md). Let `Os`, `Rs`, the *descriptors* and the *access types* be defined as for [`proxy_indirect_accessor<VF, MP>`](../proxy_indirect_accessor/README.md). For each distinct access type `A` of the types `O` in `Os` where `O::is_direct` is `false` and of the types `R` in `Rs` where `R::is_direct` is `false`, let `Ds...` be the descriptors of those operations and reflections whose access type is `A`, with duplicates removed and in an unspecified order. If `A` meets the [*ProxyAccess* requirements](../ProxyAccess.md) of `box_view<F, MP>` and `Ds...`, `typename A::template accessor<box_view<F, MP>, Ds...>` is inherited by `box_view<F, MP>`.

These are the same accessors that `proxy_indirect_accessor<VF, MP>` provides for a `proxy_view<F, MP>`, hosted on `box_view<F, MP>` instead. They are the accessors of the indirect operations and reflections of `F` and of its supers, so a `box_view` offers the same accessors as the `box` it is obtained from, except the accessors of operations whose signatures depend on the facade, which are substituted with `VF`.

## Member Types

| Name                   | Description |
| ---------------------- | ----------- |
| `facade_type`          | `F`         |
| `metadata_policy_type` | `MP`        |

## Member Functions

| Name                                                 | Description                                            |
| ---------------------------------------------------- | ------------------------------------------------------ |
| [(constructor)](constructor.md)                      | constructs a `box_view` object                         |
| [(destructor)](destructor.md)                        | destroys a `box_view` object                           |
| [`operator bool`<br />`has_value`](operator_bool.md) | checks if the `box_view` refers to a value             |
| [`operator proxy_indirect_accessor&`](conversion.md) | converts to the indirect accessor of the wrapped view  |
| [`operator=`](assignment.md)                         | assigns a `box_view` object                            |
| [`reset`](reset.md)                                  | stops referring to any value                           |
| [`swap`](swap.md)                                    | exchanges the contents                                 |

## Non-Member Functions

| Name                                        | Description                                                  |
| ------------------------------------------- | ------------------------------------------------------------ |
| [`operator==`](friend_operator_equality.md) | compares a `box_view` with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt) |
| [`invoke`](friend_invoke.md)                | invokes a `box_view` with a specified operation              |
| [`reflect`](friend_reflect.md)              | acquires reflection information of the referenced type       |

## Notes

A `box_view` obtained from a `box` is invalidated by any operation that destroys or relocates the value contained by that `box`, including destroying, resetting, assigning to, emplacing into, releasing, swapping, or moving from the `box`, because the value may be stored within the `box` object footprint. The behavior is undefined if an invalidated `box_view` that refers to a value is used to invoke an operation or acquire a reflection.

A `box_view` is not obtained from a `const` or an rvalue `box`, just as [`skills::as_view`](../skills_as_view.md) converts only a non-`const` lvalue `proxy`. A view of an rvalue would refer to a value whose lifetime is about to end.

A `box` is not constructed from a `box_view`. A `box_view` does not own the value it refers to, so it cannot transfer that ownership.

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/box.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Number : pro::facade_builder                                       //
                ::add_convention<pro::operator_dispatch<"+=">, void(int)> //
                ::add_convention<FreeToString, std::string() const>       //
                ::add_skill<pro::skills::as_view>                         //
                ::build {};

void Increase(pro::box_view<Number> v) { v += 1; }

int main() {
  pro::box<Number> b = 41;
  Increase(b);                      // The view refers to the value of b
  std::cout << ToString(b) << "\n"; // Prints "42"
}
```

## See Also

- [class template `box`](../box/README.md)
- [alias template `proxy_view`](../proxy_view.md)
- [alias template `skills::as_view`](../skills_as_view.md)
