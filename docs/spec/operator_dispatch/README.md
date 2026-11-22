# Class template `operator_dispatch`

> Header: `proxy.h`  
> Module: `proxy`  
> Namespace: `pro::inline v5`

The definition of `operator_dispatch` makes use of an exposition-only type *string-literal*, which is constructible from a `char` array at compile-time and can be used as a non-type template argument.

```cpp
template <string-literal Sign, bool Rhs = false>
class operator_dispatch;
```

Class template `operator_dispatch` is a [dispatch](../ProxyDispatch.md) type for operator expressions. `Sign` represents the sign of operator (SOP) as a string literal (e.g., `"+"` for operator `+`). `Rhs` specifies whether the [`proxy`](../proxy/README.md) operand is on the right-hand side of a binary operator. An operation of `operator_dispatch<Sign, Rhs>` added via [`basic_facade_builder`](../basic_facade_builder/add_convention.md) has the access type [`operator_access<Sign, Rhs>`](../operator_access/README.md).

*Since 5.0.0*: the accessibility is provided by `operator_access`. Previously, `operator_dispatch` provided the accessibility via its member template `accessor`, whose functions for the left-hand-side operand were member functions. `operator_dispatch` also supports `"co_await"`, `"new"`, `"new[]"`, `"delete"` and `"delete[]"`.

## Supported SOPs

As per C++20 specifications, there are 45 different SOPs. `operator_dispatch` supports the following 42 SOPs:

```text
"+", "-", "*", "/", "%", "++", "--", "==", "!=", ">", "<", ">=", "<=", "<=>", "!", "&&", "||", "~", "&", "|", "^", "<<", ">>", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", ",", "->*", "()", "[]", "co_await", "new", "new[]", "delete", "delete[]"
```

`"co_await"` is supported only when the implementation supports coroutines (`__cpp_impl_coroutine >= 201902L`).

The following 3 SOPs are not supported:

- Assignment (`=`) and member access (`->`) can only be overloaded by non-static member functions ([[over.ass]](https://eel.is/c++draft/over.ass), [[over.ref]](https://eel.is/c++draft/over.ref)). Such functions of `proxy` would collide with the assignment operators and `operator->` of `proxy` itself, and the copy assignment operator of [`proxy_indirect_accessor`](../proxy_indirect_accessor/README.md) hides any `operator=` of its accessors.
- Literal operators (`""`) only take parameters of the types listed in [[over.literal]](https://eel.is/c++draft/over.literal), so a `proxy` can never be an operand.

## Type Conversion

Type conversion expressions, although using `operator` syntax, do not have a specific sign and are not within the scope of `operator_dispatch`. They are supported by [class `explicit_conversion_dispatch` (aka. `conversion_dispatch`)](../explicit_conversion_dispatch/README.md) and [class `implicit_conversion_dispatch`](../implicit_conversion_dispatch/README.md).

## Specializations

Let `self` be the operand of [`proxy`](../proxy/README.md), and `other` and `others...` be the other operand(s) in the expression of an operator. `operator_dispatch` has the following specializations for various expressions. Instantiating `operator_dispatch` with any other `Sign` and `Rhs` is ill-formed. The `new` expressions are available through the [allocation and deallocation function templates](../operator_new.md) declared in the global namespace.

| Specializations                   | Expressions             |
| --------------------------------- | ----------------------- |
| `operator_dispatch<"+", false>`   | `+self`, `self + other` |
| `operator_dispatch<"-", false>`   | `-self`, `self - other` |
| `operator_dispatch<"*", false>`   | `*self`, `self * other` |
| `operator_dispatch<"/", false>`   | `self / other`          |
| `operator_dispatch<"%", false>`   | `self % other`          |
| `operator_dispatch<"++", false>`  | `++self`, `self++`      |
| `operator_dispatch<"--", false>`  | `--self`, `self--`      |
| `operator_dispatch<"==", false>`  | `self == other`         |
| `operator_dispatch<"!=", false>`  | `self != other`         |
| `operator_dispatch<">", false>`   | `self > other`          |
| `operator_dispatch<"<", false>`   | `self < other`          |
| `operator_dispatch<">=", false>`  | `self >= other`         |
| `operator_dispatch<"<=", false>`  | `self <= other`         |
| `operator_dispatch<"<=>", false>` | `self <=> other`        |
| `operator_dispatch<"!", false>`   | `!self`                 |
| `operator_dispatch<"&&", false>`  | `self && other`         |
| `operator_dispatch<"||", false>`  | `self || other`         |
| `operator_dispatch<"~", false>`   | `~self`                 |
| `operator_dispatch<"&", false>`   | `&self`, `self & other` |
| `operator_dispatch<"|", false>`   | `self | other`          |
| `operator_dispatch<"^", false>`   | `self ^ other`          |
| `operator_dispatch<"<<", false>`  | `self << other`         |
| `operator_dispatch<">>", false>`  | `self >> other`         |
| `operator_dispatch<"+=", false>`  | `self += other`         |
| `operator_dispatch<"-=", false>`  | `self -= other`         |
| `operator_dispatch<"*=", false>`  | `self *= other`         |
| `operator_dispatch<"/=", false>`  | `self /= other`         |
| `operator_dispatch<"%=", false>`  | `self %= other`         |
| `operator_dispatch<"&=", false>`  | `self &= other`         |
| `operator_dispatch<"|=", false>`  | `self |= other`         |
| `operator_dispatch<"^=", false>`  | `self ^= other`         |
| `operator_dispatch<"<<=", false>` | `self <<= other`        |
| `operator_dispatch<">>=", false>` | `self >>= other`        |
| `operator_dispatch<",", false>`   | `self, other`           |
| `operator_dispatch<"->*", false>` | `self ->* other`        |
| `operator_dispatch<"()", false>`  | `self(others...)`       |
| `operator_dispatch<"[]", false>`  | `self[other]` (until C++23)<br />`self[others...]` (since C++23) |
| `operator_dispatch<"co_await", false>` | `co_await self` |
| `operator_dispatch<"new", false>` | `new (self, others...) T` |
| `operator_dispatch<"new[]", false>` | `new (self, others...) T[n]` |
| `operator_dispatch<"delete", false>` | `new (self, others...) T` when the initialization of `T` throws |
| `operator_dispatch<"delete[]", false>` | `new (self, others...) T[n]` when the initialization of an element throws |
| `operator_dispatch<"+", true>`    | `other + self`          |
| `operator_dispatch<"-", true>`    | `other - self`          |
| `operator_dispatch<"*", true>`    | `other * self`          |
| `operator_dispatch<"/", true>`    | `other / self`          |
| `operator_dispatch<"%", true>`    | `other % self`          |
| `operator_dispatch<"==", true>`   | `other == self`         |
| `operator_dispatch<"!=", true>`   | `other != self`         |
| `operator_dispatch<">", true>`    | `other > self`          |
| `operator_dispatch<"<", true>`    | `other < self`          |
| `operator_dispatch<">=", true>`   | `other >= self`         |
| `operator_dispatch<"<=", true>`   | `other <= self`         |
| `operator_dispatch<"<=>", true>`  | `other <=> self`        |
| `operator_dispatch<"&&", true>`   | `other && self`         |
| `operator_dispatch<"||", true>`   | `other || self`         |
| `operator_dispatch<"&", true>`    | `other & self`          |
| `operator_dispatch<"|", true>`    | `other | self`          |
| `operator_dispatch<"^", true>`    | `other ^ self`          |
| `operator_dispatch<"<<", true>`   | `other << self`         |
| `operator_dispatch<">>", true>`   | `other >> self`         |
| `operator_dispatch<"+=", true>`   | `other += self`         |
| `operator_dispatch<"-=", true>`   | `other -= self`         |
| `operator_dispatch<"*=", true>`   | `other *= self`         |
| `operator_dispatch<"/=", true>`   | `other /= self`         |
| `operator_dispatch<"%=", true>`   | `other %= self`         |
| `operator_dispatch<"&=", true>`   | `other &= self`         |
| `operator_dispatch<"|=", true>`   | `other |= self`         |
| `operator_dispatch<"^=", true>`   | `other ^= self`         |
| `operator_dispatch<"<<=", true>`  | `other <<= self`        |
| `operator_dispatch<">>=", true>`  | `other >>= self`        |
| `operator_dispatch<",", true>`    | `other, self`           |
| `operator_dispatch<"->*", true>`  | `other ->* self`        |

## Member Functions

| Name                             | Description                              |
| -------------------------------- | ---------------------------------------- |
| (constructor) [nothrow]          | constructs an `operator_dispatch` object |
| [`operator()`](operator_call.md) | invokes the dispatch                     |

## Example

```cpp
#include <iomanip>
#include <iostream>
#include <numbers>

#include <proxy/proxy.h>

struct Number : pro::facade_builder                                       //
                ::add_convention<pro::operator_dispatch<"*=">, void(int)> //
                ::add_convention<pro::operator_dispatch<"<<", true>,
                                 std::ostream&(std::ostream&) const&> //
                ::build {};

int main() {
  pro::proxy<Number> p1 = pro::make_proxy<Number>(std::numbers::pi);
  *p1 *= 3;
  std::cout << std::setprecision(10) << *p1 << "\n"; // Prints: 9.424777961

  pro::proxy<Number> p2 = pro::make_proxy<Number>(10);
  *p2 *= 5;
  std::cout << *p2 << "\n"; // Prints: 50
}
```

## See Also

- [class template `operator_access`](../operator_access/README.md)
- [function templates `operator new`, `operator new[]`, `operator delete`, `operator delete[]`](../operator_new.md)
- [class `conversion_dispatch`](../explicit_conversion_dispatch/README.md)
