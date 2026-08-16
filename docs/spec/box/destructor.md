# `box<F>::~box`

```cpp
~box() = default;
```

Destroys the `box` object. If the `box` contains a value, the contained value is also destroyed, and any storage allocated for it is deallocated.

The destructor is defaulted; it destroys the underlying [`proxy`](../proxy/README.md)`<F>` object, and is therefore trivial when `F::destructibility` is `constraint_level::trivial`.

## Example

```cpp
#include <cstdio>
#include <utility>

#include <proxy/box.h>

struct Any : pro::facade_builder::build {};

struct Foo {
  ~Foo() { puts("Destroy Foo"); }
};

int main() {
  pro::box<Any> b{std::in_place_type<Foo>};
} // The destructor of `Foo` is called when `b` is destroyed
```

## See Also

- [`reset`](reset.md)
- [`release`](release.md)
