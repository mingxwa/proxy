# Named requirements: *ProMetadataPolicy*

> Since: 5.0.0

A metadata policy chooses the function wrappers and metadata storage used by a [`proxy`](proxy/README.md). A type `MP` meets the *ProMetadataPolicy* requirements if it provides the invoker and storage types described below.

## Invoker

Let `F` be an implementation-defined function type without cv or ref qualifiers, possibly `noexcept`. The library provides a function `f` of type `F` that implements an invocation or a lifetime operation on the contained pointer. Its parameters include a pointer to the contained value and the arguments of the operation.

`typename MP::template invoker<F>` shall denote a trivially destructible object type `I`, such as `F*`. Let `v` be an object of type `I`, and `ci` be a reference to `const I`.

| Expressions | Requirements and semantics |
| ----------- | -------------------------- |
| `I()` | Is non-throwing. Creates an empty invoker. |
| `I i(f);` | Is non-throwing. Initializes `i` to refer to `f`. |
| `I i(ci);` | Is non-throwing. Initializes `i` to be empty if `ci` is empty, or otherwise to refer to the function `ci` refers to. |
| `v = ci` | Is non-throwing. Makes `v` empty if `ci` is empty, or otherwise makes `v` refer to the function `ci` refers to. |
| `v = I()` | Is non-throwing. Makes `v` empty. |
| `static_cast<bool>(ci)` | Is non-throwing. Returns `true` if and only if `ci` is not empty. |
| `ci(std::forward<Args>(args)...)` | Is well-formed whenever `f(std::forward<Args>(args)...)` is well-formed, with the same return type and effects. Is non-throwing whenever that function call expression is non-throwing. The behavior is undefined if `ci` is empty. |

Here `Args...` are the types deduced for the forwarded arguments.

## Storage

Let `P` be a contained pointer type and `M` be the implementation-defined metadata type generated for a facade and `MP`. Its metadata describes the conventions, reflections, and lifetime operations of that facade and its supers. `M` meets the [*ProBasicMetadata* requirements](ProBasicMetadata.md) and is trivially destructible. Let `m` be an object of type `M`, and `cm` be a reference to `const M`. The library guarantees that the following expressions are well-formed and have the specified semantics.

| Expressions | Semantics |
| ----------- | --------- |
| `M()` | Is non-throwing. Creates an empty object. |
| `M(std::in_place_type<P>)` | Constructs an object holding the metadata deduced from `P`. Throws any exception thrown by constructing an invoker or a reflector of the facade. |
| `m = cm` | Is non-throwing. Replaces the metadata of `m` with the metadata of `cm`, or makes `m` empty if `cm` is empty. |
| `m.reset()` | Is non-throwing. Makes `m` empty. |
| `static_cast<bool>(cm)` | Is non-throwing. Returns `true` if and only if `cm` is not empty. |

`typename MP::template storage<M>` shall denote a type `S` that meets the [*ProBasicMetadata* requirements](ProBasicMetadata.md). Let `s` and `s2` be objects of type `S`, and `cs` be a reference to `const S`.

| Expressions | Requirements and semantics |
| ----------- | -------------------------- |
| `S s;` | Is non-throwing. Default-initializes `s`. Until `s` is reset or assigned, the library applies to it only `s.reset()`, `s.reset(std::in_place_type<P>)`, `s = cs`, `s = cs2`, or its destruction, and each of these shall be valid even if the default initialization left the members of `s` with indeterminate values. |
| `s = cs` | Is non-throwing. Makes `s` hold the metadata `cs` holds, or makes `s` empty if `cs` is empty. |
| `S s(std::move(s2));` | Is non-throwing. Initializes `s` to hold the metadata `s2` held before the initialization. Leaves `s2` in a valid but unspecified state. |
| `s = std::move(s2)` | Is non-throwing. Makes `s` hold the metadata `s2` held before the assignment. Leaves `s2` in a valid but unspecified state. |
| `s.reset()` | Is non-throwing. Makes `s` empty. |
| `s.reset(std::in_place_type<P>)` | Is non-throwing. Makes `s` hold the metadata deduced from `P`, as constructed by `M(std::in_place_type<P>)`. |
| `static_cast<bool>(cs)` | Is non-throwing. Returns `true` if and only if `cs` is not empty. |
| `*cs` | Is non-throwing. Returns a `const M&` referring to the stored metadata. The behavior is undefined if `cs` is empty. |
| `s = cs2` | Is non-throwing. Replaces the metadata of `s` with `static_cast<const M&>(*cs2)`. The behavior is undefined if `cs2` is empty. |

The last row applies only when `cs2` is a reference to `const MP::template storage<M2>`, where `M2` is the metadata type generated for a facade having the facade of `M` as a super, and `M2` differs from `M`. The library guarantees that `const M2&` is non-throwing implicitly convertible to `const M&`, referring to metadata within the `M2` object, and that `sizeof(M) <= sizeof(M2)`.

Stored metadata remains valid until its storage is modified or destroyed. Copying or moving storage, including assignment from another storage type, shall not make the destination depend on the lifetime or subsequent state of the source storage.

## Diagnostics

A `proxy<F, MP>` diagnoses `MP` with a check that depends on `MP` alone. Without the function types and the metadata type of a particular facade, the check can only use representative types: `void(void*) noexcept` for `F`, an implementation-defined metadata type for `M`, and `void*` for `P`. It covers each requirement as follows, and does not check any semantics.

| Requirement | Checked |
| ----------- | ------- |
| `I` is an object type | As part of `v = I()` |
| `I()` | As part of `v = I()` |
| `I i(f);` | Well-formed and non-throwing |
| `I i(ci);` | Well-formed and non-throwing |
| `v = ci` | Well-formed and non-throwing |
| `v = I()` | Well-formed and non-throwing |
| `I` is trivially destructible | Yes |
| `static_cast<bool>(ci)` | Well-formed and non-throwing |
| `ci(std::forward<Args>(args)...)` | Only as `ci(ptr)` with `ptr` of type `void*`, which shall be well-formed, non-throwing, and of type `void` |
| `S` meets the *ProBasicMetadata* requirements | Well-formed and non-throwing |
| `S s;` | Well-formed and non-throwing |
| `s = cs` | Well-formed and non-throwing |
| `S s(std::move(s2));` | Well-formed and non-throwing |
| `s = std::move(s2)` | Well-formed and non-throwing |
| `s.reset()` | Well-formed and non-throwing |
| `s.reset(std::in_place_type<P>)` | Well-formed and non-throwing |
| `static_cast<bool>(cs)` | Well-formed and non-throwing |
| `*cs` | Well-formed, non-throwing, and of type `const M&` |
| `s = cs2` | Not checked, because no super relationship is known |

A metadata policy that passes the check but violates any other requirement results in undefined behavior.

## Notes

The invoker abstracts a function, rather than a pointer type or a facade. The library binds the contained pointer type and dispatch when generating the function. Argument conversions and parameter construction have the same exception behavior as a direct function call, even when `F` is `noexcept`.

The library stores each invoker as a data member of the metadata, so an invoker can be a plain function pointer. Storage types need not support arbitrary user-defined metadata or conversions between unrelated metadata types.

Conversions to a super preserve `MP`. The same policy is also preserved by [`skills::as_view`](skills_as_view.md), [`skills::as_weak`](skills_as_weak.md), and [`weak_proxy::lock`](weak_proxy.md).

## See Also

- [class `compact_metadata`<br />class `inline_metadata`](compact_metadata.md)
- [*ProBasicMetadata* requirements](ProBasicMetadata.md)
- [*ProMetadata* requirements](ProMetadata.md)
