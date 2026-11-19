# Function template `reflect` (`box<F, MP>`)

```cpp
template <class M>
const M& reflect(const box<F, MP>& b) noexcept;
```

Acquires reflection information of the contained type of a `box<F, MP>`, through an *indirect* reflection.

Equivalent to [`reflect`](../proxy_indirect_accessor/friend_reflect.md)`<M>(ia)`, where `ia` is the [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md)`<F, MP>` of the underlying [`proxy`](../proxy/README.md)`<F, MP>`. The behavior is undefined if `b` does not contain a value.

There shall be a reflection type `Refl` defined in the reflection types of `F` or of any super of `F`, reachable via `typename F::super_types` transitively, where

- `Refl::is_direct` is `false`, and
- `typename Refl::metadata_type` is `M`.

The reference obtained from `reflect()` may be invalidated if `b` is subsequently modified.

This function is not visible to ordinary [unqualified](https://en.cppreference.com/w/cpp/language/unqualified_lookup) or [qualified lookup](https://en.cppreference.com/w/cpp/language/qualified_lookup). It can only be found by [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) when `box<F, MP>` is an associated class of the arguments.

To acquire a *direct* reflection, call [`release`](release.md) and use [`reflect`](../proxy/friend_reflect.md) on the resulting `proxy<F, MP>`.

## Example

```cpp
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include <proxy/box.h>

class TriviallyCopyableMetadata {
public:
  TriviallyCopyableMetadata() = default;
  template <class T>
  constexpr explicit TriviallyCopyableMetadata(std::in_place_type_t<T>) noexcept
      : value_(std::is_trivially_copyable_v<T>) {}

  template <class Self, class M>
  struct accessor {
    bool IsTriviallyCopyable() const noexcept {
      const TriviallyCopyableMetadata& self =
          reflect<M>(static_cast<const Self&>(*this));
      return self.value_;
    }
  };

private:
  bool value_;
};

struct Reflectable : pro::facade_builder                         //
                     ::add_reflection<TriviallyCopyableMetadata> //
                     ::build {};

int main() {
  pro::box<Reflectable> b1 = 123;
  std::cout << std::boolalpha << b1.IsTriviallyCopyable() << "\n"; // "true"

  pro::box<Reflectable> b2{std::in_place_type<std::string>, "hello"};
  std::cout << b2.IsTriviallyCopyable() << "\n"; // Prints "false"
}
```

## See Also

- [function template `invoke` (`box<F, MP>`)](friend_invoke.md)
- [alias template `basic_facade_builder::add_reflection`](../basic_facade_builder/add_reflection.md)
