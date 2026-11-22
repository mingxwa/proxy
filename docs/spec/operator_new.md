# Function templates `operator new`, `operator new[]`, `operator delete`, `operator delete[]`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: global  
> Since: 5.0.0

```cpp
// (1)
template <class Self, class... Args>
void* operator new(std::size_t size, Self&& self, Args&&... args)
    noexcept(/* see below */);

// (2)
template <class Self, class... Args>
void* operator new(std::size_t size, std::align_val_t alignment, Self&& self,
                   Args&&... args) noexcept(/* see below */);

// (3)
template <class Self, class... Args>
void* operator new[](std::size_t size, Self&& self, Args&&... args)
    noexcept(/* see below */);

// (4)
template <class Self, class... Args>
void* operator new[](std::size_t size, std::align_val_t alignment, Self&& self,
                     Args&&... args) noexcept(/* see below */);

// (5)
template <class Self, class... Args>
void operator delete(void* ptr, Self self, Args... args) noexcept;

// (6)
template <class Self, class... Args>
void operator delete(void* ptr, std::align_val_t alignment, Self self,
                     Args... args) noexcept;

// (7)
template <class Self, class... Args>
void operator delete[](void* ptr, Self self, Args... args) noexcept;

// (8)
template <class Self, class... Args>
void operator delete[](void* ptr, std::align_val_t alignment, Self self,
                       Args... args) noexcept;
```

These placement allocation and deallocation function templates let a [`proxy`](proxy/README.md) or a [`proxy_indirect_accessor`](proxy_indirect_accessor/README.md) be the first placement argument of a `new` expression, for example `new (*p) T` or `new (p, args...) T[n]`. Each one forwards to the friend function that the accessor of [`operator_access`](operator_access/accessor.md)`<"new">`, `<"new[]">`, `<"delete">` or `<"delete[]">` provides for the operations of the operand, denoted *placement-function* below. C++ allows allocation and deallocation functions only as class members or in the global namespace, and a class member cannot reach the contained value of a `proxy` because it is implicitly static, so these templates are declared in the global namespace.

Let *placement-function* be the friend function of the accessor of `operator_access<"new">` for `(1)` and `(2)`, of `operator_access<"new[]">` for `(3)` and `(4)`, of `operator_access<"delete">` for `(5)` and `(6)`, and of `operator_access<"delete[]">` for `(7)` and `(8)`. Let *first* be `size` for `(1)` to `(4)` and `ptr` for `(5)` to `(8)`. Each overload participates in overload resolution only if `std::remove_cvref_t<Self>` is a specialization of `proxy` or `proxy_indirect_accessor`, and the expression *call* is well-formed, where *call* is

- `placement-function(std::forward<Self>(self), first, std::forward<Args>(args)...)` for `(1)`, `(3)`, `(5)` and `(7)`, which additionally participate in overload resolution only if `sizeof...(Args)` is `0` or `std::remove_cvref_t` of the first type in `Args` is not `std::align_val_t`,
- `placement-function(std::forward<Self>(self), first, alignment, std::forward<Args>(args)...)` for `(2)`, `(4)`, `(6)` and `(8)`.

For `(1)` to `(4)`, *call* shall also be implicitly convertible to `void*`. For `(5)` to `(8)`, its type shall be `void`, and `Self` and every type in `Args` shall be reference types.

`(1)` to `(4)` are equivalent to `return call;`. Their exception specification is `noexcept(noexcept(call))`, so a `new` expression treats them as non-throwing allocation functions when the signature of the selected operation is `noexcept`.

`(5)` to `(8)` are equivalent to `call;`.

## Notes

A `new` expression for a type with new-extended alignment passes `std::align_val_t` before the placement arguments, so `(2)` and `(4)` forward it to the operation before the other arguments. Since the operation receives the arguments without the `proxy`, `std::align_val_t` as the first argument after the `proxy` would be indistinguishable from that alignment argument. `(1)`, `(3)`, `(5)` and `(7)` therefore do not accept it, and an expression such as `new (*p, std::align_val_t{16}) T` is ill-formed.

A `new` expression calls `(5)` to `(8)` only to deallocate the storage when the initialization of the object throws an exception. As for any placement allocation, a `delete` expression never calls them, so the storage allocated with `(1)` to `(4)` is released through the target directly. A `new` expression finds the deallocation function template that matches an allocation function template specialization by deducing its template arguments from the function type of that specialization ([[temp.deduct.decl]](https://eel.is/c++draft/temp.deduct.decl)), which yields reference types for `Self` and `Args`, since the parameters of `(1)` to `(4)` are forwarding references. `(5)` to `(8)` therefore take `Self` and `Args` as written and require them to be reference types, so that a call that would deduce object types and copy the operands is not accepted.

[`operator_dispatch`](operator_dispatch/operator_call.md)`<"new">` and the other placement specializations call the allocation or deallocation function of the target with the target as the first placement argument.

## Example

```cpp
#include <cstddef>
#include <iostream>
#include <new>
#include <stdexcept>

#include <proxy/proxy.h>

struct Arena {
  std::size_t allocated = 0;
  std::size_t released = 0;
};

void* operator new(std::size_t size, Arena& arena) {
  arena.allocated += size;
  return ::operator new(size);
}

void operator delete(void* ptr, Arena& arena) noexcept {
  ++arena.released;
  ::operator delete(ptr);
}

struct Allocator : pro::facade_builder                                //
                   ::add_convention<pro::operator_dispatch<"new">,    //
                                    void*(std::size_t)>               //
                   ::add_convention<pro::operator_dispatch<"delete">, //
                                    void(void*) noexcept>             //
                   ::build {};

struct Widget {
  explicit Widget(bool fail) {
    if (fail) {
      throw std::runtime_error{"failed"};
    }
  }
};

int main() {
  Arena arena;
  pro::proxy<Allocator> p = &arena;
  int* value = new (*p) int{42};
  std::cout << *value << "\n"; // Prints "42"
  ::operator delete(value);
  try {
    new (*p) Widget{true};
  } catch (const std::runtime_error&) {
  }
  std::cout << arena.released << "\n"; // Prints "1"
}
```

## See Also

- [class template `operator_access`](operator_access/README.md)
- [class template `operator_dispatch`](operator_dispatch/README.md)
