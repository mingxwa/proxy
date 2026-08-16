# Function template `reflect` (`box<F>`)

```cpp
template <class R>
const R& reflect(const box<F>& v) noexcept;
```

Acquires reflection information of the contained type of a `box<F>`, through an *indirect* reflection.

Equivalent to [`reflect`](../proxy_indirect_accessor/friend_reflect.md)`<R>(ia)`, where `ia` is the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<F>` of the underlying [`proxy`](../proxy/README.md)`<F>`. The behavior is undefined if `v` does not contain a value.

There shall be a reflection type `Refl` defined in the reflection types of `F` or of any super of `F`, reachable via `typename F::super_types` transitively, where

- `Refl::is_direct` is `false`, and
- `typename Refl::reflector_type` is `R`.

The reference obtained from `reflect()` may be invalidated if `v` is subsequently modified.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F>` is an associated class of the arguments.

To acquire a *direct* reflection, call [`release`](release.md) and use [`reflect`](../proxy/friend_reflect.md) on the resulting `proxy<F>`.

## Example

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include <proxy/box.h>

class TriviallyCopyableReflector {
public:
  TriviallyCopyableReflector() = default;
  template <class T>
  constexpr explicit TriviallyCopyableReflector(
      std::in_place_type_t<T>) noexcept
      : value_(std::is_trivially_copyable_v<T>) {}

  template <class P, class R>
  struct accessor {
    bool IsTriviallyCopyable() const noexcept {
      const TriviallyCopyableReflector& self =
          reflect<R>(static_cast<const P&>(*this));
      return self.value_;
    }
  };

private:
  bool value_;
};

struct Reflectable : pro::facade_builder                          //
                     ::add_reflection<TriviallyCopyableReflector> //
                     ::build {};

int main() {
  pro::box<Reflectable> b1 = 123;
  std::cout << std::boolalpha << b1.IsTriviallyCopyable() << "\n"; // "true"

  pro::box<Reflectable> b2{std::in_place_type<std::string>, "hello"};
  std::cout << b2.IsTriviallyCopyable() << "\n"; // Prints "false"
}
```

## See Also

- [function template `invoke` (`box<F>`)](friend_invoke.md)
- [alias template `basic_facade_builder::add_reflection`](../basic_facade_builder/add_reflection.md)
