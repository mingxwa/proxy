# `basic_facade_builder::add_view`

```cpp
template <class F>
using add_view = basic_facade_builder</* see below */>;  // since 3.2.0, deprecated since 3.2.1
```

The alias template `add_view` of `basic_facade_builder<Cs, Rs, C>` adds necessary convention types to allow implicit conversion from [`proxy`](../proxy.md)`<F1>` to [`proxy_view`](../proxy_view.md)`<F>` where `F1` is a [facade](../facade.md) type built from `basic_facade_builder`.

Let `p` be a value of type `proxy<F>`, `ptr` be the contained value of `p` (if any), the conversion from type `proxy<F1>&` to type `proxy_view<F>` is equivalent to `return raw-ptr{std::addressof(*ptr)}` if `p` contains a value, or otherwise equivalent to `return nullptr`. `raw-ptr` is an exposition-only type that `*raw-ptr`, `*std::as_const(raw-ptr)`, `*std::move(raw-ptr)` and `*std::move(std::as_const(raw-ptr))` are equivalent to `*ptr`, `*std::as_const(ptr)`, `*std::move(ptr)` and `*std::move(std::as_const(ptr))`, respectively.

## Notes

`add_view` is useful when a certain context does not take ownership of a `proxy` object. Similar to [`std::unique_ptr::get`](https://en.cppreference.com/w/cpp/memory/unique_ptr/get), [`std::shared_ptr::get`](https://en.cppreference.com/w/cpp/memory/shared_ptr/get) and the [borrowing mechanism in Rust](https://doc.rust-lang.org/rust-by-example/scope/borrow.html).

## Example

```cpp
#include <iostream>

#include "proxy.h"

struct RttiAware : pro::facade_builder
    ::support_rtti
    ::add_view<RttiAware>
    ::build {};

int main() {
  pro::proxy<RttiAware> p = pro::make_proxy<RttiAware>(123);
  pro::proxy_view<RttiAware> pv = p;
  proxy_cast<int&>(*pv) = 456;  // Modifies the contained object of p
  std::cout << proxy_cast<const int&>(*pv) << "\n";  // Prints "456"
  std::cout << proxy_cast<const int&>(*p) << "\n";  // Prints "456"
}
```

## See Also

- [`add_convention`](add_convention.md)
