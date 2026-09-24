# Class template `proxy_operation`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: `pro::inline v5`  
> Since: 5.0.0

```cpp
template <class D, class O>
struct proxy_operation {};
```

Class template `proxy_operation` is a *descriptor* of an operation performed on a [`proxy`](proxy/README.md) with a [dispatch](ProDispatch.md) type `D` and an [overload](ProOverload.md) type `O`. Descriptors are the template arguments following `Self` of the member template `accessor` of an access type (see [*ProAccessible* requirements](ProAccessible.md)), telling the access type which operations and reflections its accessor provides accessibility for.

For a convention type `C` of a [facade](facade.md) type `F` or of any super of `F`, the descriptor of `C` in [`proxy<F, MP>`](proxy/README.md) or [`proxy_indirect_accessor<F, MP>`](proxy_indirect_accessor/README.md) is `proxy_operation<typename C::dispatch_type, `[`substituted-overload`](ProOverload.md)`<typename C::overload_type, F, MP>>`. Within an accessor, the described operation is performed by calling [`invoke<D, O>`](proxy/friend_invoke.md) with the `Self` object, qualified with the *cv ref* qualifiers of `O`, and the arguments of `O`.

## Example

```cpp
#include <iostream>

#include <proxy/proxy.h>

struct AreaAccess {
  template <class Self, class... Ds>
  struct accessor {
    accessor() = delete;
  };
  template <class Self, class D>
  struct accessor<Self, pro::proxy_operation<D, double() const>> {
    double Area() const {
      return invoke<D, double() const>(static_cast<const Self&>(*this));
    }
  };
};

struct AreaDispatch {
  using access_type = AreaAccess;

  template <class T>
  double operator()(const T& self) const {
    return self.Width() * self.Height();
  }
};

struct Shape : pro::facade_builder                            //
               ::add_convention<AreaDispatch, double() const> //
               ::build {};

class Rectangle {
public:
  Rectangle(double width, double height) : width_(width), height_(height) {}
  double Width() const { return width_; }
  double Height() const { return height_; }

private:
  double width_;
  double height_;
};

int main() {
  pro::proxy<Shape> p = pro::make_proxy<Shape, Rectangle>(3, 5);
  std::cout << p->Area() << "\n"; // Prints "15"
}
```

## See Also

- [class template `proxy_reflection`](proxy_reflection.md)
- [named requirements *ProAccessible*](ProAccessible.md)
- [named requirements *ProBasicConvention*](ProBasicConvention.md)
