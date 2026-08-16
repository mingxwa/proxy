# Class template `box`

> Header: `box.h`  
> Module: `proxy`  
> Namespace: `pro::inline v4`  
> Since: 5.0.0

```cpp
template <facade F>
class box;  // freestanding-deleted
```

Class template `box` is a value-semantics counterpart of [`proxy`](../proxy/README.md). A `proxy<F>` wraps a *pointer* and delegates lifetime management to that pointer; a `box<F>` always *owns* the object it contains, like [`std::any`](https://en.cppreference.com/w/cpp/utility/any) or [`std::optional`](https://en.cppreference.com/w/cpp/utility/optional), while keeping the abstraction expressed by the [facade](../facade.md) `F`.

Any instance of `box<F>` at any given point in time either *contains a value* or *does not contain a value*. When it contains a value, the value is an object of some type `T`, whose lifetime ends when the `box` is destroyed, reset, or assigned to. The storage for that object is either part of the `box` object footprint or allocated, following the same rules as [`make_proxy`](../make_proxy.md) and [`allocate_proxy`](../allocate_proxy.md).

Let `Cs` be the convention types of `F` and of every super of `F`, reachable via `typename F::super_types` transitively, and `Rs` be the reflection types of `F` and of every such super.

- For each distinct dispatch type `D` among the types `C` in `Cs` where `C::is_direct` is `false`, let `Os...` be the overload types of those conventions with duplicates removed, and `substituted-overload-types...` be [`substituted-overload<Os, F>...`](../ProOverload.md). If `D` meets the [*ProAccessible* requirements](../ProAccessible.md) of `box<F>, D, substituted-overload-types...`, `typename D::template accessor<box<F>, D, substituted-overload-types...>` is inherited by `box<F>`.
- For each type `R` in `Rs`, if `R::is_direct` is `false` and `typename R::reflector_type` meets the [*ProAccessible* requirements](../ProAccessible.md) of `box<F>, typename R::reflector_type`, `typename R::reflector_type::template accessor<box<F>, typename R::reflector_type>` is inherited by `box<F>`.

These are the same accessors that [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<F>` provides, hosted on `box<F>` instead. They are therefore reached directly on the box, without an intervening `operator->` or `operator*`.

## Member Types

| Name          | Description |
| ------------- | ----------- |
| `facade_type` | `F`         |

## Member Functions

| Name                                                         | Description                                            |
| ------------------------------------------------------------ | ------------------------------------------------------ |
| [(constructor)](constructor.md)                              | constructs a `box` object                              |
| [(destructor)](destructor.md)                                | destroys a `box` object                                |
| [`emplace`<br />`emplace_alloc`](emplace.md)                 | constructs the contained value in-place                |
| [`operator bool`<br />`has_value`](operator_bool.md)         | checks if the `box` contains a value                   |
| [`operator proxy_indirect_accessor&`](conversion.md)         | converts to the indirect accessor of the wrapped proxy |
| [`operator=`](assignment.md)                                 | assigns a `box` object                                 |
| [`release`](release.md)                                      | releases the ownership to a `proxy` object             |
| [`reset`](reset.md)                                          | destroys any contained value                           |
| [`swap`](swap.md)                                            | exchanges the contents                                 |

## Non-Member Functions

| Name                                        | Description                                                  |
| ------------------------------------------- | ------------------------------------------------------------ |
| [`operator==`](friend_operator_equality.md) | compares a `box` with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt) |
| [`swap`](friend_swap.md)                    | overloads the [`std::swap`](https://en.cppreference.com/w/cpp/algorithm/swap) algorithm |
| [`invoke`](friend_invoke.md)                | invokes a `box` with a specified convention                  |
| [`reflect`](friend_reflect.md)              | acquires reflection information of a contained type          |

## Notes

Compared with [`std::any`](https://en.cppreference.com/w/cpp/utility/any), `box` is open to abstractions: the runtime requirements are described by a [facade](../facade.md), so the contained value can be used through the conventions and reflections defined by that facade, while `std::any` supports only type-safe casting. `box` also inherits from `proxy` the support for custom allocators, conditional copyability, and a strong no-allocation guarantee for small targets.

Only the *indirect* conventions and reflections of `F` are accessible from a `box`. A *direct* convention or reflection describes the contained *pointer*, which is an implementation detail of the ownership model chosen by the `box`. Use [`release`](release.md) to obtain the underlying `proxy<F>` when a direct accessor is needed.

`box<F>` compares with [`std::nullopt`](https://en.cppreference.com/w/cpp/utility/optional/nullopt) rather than with `nullptr`, because a `box` is not a pointer.

## Example

```cpp
#include <iostream>
#include <vector>

#include <proxy/box.h>

PRO_DEF_MEM_DISPATCH(MemArea, Area);

struct Shape : pro::facade_builder                               //
               ::add_convention<MemArea, double() const>         //
               ::support_copy<pro::constraint_level::nontrivial> //
               ::build {};

class Rectangle {
public:
  Rectangle(double width, double height) : width_(width), height_(height) {}
  double Area() const { return width_ * height_; }

private:
  double width_;
  double height_;
};

class Circle {
public:
  explicit Circle(double radius) : radius_(radius) {}
  double Area() const { return 3.14159 * radius_ * radius_; }

private:
  double radius_;
};

int main() {
  // A box is a value: no `&`, no `make_proxy`, no dereference
  std::vector<pro::box<Shape>> shapes;
  shapes.emplace_back(Rectangle{2, 3});
  shapes.emplace_back(Circle{1});

  double total = 0;
  for (const auto& shape : shapes) {
    total += shape.Area();
  }
  std::cout << total << "\n"; // Prints "9.14159"
}
```

## See Also

- [class template `proxy`](../proxy/README.md)
- [function template `make_proxy`](../make_proxy.md)
