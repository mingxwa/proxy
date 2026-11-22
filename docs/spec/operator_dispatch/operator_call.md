# `operator_dispatch::operator()`

For different `Sign` and `Rhs`, `operator_dispatch<Sign, Rhs>::operator()` are defined differently. `sop` denotes the sign of operator of each specialization.

## Left-Hand-Side Operand Specializations

When `Rhs` is `false`, `operator()` of the specializations are defined as follows:

### Unary & Binary SOPs

When `Sign` is one of `"+"`, `"-"`, `"*"`, `"&"`,

```cpp
// (1)
template <class T>
/* see below */ operator()(T&& self) const
    noexcept(noexcept(sop std::forward<T>(self)))
    requires(requires { sop std::forward<T>(self); });

// (2)
template <class T, class Arg>
/* see below */ operator()(T&& self, Arg&& arg) const
    noexcept(noexcept(std::forward<T>(self) sop std::forward<Arg>(arg)))
    requires(requires { std::forward<T>(self) sop std::forward<Arg>(arg); });
```

`(1)` Returns `sop std::forward<T>(self)`.

`(2)` Returns `std::forward<T>(self) sop std::forward<Arg>(arg)`.

### Binary SOPs

When `Sign` is one of `"/"`, `"%"`, `"=="`, `"!="`, `">"`, `"<"`, `">="`, `"<="`, `"<=>"`, `"&&"`, `"||"`, `"|"`, `"^"`, `"<<"`, `">>"`, `"+="`, `"-="`, `"*="`, `"/="`, `"%="`, `"&="`, `"|="`, `"^="`, `"<<="`, `">>="`, `","`, `"->*"`,

```cpp
// (3)
template <class T, class Arg>
/* see below */ operator()(T&& self, Arg&& arg) const
    noexcept(noexcept(std::forward<T>(self) sop std::forward<Arg>(arg)))
    requires(requires { std::forward<T>(self) sop std::forward<Arg>(arg); });
```

`(3)` Returns `std::forward<T>(self) sop std::forward<Arg>(arg)`.

### `"++"` and `"--"`

When `Sign` is either `"++"`, `"--"`,

```cpp
// (4)
template <class T>
/* see below */ operator()(T&& self) const
    noexcept(noexcept(sop std::forward<T>(self)))
    requires(requires { sop std::forward<T>(self); });

// (5)
template <class T>
/* see below */ operator()(T&& self, int) const
    noexcept(noexcept(std::forward<T>(self) sop))
    requires(requires { std::forward<T>(self) sop; });
```

`(4)` Returns `sop std::forward<T>(self)`.

`(5)` Returns `std::forward<T>(self) sop`.

### `"!"` and `"~"`

When `Sign` is either `"!"`, `"~"`,

```cpp
// (6)
template <class T>
/* see below */ operator()(T&& self) const
    noexcept(noexcept(sop std::forward<T>(self)))
    requires(requires { sop std::forward<T>(self); });
```

`(6)` Returns `sop std::forward<T>(self)`.

### `"()"`

When `Sign` is `"()"`,

```cpp
// (7)
template <class T, class... Args>
/* see below */ operator()(T&& self, Args&&... args) const
    noexcept(noexcept(std::forward<T>(self)(std::forward<Args>(args)...)))
    requires(requires { std::forward<T>(self)(std::forward<Args>(args)...); });
```

`(7)` Returns `std::forward<T>(self)(std::forward<Args>(args)...)`.

### `"[]"`

When `Sign` is `"[]"`,

```cpp
// (8) (until C++23)
template <class T, class Arg>
/* see below */ operator()(T&& self, Arg&& arg) const
    noexcept(noexcept(std::forward<T>(self)[std::forward<Arg>(arg)]))
    requires(requires { std::forward<T>(self)[std::forward<Arg>(arg)]; });

// (9) (since C++23)
template <class T, class... Args>
/* see below */ operator()(T&& self, Args&&... args) const
    noexcept(noexcept(std::forward<T>(self)[std::forward<Args>(args)...]))
    requires(requires { std::forward<T>(self)[std::forward<Args>(args)...]; });
```

`(8)` Returns `std::forward<T>(self)[std::forward<Arg>(arg)]`.

`(9)` Returns `std::forward<T>(self)[std::forward<Args>(args)...]`. Requires the [multidimensional subscript feature](https://en.cppreference.com/w/cpp/language/operators#Array_subscript_operator) in C++23.

### `"co_await"`

When `Sign` is `"co_await"` and the implementation supports coroutines (`__cpp_impl_coroutine >= 201902L`),

```cpp
// (10)
template <class T>
/* see below */ operator()(T&& self) const
    noexcept(noexcept(std::forward<T>(self).operator co_await()))
    requires(requires { std::forward<T>(self).operator co_await(); });

// (11)
template <class T>
/* see below */ operator()(T&& self) const
    noexcept(noexcept(operator co_await(std::forward<T>(self))))
    requires(!requires { std::forward<T>(self).operator co_await(); } &&
             requires { operator co_await(std::forward<T>(self)); });

// (12)
template <class T>
T&& operator()(T&& self) const noexcept
    requires(!requires { std::forward<T>(self).operator co_await(); } &&
             !requires { operator co_await(std::forward<T>(self)); });
```

`(10)` Returns `std::forward<T>(self).operator co_await()`.

`(11)` Returns `operator co_await(std::forward<T>(self))`.

`(12)` Returns `std::forward<T>(self)`.

`(10)` to `(12)` obtain the awaiter of `self` like an await-expression whose promise type has no `await_transform` member, which the C++ standard names `GET-AWAITER(self)` in [[exec.awaitable]](https://eel.is/c++draft/exec.awaitable). An await-expression cannot appear outside a coroutine, so they call `operator co_await` explicitly instead. Unlike an await-expression, which selects among the member and non-member `operator co_await` by overload resolution ([[expr.await]](https://eel.is/c++draft/expr.await)), `(10)` takes precedence over `(11)` whenever it is viable.

### Placement SOPs

When `Sign` is one of `"new"`, `"new[]"`, `"delete"`, `"delete[]"`, let `First` be `std::size_t` for `"new"` and `"new[]"`, or `void*` for `"delete"` and `"delete[]"`,

```cpp
// (13)
template <class T, class... Args>
/* see below */ operator()(T&& self, First first, std::align_val_t alignment,
                           Args&&... args) const
    noexcept(noexcept(operator sop(first, alignment, std::forward<T>(self),
                                   std::forward<Args>(args)...)))
    requires(requires { operator sop(first, alignment, std::forward<T>(self),
                                     std::forward<Args>(args)...); });

// (14)
template <class T, class... Args>
/* see below */ operator()(T&& self, First first, Args&&... args) const
    noexcept(noexcept(operator sop(first, std::forward<T>(self),
                                   std::forward<Args>(args)...)))
    requires(requires { operator sop(first, std::forward<T>(self),
                                     std::forward<Args>(args)...); });
```

`(13)` Returns `operator sop(first, alignment, std::forward<T>(self), std::forward<Args>(args)...)`.

`(14)` Returns `operator sop(first, std::forward<T>(self), std::forward<Args>(args)...)`. Participates in overload resolution only if `sizeof...(Args)` is `0` or `std::remove_cvref_t` of the first type in `Args` is not `std::align_val_t`.

`(13)` and `(14)` call the allocation or deallocation function that a `new` expression with `self` as its first placement argument would call, placing an alignment argument before `self` as a `new` expression does for a type with new-extended alignment. The function is found by unqualified name lookup from the definition of `operator_dispatch` together with [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl), rather than by the lookup in the global scope that a `new` expression performs. Since allocation and deallocation functions are declared in the global namespace, a function declared after the definition of `operator_dispatch` is found only when the global namespace is an associated namespace of an argument, for example when `self` is of a class type declared in the global namespace.

## Right-Hand-Side Operand Specializations

When `Sign` is one of `"+"`, `"-"`, `"*"`, `"/"`, `"%"`, `"=="`, `"!="`, `">"`, `"<"`, `">="`, `"<="`, `"<=>"`, `"&&"`, `"||"`, `"&"`, `"|"`, `"^"`, `"<<"`, `">>"`, `"+="`, `"-="`, `"*="`, `"/="`, `"%="`, `"&="`, `"|="`, `"^="`, `"<<="`, `">>="`, `","`, `"->*"`, and `Rhs` is `true`,

```cpp
// (15)
template <class T, class Arg>
/* see below */ operator()(T&& self, Arg&& arg) const
    noexcept(noexcept(std::forward<Arg>(arg) sop std::forward<T>(self)))
    requires(requires { std::forward<Arg>(arg) sop std::forward<T>(self); });
```

`(15)` Returns `std::forward<Arg>(arg) sop std::forward<T>(self)`.
