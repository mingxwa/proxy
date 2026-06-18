# Class template `box`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: `pro::inline v4`

```cpp
template <facade F>
class box;
```

Class template `box` is an owning, value-semantic polymorphic wrapper for C++ objects. While [`proxy`](../proxy/README.md) is based on *pointer semantics* and may refer to an object whose lifetime is managed elsewhere, `box` is based on *value semantics*: it always *owns* the contained object, much like [`std::any`](https://en.cppreference.com/w/cpp/utility/any). Copying a `box` performs a deep copy of the contained object (when the facade is copyable), and destroying a `box` destroys the contained object.

A `box<F>` is implemented in terms of a `proxy<F>` that only ever contains an *owning* pointer (see [`make_proxy`](../make_proxy.md) and [`allocate_proxy`](../allocate_proxy.md) for the owning pointer models). As a result, `box<F>` has the same size and alignment as `proxy<F>`, and never refers to external storage.

Any instance of `box<F>` at any given point in time either *contains a value* or *does not contain a value*.

A facade type `F` is *eligible for boxing* when an owning pointer can satisfy the lifetime-management and direct accessibility requirements of `F`. Effectively, this means that the direct conventions of `F` shall be limited to substitution (e.g., [`skills::as_view`](../skills_as_view.md), [`skills::as_weak`](../skills_as_weak.md)) and that the contained value's copyability, relocatability, and destructibility are compatible with `F`. If `F` is not eligible for boxing, instantiating `box<F>` is ill-formed, and a diagnostic message ("facade type not eligible for boxing") is generated.

As per `facade<F>`, `typename F::convention_types` shall be a [tuple-like](https://en.cppreference.com/w/cpp/utility/tuple/tuple-like) type containing any number of distinct types `Cs`, and `typename F::reflection_types` shall be a [tuple-like](https://en.cppreference.com/w/cpp/utility/tuple/tuple-like) type containing any number of distinct types `Rs`.

- For each type `C` in `Cs`, if `C::is_direct` is `false` and `typename C::dispatch_type` meets the [*ProAccessible* requirements](../ProAccessible.md) of `box<F>, typename C::dispatch_type, substituted-overload-types...`, `typename C::dispatch_type::template accessor<box<F>, typename C::dispatch_type, substituted-overload-types...>` is inherited by `box<F>`. Let `Os...` be the element types of `typename C::overload_types`, `substituted-overload-types...` is [`substituted-overload<Os, F>...`](../ProOverload.md).
- For each type `R` in `Rs`, if `R::is_direct` is `false` and `typename R::reflector_type` meets the [*ProAccessible* requirements](../ProAccessible.md) of `box<F>, typename R::reflector_type`, `typename R::reflector_type::template accessor<box<F>, typename R::reflector_type>` is inherited by `box<F>`.

That is, `box<F>` inherits the *indirect* accessors of `F` (those that operate on the contained value). The contained value can therefore be used directly through a `box`, without an explicit indirection operator.

## Member Types

| Name          | Description |
| ------------- | ----------- |
| `facade_type` | `F`         |

## Member Functions

| Name                                                 | Description                                          |
| ---------------------------------------------------- | --------------------------------------------------- |
| [(constructor)](constructor.md)                      | constructs a `box` object                           |
| [(destructor)](destructor.md)                        | destroys a `box` object                             |
| [`emplace`<br />`emplace_alloc`](emplace.md)         | constructs the contained value in-place             |
| [`operator bool`<br />`has_value`](operator_bool.md) | checks if the `box` contains a value                |
| [`operator=`](assignment.md)                         | assigns a `box` object                              |
| [`release`](release.md)                              | releases the ownership as a `proxy`                 |
| [`reset`](reset.md)                                  | destroys any contained value                        |
| [`swap`](swap.md)                                    | exchanges the contents                              |
| [`operator proxy_indirect_accessor`](conversion.md)  | converts to a reference of the indirect accessor    |

## Non-Member Functions

| Name                                        | Description                                                  |
| ------------------------------------------- | ------------------------------------------------------------ |
| [`operator==`](friend_operator_equality.md) | compares a `box` with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt) |
| [`swap`](friend_swap.md)                    | overload the [`std::swap`](https://en.cppreference.com/w/cpp/algorithm/swap) algorithm |

## Comparing with `proxy` and `std::any`

`box` complements [`proxy`](../proxy/README.md): use `proxy` when you want to refer to an object whose storage is managed elsewhere (pointer semantics), and use `box` when you want an independent, value-semantic object that owns whatever it holds.

Compared with [`std::any`](https://en.cppreference.com/w/cpp/utility/any), `box` is open to abstractions: the runtime requirements are described by a [`facade`](../facade.md), so the contained value can be used through any conventions or reflections defined by that facade, while `std::any` only supports type-safe casting. `box` also supports custom allocators, conditional copyability, and a strong no-allocation guarantee for small targets, all of which are inherited from the underlying `proxy`.

## Example

```cpp
#include <iostream>
#include <string>

#include <proxy/proxy.h>

PRO_DEF_FREE_DISPATCH(FreeToString, std::to_string, ToString);

struct Stringable : pro::facade_builder                                 //
                    ::add_convention<FreeToString, std::string() const> //
                    ::support_copy<pro::constraint_level::nontrivial>   //
                    ::build {};

int main() {
  pro::box<Stringable> b1 = 123;
  std::cout << ToString(b1) << "\n"; // Prints "123"

  // Copying a box performs a deep copy of the contained value.
  pro::box<Stringable> b2 = b1;
  b1 = 3.14;
  std::cout << ToString(b1) << "\n"; // Prints "3.140000"
  std::cout << ToString(b2) << "\n"; // Prints "123"

  // A box can be emptied and queried like std::optional.
  b1 = std::nullopt;
  std::cout << std::boolalpha << (b1 == std::nullopt) << "\n"; // Prints "true"
}
```

## See Also

- [class template `proxy`](../proxy/README.md)
- [function template `make_proxy`](../make_proxy.md)
- [concept `proxiable`](../proxiable.md)
