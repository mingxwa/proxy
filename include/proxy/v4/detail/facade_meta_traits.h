// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_DETAIL_FACADE_META_TRAITS_H_
#define MSFT_PROXY_V4_DETAIL_FACADE_META_TRAITS_H_

// `proxy`'s hand-rolled dispatch v-table: `invoker` (one dispatched function),
// `composite_meta` (a bundle of invokers/reflection metas), and `meta_storage`
// (the per-proxy metadata, embedded inline when small or referenced out of line
// otherwise). This header gives two complete implementations of these types:
//
//   - On Apple arm64e the C++ ABI signs both the v-table pointer of a
//     polymorphic object and every virtual function pointer in the v-table with
//     ARMv8.3 pointer authentication, using address diversity (the signature
//     depends on where the pointer is stored) and type diversity (a constant
//     discriminator derived from the type). `proxy`'s metadata is signed the
//     same way to be no less secure than a real virtual function.
//   - On every other major platform v-tables are not signed (e.g. Linux/AArch64
//     ships only `pac-ret`, which does not change the C++ ABI), so the metadata
//     is plain `constexpr`, trivially copyable, and lives in `.rodata`.
//
// `PRO4D_PAC` selects between them; it is enabled exactly when the toolchain
// signs code pointers in the ABI (Clang's `ptrauth_calls`) and exposes the
// `__ptrauth` qualifier (`ptrauth_qualifier`), and may be predefined to `0`/`1`
// to override. It is left defined for downstream code (e.g. tests).
//
// NOTE: this header is not standalone -- proxy.h `#include`s it (at namespace
// scope, closing/reopening its namespaces) after defining `specialization_of`
// and declaring `reinterpret_invoke` (used as a dependent name).

#include <cstddef>
#include <type_traits>
#include <utility>

#include "../proxy_macros.h"

#ifndef PRO4D_PAC
#if defined(__has_feature)
#if __has_feature(ptrauth_qualifier) && __has_feature(ptrauth_calls)
#define PRO4D_PAC 1
#endif // __has_feature(ptrauth_qualifier) && __has_feature(ptrauth_calls)
#endif // defined(__has_feature)
#ifndef PRO4D_PAC
#define PRO4D_PAC 0
#endif // PRO4D_PAC
#endif // PRO4D_PAC

#if PRO4D_PAC
#include <ptrauth.h>
#endif // PRO4D_PAC

namespace pro::inline v4::details {

using ptr_prototype = void* [2];

#if PRO4D_PAC
// With pointer authentication, address-diversified signed members make
// `composite_meta` non-trivially-copyable, because every copy must
// authenticate-and-resign the pointer for its new storage address. Such a meta
// is still safe to embed inline as long as it can be relocated cheaply and
// without throwing, which is the property the small-meta optimization relies on.
// This concept is unused (and therefore false) on non-PAC builds.
template <class M>
concept pac_relocatable_meta =
    std::is_nothrow_copy_constructible_v<M> &&
    std::is_nothrow_move_constructible_v<M> &&
    std::is_nothrow_copy_assignable_v<M> &&
    std::is_nothrow_move_assignable_v<M> && std::is_trivially_destructible_v<M>;
#else
template <class M>
concept pac_relocatable_meta = false;
#endif // PRO4D_PAC
template <class M>
concept lightweight_meta =
    sizeof(M) <= sizeof(ptr_prototype) &&
    alignof(M) <= alignof(ptr_prototype) &&
    std::is_nothrow_default_constructible_v<M> &&
    (std::is_trivially_copyable_v<M> || pac_relocatable_meta<M>);

template <class ProP, class D, class O>
struct invoker;

#if PRO4D_PAC

// A non-zero 16-bit discriminator unique to the type `T` (type diversity). This
// is what an arm64e v-table uses (`ptrauth_string_discriminator` of the mangled
// function name); we cannot reach that builtin because it needs a string
// *literal*, and `ptrauth_type_discriminator` is useless here -- the default
// arm64e ABI disables function-pointer type discrimination, so it returns the
// same constant for every function type. Instead FNV-hash the spelling of `T`
// (extracted from __PRETTY_FUNCTION__, the way magic_enum/nameof derive type
// identity); the result feeds the runtime ptrauth intrinsics, which (unlike the
// `__ptrauth` qualifier) accept a value-dependent discriminator. A missing
// marker is a hard error (a `throw` in this `consteval` body) rather than a
// silent diversity loss; exact values are pinned by proxy_pac_tests.cpp.
template <class T>
consteval ptrauth_extra_data_t pac_type_disc() {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpedantic" // __PRETTY_FUNCTION__
#endif
  const char* const sig = __PRETTY_FUNCTION__;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
  const char* begin = sig;
  const char* end = sig;
  for (const char* p = sig; *p != '\0'; ++p) {
    if (p[0] == '=' && p[1] == ' ') {
      begin = p + 2; // start of "<type>" (last "= ")
    }
    if (*p == ']') {
      end = p; // last ']' closes "[T = <type>]"
    }
  }
  if (begin >= end) {
    throw "pac_type_disc: could not extract the type name from "
          "__PRETTY_FUNCTION__; the compiler's format is unexpected";
  }
  unsigned long long hash = 14695981039346656037ull; // FNV-1a, 64-bit
  for (const char* p = begin; p < end; ++p) {
    hash = (hash ^ static_cast<unsigned char>(*p)) * 1099511628211ull;
  }
  // Map into [1, 65535]: a 16-bit discriminator of 0 means "no discriminator"
  // (no type diversity) in the ptrauth ABI, so it must be non-zero -- the same
  // range and mapping `ptrauth_string_discriminator` itself uses.
  return static_cast<ptrauth_extra_data_t>(hash % 65535u) + 1u;
}

// Internal helper: a function pointer signed like an arm64e virtual-function
// slot (IA key, address + type diversity). The empty state is a *signed* null,
// so copy/get are a single unconditional authenticate-and-resign with no null
// branch; an empty slot round-trips through it. The default ctor must establish
// the signed null (not `= default`): copying an empty proxy resigns *all* slots,
// and the non-sentinel ones are initialized only here.
template <class FP, class Disc>
class code_ptr {
public:
  code_ptr() noexcept
      : value_(ptrauth_sign_unauthenticated(
            static_cast<FP>(nullptr), ptrauth_key_function_pointer,
            schema(&value_))) {}
  code_ptr(FP fp) noexcept
      : value_(ptrauth_auth_and_resign(
            fp, ptrauth_key_function_pointer,
            ptrauth_function_pointer_type_discriminator(FP),
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
    value_ = ptrauth_sign_unauthenticated(
        static_cast<FP>(nullptr), ptrauth_key_function_pointer, schema(&value_));
    return *this;
  }
  FP get() const noexcept {
    return ptrauth_auth_and_resign(
        value_, ptrauth_key_function_pointer, schema(&value_),
        ptrauth_key_function_pointer,
        ptrauth_function_pointer_type_discriminator(FP));
  }
  friend bool operator==(const code_ptr& self, std::nullptr_t) noexcept {
    return ptrauth_strip(self.value_, ptrauth_key_function_pointer) == nullptr;
  }

private:
  static ptrauth_extra_data_t schema(const void* addr) noexcept {
    return ptrauth_blend_discriminator(addr, pac_type_disc<Disc>());
  }
  FP value_;
};

// Internal helper: a pointer signed like an arm64e v-table pointer (DA key);
// empty state and default ctor follow `code_ptr`.
template <class T, class Disc>
class data_ptr {
public:
  data_ptr() noexcept
      : value_(ptrauth_sign_unauthenticated(
            static_cast<const T*>(nullptr), ptrauth_key_cxx_vtable_pointer,
            schema(&value_))) {}
  data_ptr(const T* p) noexcept
      : value_(ptrauth_sign_unauthenticated(p, ptrauth_key_cxx_vtable_pointer,
                                            schema(&value_))) {}
  data_ptr(const data_ptr& rhs) noexcept
      : value_(ptrauth_auth_and_resign(
            rhs.value_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.value_),
            ptrauth_key_cxx_vtable_pointer, schema(&value_))) {}
  data_ptr& operator=(const data_ptr& rhs) noexcept {
    value_ = ptrauth_auth_and_resign(
        rhs.value_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.value_),
        ptrauth_key_cxx_vtable_pointer, schema(&value_));
    return *this;
  }
  data_ptr& operator=(std::nullptr_t) noexcept {
    value_ = ptrauth_sign_unauthenticated(
        static_cast<const T*>(nullptr), ptrauth_key_cxx_vtable_pointer,
        schema(&value_));
    return *this;
  }
  const T& operator*() const noexcept {
    return *ptrauth_auth_data(value_, ptrauth_key_cxx_vtable_pointer,
                              schema(&value_));
  }
  friend bool operator==(const data_ptr& self, std::nullptr_t) noexcept {
    return ptrauth_strip(self.value_, ptrauth_key_cxx_vtable_pointer) ==
           nullptr;
  }

private:
  static ptrauth_extra_data_t schema(const void* addr) noexcept {
    return ptrauth_blend_discriminator(addr, pac_type_disc<Disc>());
  }
  const T* value_;
};

// `f_` holds the dispatched function in a signed `code_ptr` (reached via
// `.get()`); `disc_t` is the function type supplying type diversity by encoding
// the operand, dispatch, and signature. Signing is not constant-evaluable, so
// the constructor is not `constexpr`.
#define PROD_DEF_INVOKER(oq, pq, ne, ...)                                      \
  template <class ProP, class D, class R, class... Args>                       \
  struct invoker<ProP, D, R(Args...) oq ne> {                                  \
    using fp_t = R (*)(ProP pq, Args...) ne;                                    \
    using disc_t = R (*)(D*, ProP pq, Args...) ne;                              \
    invoker() = default;                                                       \
    template <class P>                                                         \
    explicit invoker(std::in_place_type_t<P>)                                  \
        : f_([](ProP pq self, Args... args) ne -> R {                          \
            return reinterpret_invoke<P, D, R>(static_cast<ProP pq>(self),     \
                                               std::forward<Args>(args)...);   \
          }) {}                                                                \
    template <class... ActualArgs>                                             \
    R operator()(ActualArgs&&... args) const {                                 \
      return f_.get()(std::forward<ActualArgs>(args)...);                       \
    }                                                                          \
    code_ptr<fp_t, disc_t> f_;                                                  \
  }
PRO4D_DEF_OVERLOAD_SPECIALIZATIONS(PROD_DEF_INVOKER)
#undef PROD_DEF_INVOKER

template <class... Ms>
struct PRO4D_ENFORCE_EBO composite_meta : Ms... {
  composite_meta() = default;
  template <class P>
  explicit composite_meta(std::in_place_type_t<P>)
      : Ms(std::in_place_type<P>)... {}
};

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
  data_ptr<composite_meta<Ms...>, void (*)(composite_meta<Ms...>*)> ptr_;
  // Manual signing is not constant-evaluable, so the v-table cannot be a
  // constexpr object. Make it an `inline` variable signed once during static
  // initialization rather than a function-local static: the latter is a Meyers
  // singleton whose thread-safe guard would be re-checked on *every* proxy
  // construction, whereas this is initialized once at startup and `&storage<P>`
  // is then as cheap as the constexpr case (just a constant address).
  template <class P>
  static inline const composite_meta<Ms...> storage{std::in_place_type<P>};
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

#else // PRO4D_PAC

// `f_` holds the dispatched function as a plain pointer; everything is
// `constexpr` and trivially copyable, so the metadata is a compile-time constant.
#define PROD_DEF_INVOKER(oq, pq, ne, ...)                                      \
  template <class ProP, class D, class R, class... Args>                       \
  struct invoker<ProP, D, R(Args...) oq ne> {                                  \
    using fp_t = R (*)(ProP pq, Args...) ne;                                    \
    invoker() = default;                                                       \
    template <class P>                                                         \
    constexpr explicit invoker(std::in_place_type_t<P>)                        \
        : f_([](ProP pq self, Args... args) ne -> R {                          \
            return reinterpret_invoke<P, D, R>(static_cast<ProP pq>(self),     \
                                               std::forward<Args>(args)...);   \
          }) {}                                                                \
    template <class... ActualArgs>                                             \
    R operator()(ActualArgs&&... args) const {                                 \
      return f_(std::forward<ActualArgs>(args)...);                            \
    }                                                                          \
    fp_t f_;                                                                    \
  }
PRO4D_DEF_OVERLOAD_SPECIALIZATIONS(PROD_DEF_INVOKER)
#undef PROD_DEF_INVOKER

template <class... Ms>
struct PRO4D_ENFORCE_EBO composite_meta : Ms... {
  composite_meta() = default;
  template <class P>
  constexpr explicit composite_meta(std::in_place_type_t<P>)
      : Ms(std::in_place_type<P>)... {}
};

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
  const composite_meta<Ms...>* ptr_;
  template <class P>
  static constexpr composite_meta<Ms...> storage{std::in_place_type<P>};
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

#endif // PRO4D_PAC

} // namespace pro::inline v4::details

#endif // MSFT_PROXY_V4_DETAIL_FACADE_META_TRAITS_H_
