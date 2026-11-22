# Class template `operator_access::accessor`

```cpp
// (1)
template <class Self, class... Ds>
struct accessor {
  accessor() = delete;
};
```

`(1)` The default implementation of `accessor` is not constructible.

For different `Sign` and `Rhs`, `operator_access<Sign, Rhs>::accessor` has different specializations. `sop` denotes the sign of operator of each specialization. Each type in `Ds` is a *descriptor* (see [*ProxyAccess* requirements](../ProxyAccess.md)), and an operation is described by [`proxy_operation`](../proxy_operation.md)`<D, S>`, where `D` is a [dispatch](../ProxyDispatch.md) type and `S` is a [signature](../ProxySignature.md) type.

C++ requires `operator()` and `operator[]` to be member functions, so the accessors for `"()"` and `"[]"` provide member functions. All the other functions are friend functions defined in the accessor, which [argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl) finds when `Self` is an associated class of the arguments. Unlike member functions, they take the `proxy` operand with the same conversions as the other operands, so the [rewritten and reversed candidates](https://en.cppreference.com/w/cpp/language/overload_resolution#Call_to_an_overloaded_operator) of C++20 comparisons neither take precedence over them nor make them ambiguous.

## Aggregate Specializations

```cpp
// (2)
template <class Self, class... Ds>
    requires(sizeof...(Ds) > 1u && (std::is_constructible_v<accessor<Self, Ds>> && ...))
struct accessor<Self, Ds...> : accessor<Self, Ds>... {
  using accessor<Self, Ds>::operator sop...;
};

// (3)
template <class Self, class... Ds>
    requires(sizeof...(Ds) > 1u && (std::is_constructible_v<accessor<Self, Ds>> && ...))
struct accessor<Self, Ds...> : accessor<Self, Ds>... {};
```

`(2)` When `Sign` is `"()"` or `"[]"`, `sizeof...(Ds)` is greater than `1`, and `accessor<Self, Ds>...` are default-constructible types, inherits all `accessor<Self, Ds>...` types and `using` their `operator sop`.

`(3)` For any other `Sign` and `Rhs`, when `sizeof...(Ds)` is greater than `1`, and `accessor<Self, Ds>...` are default-constructible types, inherits all `accessor<Self, Ds>...` types.

The other specializations are defined as follows, where `sizeof...(Ds)` is `1` and the only type in `Ds` is `proxy_operation<D, S>`, with `S` qualified with `cv ref noex`. When `S` matches none of the specializations for `Sign` and `Rhs`, such as `R()` for `"/"`, the default implementation `(1)` applies, so `operator_access<Sign, Rhs>` provides no accessor (see [*ProxyAccess* requirements](../ProxyAccess.md)).

## Left-Hand-Side Operand Specializations

When `Rhs` is `false`:

### Unary & Binary SOPs

When `Sign` is one of `"+"`, `"-"`, `"*"`, `"&"`,

```cpp
// (4)
template <class Self, class D, class R>
struct accessor<Self, proxy_operation<D, R() cv ref noex>> {
  friend R operator sop (Self cv <ref ? ref : &> self) noex;
};

// (5)
template <class Self, class D, class R, class Arg>
struct accessor<Self, proxy_operation<D, R(Arg) cv ref noex>> {
  friend R operator sop (Self cv <ref ? ref : &> self, Arg arg) noex;
};
```

`(4)` Provides a unary `operator sop(Self cv <ref ? ref : &>)` with the same *noex* specifier as the signature type. It is equivalent to `return invoke<D, R() cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self))`.

`(5)` Provides a binary `operator sop(Self cv <ref ? ref : &>, Arg)` with the same *noex* specifier as the signature type. It is equivalent to `return invoke<D, R(Arg) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), std::forward<Arg>(arg))`.

### Binary SOPs

When `Sign` is one of `"/"`, `"%"`, `"=="`, `"!="`, `">"`, `"<"`, `">="`, `"<="`, `"<=>"`, `"&&"`, `"||"`, `"|"`, `"^"`, `"<<"`, `">>"`, `","`, `"->*"`,

```cpp
// (6)
template <class Self, class D, class R, class Arg>
struct accessor<Self, proxy_operation<D, R(Arg) cv ref noex>> {
  friend R operator sop (Self cv <ref ? ref : &> self, Arg arg) noex;
};
```

`(6)` Provides an `operator sop(Self cv <ref ? ref : &>, Arg)` with the same *noex* specifier as the signature type. It is equivalent to `return invoke<D, R(Arg) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), std::forward<Arg>(arg))`.

### `"!"`, `"~"` and `"co_await"`

When `Sign` is one of `"!"`, `"~"`, `"co_await"`,

```cpp
// (7)
template <class Self, class D, class R>
struct accessor<Self, proxy_operation<D, R() cv ref noex>> {
  friend R operator sop (Self cv <ref ? ref : &> self) noex;
};
```

`(7)` Provides an `operator sop(Self cv <ref ? ref : &>)` with the same *noex* specifier as the signature type. It is equivalent to `return invoke<D, R() cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self))`. `operator_access<"co_await">` is defined only when the implementation supports coroutines (`__cpp_impl_coroutine >= 201902L`).

### `"++"` and `"--"`

When `Sign` is either `"++"` or `"--"`,

```cpp
// (8)
template <class Self, class D, class R>
struct accessor<Self, proxy_operation<D, R() cv ref noex>> {
  friend Self cv <ref ? ref : &> operator sop (Self cv <ref ? ref : &> self) noex;
};

// (9)
template <class Self, class D, class R>
struct accessor<Self, proxy_operation<D, R(int) cv ref noex>> {
  friend R operator sop (Self cv <ref ? ref : &> self, int) noex;
};
```

`(8)` Provides a prefix `operator sop(Self cv <ref ? ref : &>)` with the same *noex* specifier as the signature type. It calls `invoke<D, R() cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self))` and returns `static_cast<Self cv <ref ? ref : &>>(self)`.

`(9)` Provides a postfix `operator sop(Self cv <ref ? ref : &>, int)` with the same *noex* specifier as the signature type. It is equivalent to `return invoke<D, R(int) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), 0)`.

### Assignment SOPs

When `Sign` is one of `"+="`, `"-="`, `"*="`, `"/="`, `"%="`, `"&="`, `"|="`, `"^="`, `"<<="`, `">>="`,

```cpp
// (10)
template <class Self, class D, class R, class Arg>
struct accessor<Self, proxy_operation<D, R(Arg) cv ref noex>> {
  friend Self cv <ref ? ref : &> operator sop (Self cv <ref ? ref : &> self, Arg arg) noex;
};
```

`(10)` Provides an `operator sop(Self cv <ref ? ref : &>, Arg)` with the same *noex* specifier as the signature type. It calls `invoke<D, R(Arg) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), std::forward<Arg>(arg))` and returns `static_cast<Self cv <ref ? ref : &>>(self)`.

### Placement SOPs

When `Sign` is one of `"new"`, `"new[]"`, `"delete"`, `"delete[]"`,

```cpp
// (11)
template <class Self, class D, class R, class... Args>
struct accessor<Self, proxy_operation<D, R(Args...) cv ref noex>> {
  friend R placement-function (Self cv <ref ? ref : &> self, Args... args) noex;
};
```

`(11)` Provides a function *placement-function*`(Self cv <ref ? ref : &>, Args...)` with the same *noex* specifier as the signature type, where *placement-function* is an exposition-only name, distinct for each `Sign`, that only argument-dependent lookup finds. It is equivalent to `return invoke<D, R(Args...) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), std::forward<Args>(args)...)`. C++ allows allocation and deallocation functions only as class members or in the global namespace, so the [allocation and deallocation function templates](../operator_new.md) declared in the global namespace call it to evaluate `new` expressions whose first placement argument is a `proxy`.

### `"()"`

When `Sign` is `"()"`,

```cpp
// (12)
template <class Self, class D, class R, class... Args>
struct accessor<Self, proxy_operation<D, R(Args...) cv ref noex>> {
  R operator() (Args... args) cv ref noex;
};
```

`(12)` Provides an `operator()(Args...)` with the same *cv ref noex* specifiers as the signature type. `accessor::operator()(Args...)` is equivalent to `return invoke<D, R(Args...) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(*this), std::forward<Args>(args)...)`.

### `"[]"`

When `Sign` is `"[]"`,

```cpp
// (13) (until C++23)
template <class Self, class D, class R, class Arg>
struct accessor<Self, proxy_operation<D, R(Arg) cv ref noex>> {
  R operator[] (Arg arg) cv ref noex;
};

// (14) (since C++23)
template <class Self, class D, class R, class... Args>
struct accessor<Self, proxy_operation<D, R(Args...) cv ref noex>> {
  R operator[] (Args... args) cv ref noex;
};
```

`(13)` Provides an `operator[](Arg)` with the same *cv ref noex* specifiers as the signature type. `accessor::operator[](Arg)` is equivalent to `return invoke<D, R(Arg) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(*this), std::forward<Arg>(arg))`.

`(14)` Provides an `operator[](Args...)` with the same *cv ref noex* specifiers as the signature type. `accessor::operator[](Args...)` is equivalent to `return invoke<D, R(Args...) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(*this), std::forward<Args>(args)...)`. Requires the [multidimensional subscript feature](https://en.cppreference.com/w/cpp/language/operators#Array_subscript_operator) in C++23.

## Right-Hand-Side Operand Specializations

When `Rhs` is `true`:

### Regular SOPs

When `Sign` is one of `"+"`, `"-"`, `"*"`, `"/"`, `"%"`, `"=="`, `"!="`, `">"`, `"<"`, `">="`, `"<="`, `"<=>"`, `"&&"`, `"||"`, `"&"`, `"|"`, `"^"`, `"<<"`, `">>"`, `","`, `"->*"`,

```cpp
// (15)
template <class Self, class D, class R, class Arg>
struct accessor<Self, proxy_operation<D, R(Arg) cv ref noex>> {
  friend R operator sop (Arg arg, Self cv <ref ? ref : &> self) noex;
};
```

`(15)` Provides an `operator sop(Arg, Self cv <ref ? ref : &>)` with the same *noex* specifier as the signature type. It is equivalent to `return invoke<D, R(Arg) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), std::forward<Arg>(arg))`.

### Assignment SOPs

When `Sign` is one of `"+="`, `"-="`, `"*="`, `"/="`, `"%="`, `"&="`, `"|="`, `"^="`, `"<<="`, `">>="`,

```cpp
// (16)
template <class Self, class D, class R, class Arg>
struct accessor<Self, proxy_operation<D, R(Arg&) cv ref noex>> {
  friend Arg& operator sop (Arg& arg, Self cv <ref ? ref : &> self) noex;
};
```

`(16)` Provides an `operator sop(Arg&, Self cv <ref ? ref : &>)` with the same *noex* specifier as the signature type. It calls `invoke<D, R(Arg&) cv ref noex>(static_cast<Self cv <ref ? ref : &>>(self), arg)` and returns `arg`.
