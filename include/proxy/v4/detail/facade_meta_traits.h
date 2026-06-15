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
//     same way to be no less secure than a real virtual function. The signing
//     intrinsics live directly inside `invoker` (the dispatched function
//     pointer, IA key) and `meta_storage` (the out-of-line v-table pointer, DA
//     key); there is no separate signed-pointer wrapper.
//   - On every other major platform v-tables are not signed (e.g. Linux/AArch64
//     ships only `pac-ret`, which does not change the C++ ABI), so the metadata
//     is plain `constexpr`, trivially copyable, and lives in `.rodata`.
//
// `PRO4D_PAC` selects between them; it is enabled whenever the toolchain
// exposes the pointer-authentication intrinsics (the `<ptrauth.h>` interface,
// signaled by the predefined `__PTRAUTH__` macro -- the portable spelling
// across Apple and upstream Clang; tested with `#ifdef`, as the ABI does not
// promise a value), and may be predefined to `0`/`1` to override. It is left
// defined for downstream code (e.g. tests).
//
// NOTE: this header is not standalone -- detail/core.h `#include`s it (at
// namespace scope, closing/reopening its namespaces) after defining
// `specialization_of` and declaring `reinterpret_invoke` (used as a dependent
// name).

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
#endif // PRO4D_PAC

namespace pro::inline v4::detail {

using ptr_prototype = void* [2];

#if PRO4D_PAC
// With pointer authentication, address-diversified signed members make
// `composite_meta` non-trivially-copyable, because every copy must
// authenticate-and-resign the pointer for its new storage address. Such a meta
// is still safe to embed inline as long as it can be relocated cheaply and
// without throwing, which is the property the small-meta optimization relies
// on. This concept is unused (and therefore false) on non-PAC builds.
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

// `f_` is the dispatched function, signed like an arm64e virtual-function slot
// (IA key, address + type diversity), so the metadata is no less protected than
// a real v-table entry. The empty state is a *signed* null, so copy and call
// are a single unconditional authenticate-and-resign with no null branch; an
// empty slot round-trips through it. The default ctor must establish that
// signed null (not `= default`): copying an empty proxy resigns *all* slots,
// and the non-sentinel ones are initialized only here. `disc_t` is a function
// type whose `ptrauth_type_discriminator` supplies the type-diversity
// discriminator; it encodes the dispatch `D`, the proxy type/qualifier, and the
// call signature. `D` is passed *by value* (not as `D*`) on purpose:
// `ptrauth_type_discriminator` canonicalizes every pointer parameter to a
// single token, so a `D*` would collapse all dispatches to the same
// discriminator and silently lose type diversity. Signing is not
// constant-evaluable, so the ctors are not `constexpr`.
#define PRO4D_DEF_INVOKER(oq, pq, ne, ...)                                     \
  template <class ProP, class D, class R, class... Args>                       \
  struct invoker<ProP, D, R(Args...) oq ne> {                                  \
    using fp_t = R (*)(ProP pq, Args...) ne;                                   \
    using disc_t = R (*)(ProP pq, D, Args...) ne;                              \
    invoker() noexcept                                                         \
        : f_(ptrauth_sign_unauthenticated(static_cast<fp_t>(nullptr),          \
                                          ptrauth_key_function_pointer,        \
                                          schema(&f_))) {}                     \
    template <class P>                                                         \
    explicit invoker(std::in_place_type_t<P>)                                  \
        : f_(ptrauth_auth_and_resign(                                          \
              (+[](ProP pq self, Args... args) ne -> R {                       \
                return reinterpret_invoke<P, D, R>(                            \
                    static_cast<ProP pq>(self), std::forward<Args>(args)...);  \
              }),                                                              \
              ptrauth_key_function_pointer,                                    \
              ptrauth_function_pointer_type_discriminator(fp_t),               \
              ptrauth_key_function_pointer, schema(&f_))) {}                   \
    invoker(const invoker& rhs) noexcept                                       \
        : f_(ptrauth_auth_and_resign(                                          \
              rhs.f_, ptrauth_key_function_pointer, schema(&rhs.f_),           \
              ptrauth_key_function_pointer, schema(&f_))) {}                   \
    invoker& operator=(const invoker& rhs) noexcept {                          \
      f_ = ptrauth_auth_and_resign(rhs.f_, ptrauth_key_function_pointer,       \
                                   schema(&rhs.f_),                            \
                                   ptrauth_key_function_pointer, schema(&f_)); \
      return *this;                                                            \
    }                                                                          \
    template <class... ActualArgs>                                             \
    R operator()(ActualArgs&&... args) const {                                 \
      return ptrauth_auth_and_resign(                                          \
          f_, ptrauth_key_function_pointer, schema(&f_),                       \
          ptrauth_key_function_pointer,                                        \
          ptrauth_function_pointer_type_discriminator(fp_t))(                  \
          std::forward<ActualArgs>(args)...);                                  \
    }                                                                          \
    bool has_value() const noexcept {                                          \
      return ptrauth_strip(f_, ptrauth_key_function_pointer) != nullptr;       \
    }                                                                          \
    void reset() noexcept {                                                    \
      f_ = ptrauth_sign_unauthenticated(static_cast<fp_t>(nullptr),            \
                                        ptrauth_key_function_pointer,          \
                                        schema(&f_));                          \
    }                                                                          \
                                                                               \
  private:                                                                     \
    static ptrauth_extra_data_t schema(const void* addr) noexcept {            \
      return ptrauth_blend_discriminator(addr,                                 \
                                         ptrauth_type_discriminator(disc_t));  \
    }                                                                          \
    fp_t f_;                                                                   \
  }
PRO4D_DEF_OVERLOAD_SPECIALIZATIONS(PRO4D_DEF_INVOKER)
#undef PRO4D_DEF_INVOKER

template <class... Ms>
struct PRO4D_ENFORCE_EBO composite_meta : Ms... {
  composite_meta() = default;
  template <class P>
  explicit composite_meta(std::in_place_type_t<P>)
      : Ms(std::in_place_type<P>)... {}
};

// `ptr_` is the out-of-line v-table pointer, signed like an arm64e v-table
// pointer (DA key). Its type-diversity discriminator is
// `ptrauth_type_discriminator(void(*)(Ms...))`, encoding the v-table's exact
// composition with each member meta `Ms` by value (a `composite_meta<Ms...>*`
// would collapse, as in `invoker`). The empty state is a signed null, so copy
// is a single unconditional resign with no branch (an empty out-of-line meta
// *is* just this null pointer). Manual signing is not constant-evaluable, so
// the ctors are not `constexpr`.
template <class... Ms>
struct meta_storage {
private:
  using value_type = composite_meta<Ms...>;
  using disc_t = void (*)(Ms...);
  static ptrauth_extra_data_t schema(const void* addr) noexcept {
    return ptrauth_blend_discriminator(addr,
                                       ptrauth_type_discriminator(disc_t));
  }

public:
  meta_storage() noexcept
      : ptr_(ptrauth_sign_unauthenticated(
            static_cast<const value_type*>(nullptr),
            ptrauth_key_cxx_vtable_pointer, schema(&ptr_))) {}
  template <class P>
  explicit meta_storage(std::in_place_type_t<P>) noexcept
      : ptr_(ptrauth_sign_unauthenticated(
            &storage<P>, ptrauth_key_cxx_vtable_pointer, schema(&ptr_))) {}
  meta_storage(const meta_storage& rhs) noexcept
      : ptr_(ptrauth_auth_and_resign(
            rhs.ptr_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.ptr_),
            ptrauth_key_cxx_vtable_pointer, schema(&ptr_))) {}
  meta_storage& operator=(const meta_storage& rhs) noexcept {
    ptr_ = ptrauth_auth_and_resign(
        rhs.ptr_, ptrauth_key_cxx_vtable_pointer, schema(&rhs.ptr_),
        ptrauth_key_cxx_vtable_pointer, schema(&ptr_));
    return *this;
  }
  bool has_value() const noexcept {
    return ptrauth_strip(ptr_, ptrauth_key_cxx_vtable_pointer) != nullptr;
  }
  void reset() noexcept {
    ptr_ = ptrauth_sign_unauthenticated(static_cast<const value_type*>(nullptr),
                                        ptrauth_key_cxx_vtable_pointer,
                                        schema(&ptr_));
  }
  template <class M>
  friend const M& get(const meta_storage& self) noexcept {
    return static_cast<const M&>(*ptrauth_auth_data(
        self.ptr_, ptrauth_key_cxx_vtable_pointer, schema(&self.ptr_)));
  }

private:
  const value_type* ptr_;
  // An `inline` variable signed once during static initialization rather than a
  // function-local static: the latter is a Meyers singleton whose thread-safe
  // guard would be re-checked on *every* proxy construction, whereas this is
  // initialized once at startup and `&storage<P>` is then as cheap as the
  // constexpr case (just a constant address).
  template <class P>
  static inline const value_type storage{std::in_place_type<P>};
};

#else // PRO4D_PAC

// `f_` holds the dispatched function as a plain pointer; everything is
// `constexpr` and trivially copyable, so the metadata is a compile-time
// constant.
#define PRO4D_DEF_INVOKER(oq, pq, ne, ...)                                     \
  template <class ProP, class D, class R, class... Args>                       \
  struct invoker<ProP, D, R(Args...) oq ne> {                                  \
    using fp_t = R (*)(ProP pq, Args...) ne;                                   \
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
    bool has_value() const noexcept { return f_ != nullptr; }                  \
    void reset() noexcept { f_ = nullptr; }                                    \
                                                                               \
  private:                                                                     \
    fp_t f_;                                                                   \
  }
PRO4D_DEF_OVERLOAD_SPECIALIZATIONS(PRO4D_DEF_INVOKER)
#undef PRO4D_DEF_INVOKER

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

#endif // PRO4D_PAC

// When the metadata fits in the small buffer and its first member is an
// `invoker`, it is embedded *inline* in the proxy and the leading invoker's
// signed function pointer doubles as the empty-state sentinel, so there is no
// separate out-of-line pointer to chase. Shared by both implementations: the
// emptiness check and reset go through `invoker`, which knows how its `f_` is
// (un)signed.
template <specialization_of<invoker> M1, class... Ms>
  requires(lightweight_meta<composite_meta<M1, Ms...>>)
struct meta_storage<M1, Ms...> : private composite_meta<M1, Ms...> {
  using composite_meta<M1, Ms...>::composite_meta;
  bool has_value() const noexcept {
    return static_cast<const M1&>(*this).has_value();
  }
  void reset() noexcept { static_cast<M1&>(*this).reset(); }
  template <class M>
  friend const M& get(const meta_storage& self) noexcept {
    return static_cast<const M&>(self);
  }
};

} // namespace pro::inline v4::detail

#endif // MSFT_PROXY_V4_DETAIL_FACADE_META_TRAITS_H_
