// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_DETAIL_FACADE_META_TRAITS_H_
#define MSFT_PROXY_V4_DETAIL_FACADE_META_TRAITS_H_

// `proxy`'s hand-rolled dispatch v-table: `invoker` (one dispatched function),
// `composite_meta` (a bundle of invokers/reflection metas), and `meta_storage`
// (the per-proxy metadata, embedded inline when small or referenced out of line
// otherwise). These three types have a *single* implementation; the only thing
// that varies with the platform is how a stored pointer is represented, which
// is isolated in two helpers:
//
//   - `code_ptr` holds a dispatched function pointer; `meta_ptr` holds the
//     out-of-line v-table pointer.
//   - On Apple arm64e the C++ ABI signs both a polymorphic object's v-table
//     pointer and every virtual function pointer in it with ARMv8.3 pointer
//     authentication -- address diversity (the signature depends on where the
//     pointer is stored) plus type diversity (a constant discriminator derived
//     from the type). `code_ptr`/`meta_ptr` sign their pointer the same way, so
//     `proxy`'s metadata is no less secure than a real virtual function.
//   - On every other major platform v-tables are not signed (e.g. Linux/AArch64
//     ships only `pac-ret`, which does not change the C++ ABI), so the helpers
//     are plain pointer wrappers: `constexpr`, trivially copyable, `.rodata`.
//
// `PRO4D_PAC` selects between the two; it is enabled whenever the toolchain
// exposes the pointer-authentication intrinsics (the `<ptrauth.h>` interface,
// signaled by the predefined `__PTRAUTH__` macro -- the portable spelling
// across Apple and upstream Clang; tested with `#ifdef`, as the ABI does not
// promise a value), and may be predefined to `0`/`1` to override. It is left
// defined for downstream code (e.g. tests).
//
// This header is self-contained and is meant to be `#include`d before the rest
// of the library. `invoker` calls `reinterpret_invoke` (defined later, in
// detail/core.h); that is a dependent name resolved by ADL when an `invoker` is
// instantiated, so it needs no prior declaration here.

#include <cstddef>
#include <type_traits>
#include <utility>

#include "../proxy_macros.h"

#ifndef PRO4D_PAC
#ifdef __PTRAUTH__
#define PRO4D_PAC 1
#else
#define PRO4D_PAC 0
#endif // __PTRAUTH__
#endif // PRO4D_PAC

#if PRO4D_PAC
#include <ptrauth.h>
// Signing is not constant-evaluable, so on PAC targets the metadata is built at
// runtime: its constructors are not `constexpr` and the out-of-line v-table is
// a signed-once `inline` variable. Without PAC the metadata is a compile-time
// constant. These two macros are the only spelling difference the shared
// v-table types need; everything else is identical.
#define PRO4D_META_CONSTEXPR
#define PRO4D_META_STORAGE inline const
#else
#define PRO4D_META_CONSTEXPR constexpr
#define PRO4D_META_STORAGE constexpr
#endif // PRO4D_PAC

namespace pro::inline v4::detail {

template <class T, template <class...> class TT>
struct specialization_traits : std::false_type {};
template <template <class...> class TT, class... Args>
struct specialization_traits<TT<Args...>, TT> : std::true_type {};
template <class T, template <class...> class TT>
concept specialization_of = specialization_traits<T, TT>::value;

using ptr_prototype = void* [2];

#if PRO4D_PAC
template <class M>
concept lightweight_meta =
    sizeof(M) <= sizeof(void*) && alignof(M) <= alignof(void*) &&
    std::is_nothrow_default_constructible_v<M> &&
    std::is_nothrow_copy_constructible_v<M> &&
    std::is_nothrow_destructible_v<M>;

// A function pointer signed like an arm64e virtual-function slot (IA key,
// address + type diversity). `F` is the function type, so the stored pointer is
// `F*`. The empty state is a *signed* null, so copy and dereference are a
// single unconditional authenticate-and-resign with no null branch; an empty
// slot round-trips through it. The default ctor must establish that signed null
// (not
// `= default`): copying an empty proxy resigns *all* its slots, and the
// non-sentinel ones are initialized only here. `Disc` is a type whose
// `ptrauth_type_discriminator` supplies the type-diversity discriminator.
template <class F, class Disc>
class code_ptr {
public:
  code_ptr() noexcept
      : value_(ptrauth_sign_unauthenticated(static_cast<F*>(nullptr),
                                            ptrauth_key_function_pointer,
                                            schema(&value_))) {}
  explicit code_ptr(F* value) noexcept
      : value_(ptrauth_auth_and_resign(
            value, ptrauth_key_function_pointer,
            ptrauth_function_pointer_type_discriminator(F*),
            ptrauth_key_function_pointer, schema(&value_))) {}
  code_ptr(const code_ptr& rhs) noexcept
      : value_(ptrauth_auth_and_resign(
            rhs.value_, ptrauth_key_function_pointer, schema(&rhs.value_),
            ptrauth_key_function_pointer, schema(&value_))) {}
  code_ptr& operator=(const code_ptr& rhs) noexcept {
    value_ = ptrauth_auth_and_resign(
        rhs.value_, ptrauth_key_function_pointer, schema(&rhs.value_),
        ptrauth_key_function_pointer, schema(&value_));
    return *this;
  }
  code_ptr& operator=(std::nullptr_t) noexcept {
    value_ = ptrauth_sign_unauthenticated(static_cast<F*>(nullptr),
                                          ptrauth_key_function_pointer,
                                          schema(&value_));
    return *this;
  }
  F& operator*() const noexcept {
    return *ptrauth_auth_and_resign(
        value_, ptrauth_key_function_pointer, schema(&value_),
        ptrauth_key_function_pointer,
        ptrauth_function_pointer_type_discriminator(F*));
  }
  friend bool operator==(const code_ptr& self, std::nullptr_t) noexcept {
    return ptrauth_strip(self.value_, ptrauth_key_function_pointer) == nullptr;
  }

private:
  static ptrauth_extra_data_t schema(const void* addr) noexcept {
    return ptrauth_blend_discriminator(addr, ptrauth_type_discriminator(Disc));
  }
  F* value_;
};

// The out-of-line v-table pointer, signed like an arm64e v-table pointer (DA
// key). Empty state and copy semantics mirror `code_ptr`.
template <class T, class Disc>
class meta_ptr {
public:
  meta_ptr() noexcept
      : value_(ptrauth_sign_unauthenticated(static_cast<const T*>(nullptr),
                                            ptrauth_key_cxx_vtable_pointer,
                                            schema(&value_))) {}
  explicit meta_ptr(const T* value) noexcept
      : value_(ptrauth_sign_unauthenticated(
            value, ptrauth_key_cxx_vtable_pointer, schema(&value_))) {}
  meta_ptr(const meta_ptr& rhs) noexcept
      : value_(ptrauth_auth_and_resign(
            rhs.value_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.value_),
            ptrauth_key_cxx_vtable_pointer, schema(&value_))) {}
  meta_ptr& operator=(const meta_ptr& rhs) noexcept {
    value_ = ptrauth_auth_and_resign(
        rhs.value_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.value_),
        ptrauth_key_cxx_vtable_pointer, schema(&value_));
    return *this;
  }
  meta_ptr& operator=(std::nullptr_t) noexcept {
    value_ = ptrauth_sign_unauthenticated(static_cast<const T*>(nullptr),
                                          ptrauth_key_cxx_vtable_pointer,
                                          schema(&value_));
    return *this;
  }
  const T& operator*() const noexcept {
    return *ptrauth_auth_data(value_, ptrauth_key_cxx_vtable_pointer,
                              schema(&value_));
  }
  friend bool operator==(const meta_ptr& self, std::nullptr_t) noexcept {
    return ptrauth_strip(self.value_, ptrauth_key_cxx_vtable_pointer) ==
           nullptr;
  }

private:
  static ptrauth_extra_data_t schema(const void* addr) noexcept {
    return ptrauth_blend_discriminator(addr, ptrauth_type_discriminator(Disc));
  }
  const T* value_;
};
#else
template <class M>
concept lightweight_meta = sizeof(M) <= sizeof(ptr_prototype) &&
                           alignof(M) <= alignof(ptr_prototype) &&
                           std::is_nothrow_default_constructible_v<M> &&
                           std::is_trivially_copyable_v<M>;

template <class F, class Disc>
using code_ptr = F*;

template <class T, class Disc>
using meta_ptr = const T*;
#endif // PRO4D_PAC

// `f_` holds the dispatched function in a `code_ptr` (reached via `operator*`).
// Its discriminator -- the second `code_ptr` argument -- is a function pointer
// encoding the dispatch `D`, the proxy type/qualifier, and the call signature.
// `D` is passed *by value* (not as `D*`) on purpose:
// `ptrauth_type_discriminator` canonicalizes every pointer parameter to a
// single token, so a `D*` would collapse all dispatches to the same
// discriminator and silently lose type diversity.
template <class ProP, class D, class O>
struct invoker;
#define PRO4D_DEF_INVOKER(oq, pq, ne, ...)                                     \
  template <class ProP, class D, class R, class... Args>                       \
  struct invoker<ProP, D, R(Args...) oq ne> {                                  \
    invoker() = default;                                                       \
    template <class P>                                                         \
    PRO4D_META_CONSTEXPR explicit invoker(std::in_place_type_t<P>)             \
        : f_([](ProP pq self, Args... args) ne -> R {                          \
            return reinterpret_invoke<P, D, R>(static_cast<ProP pq>(self),     \
                                               std::forward<Args>(args)...);   \
          }) {}                                                                \
    template <class... ActualArgs>                                             \
    R operator()(ActualArgs&&... args) const {                                 \
      return (*f_)(std::forward<ActualArgs>(args)...);                         \
    }                                                                          \
    code_ptr<R(ProP pq, Args...) ne, R (*)(ProP pq, D, Args...) ne> f_;        \
  }
PRO4D_DEF_OVERLOAD_SPECIALIZATIONS(PRO4D_DEF_INVOKER)
#undef PRO4D_DEF_INVOKER

template <class... Ms>
struct PRO4D_ENFORCE_EBO composite_meta : Ms... {
  composite_meta() = default;
  template <class P>
  PRO4D_META_CONSTEXPR explicit composite_meta(std::in_place_type_t<P>)
      : Ms(std::in_place_type<P>)... {}
};

// The metadata, either referenced out of line through a signed pointer (the
// primary template) or -- when it is small and its first member is an `invoker`
// -- embedded inline, with that leading invoker's function pointer doubling as
// the empty-state sentinel (the partial specialization).
template <class... Ms>
struct meta_storage {
  meta_storage() = default;
  template <class P>
  explicit meta_storage(std::in_place_type_t<P>) : ptr_(&storage<P>) {}
  bool has_value() const noexcept { return ptr_ != nullptr; }
  void reset() noexcept { ptr_ = nullptr; }
  template <class M>
  friend const M& get(const meta_storage& self) noexcept {
    return static_cast<const M&>(*self.ptr_);
  }

private:
  // The discriminator encodes the v-table's exact composition with each member
  // meta `Ms` by value (a `composite_meta<Ms...>*` would collapse, as in
  // `invoker`).
  meta_ptr<composite_meta<Ms...>, void (*)(Ms...)> ptr_;
  template <class P>
  static PRO4D_META_STORAGE composite_meta<Ms...> storage{
      std::in_place_type<P>};
};
template <specialization_of<invoker> M1, class... Ms>
  requires(lightweight_meta<composite_meta<M1, Ms...>>)
struct meta_storage<M1, Ms...> : private composite_meta<M1, Ms...> {
  using composite_meta<M1, Ms...>::composite_meta;
  bool has_value() const noexcept {
    return static_cast<const M1&>(*this).f_ != nullptr;
  }
  void reset() noexcept { static_cast<M1&>(*this).f_ = nullptr; }
  template <class M>
  friend const M& get(const meta_storage& self) noexcept {
    return static_cast<const M&>(self);
  }
};

} // namespace pro::inline v4::detail

#undef PRO4D_META_CONSTEXPR
#undef PRO4D_META_STORAGE

#endif // MSFT_PROXY_V4_DETAIL_FACADE_META_TRAITS_H_
