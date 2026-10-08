// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V5_DETAIL_METADATA_POLICY_H_
#define MSFT_PROXY_V5_DETAIL_METADATA_POLICY_H_

#include <memory>
#include <type_traits>
#include <utility>

#ifdef __has_feature
#if __has_feature(ptrauth_calls)
#include <ptrauth.h>
#define PRO5D_HAS_PAC
#endif // __has_feature(ptrauth_calls)
#endif // __has_feature

namespace pro::inline v5 {

namespace detail {

#ifdef PRO5D_HAS_PAC
template <class F>
class code_ptr {
public:
  code_ptr() = default;
  explicit code_ptr(F* p) noexcept
      : p_(ptrauth_sign_unauthenticated(
            ptrauth_strip(p, ptrauth_key_function_pointer),
            ptrauth_key_function_pointer, schema())) {}
  code_ptr(const code_ptr& rhs) noexcept { initialize(rhs); }
  code_ptr& operator=(const code_ptr& rhs) noexcept {
    initialize(rhs);
    return *this;
  }
  explicit operator bool() const noexcept { return p_ != nullptr; }
  template <class... Args>
    requires(std::is_invocable_v<F*, Args...>)
  decltype(auto) operator()(Args&&... args) const
      noexcept(std::is_nothrow_invocable_v<F*, Args...>) {
    return ptrauth_auth_function(p_, ptrauth_key_function_pointer,
                                 schema())(std::forward<Args>(args)...);
  }

private:
  void initialize(const code_ptr& rhs) noexcept {
    p_ = rhs.p_ == nullptr
             ? nullptr
             : ptrauth_auth_and_resign(rhs.p_, ptrauth_key_function_pointer,
                                       rhs.schema(),
                                       ptrauth_key_function_pointer, schema());
  }
  ptrauth_extra_data_t schema() const noexcept {
    return ptrauth_blend_discriminator(&p_, ptrauth_type_discriminator(F*));
  }

  F* p_;
};

template <class T>
class meta_ptr {
public:
  meta_ptr() = default;
  meta_ptr(const meta_ptr& rhs) noexcept { initialize(rhs); }
  meta_ptr& operator=(const meta_ptr& rhs) noexcept {
    initialize(rhs);
    return *this;
  }
  meta_ptr& operator=(const T* p) noexcept {
    p_ = ptrauth_sign_unauthenticated(p, ptrauth_key_cxx_vtable_pointer,
                                      schema());
    return *this;
  }
  explicit operator bool() const noexcept { return p_ != nullptr; }
  const T& operator*() const noexcept {
    return *ptrauth_auth_data(p_, ptrauth_key_cxx_vtable_pointer, schema());
  }

private:
  void initialize(const meta_ptr& rhs) noexcept {
    p_ = rhs.p_ == nullptr
             ? nullptr
             : ptrauth_auth_and_resign(
                   rhs.p_, ptrauth_key_cxx_vtable_pointer, rhs.schema(),
                   ptrauth_key_cxx_vtable_pointer, schema());
  }
  ptrauth_extra_data_t schema() const noexcept {
    return ptrauth_blend_discriminator(&p_,
                                       ptrauth_type_discriminator(void (*)(T)));
  }

  const T* p_;
};
#else
template <class F>
using code_ptr = F*;

template <class T>
using meta_ptr = const T*;
#endif // PRO5D_HAS_PAC

template <class M>
struct static_meta_storage {
  template <class M2>
    requires(std::is_nothrow_convertible_v<const M2&, const M&>)
  static_meta_storage& operator=(const static_meta_storage<M2>& rhs) noexcept {
    ptr_ = std::addressof(static_cast<const M&>(*rhs));
    return *this;
  }
  void reset() noexcept { ptr_ = meta_ptr<M>(); }
  template <class P>
  void reset(std::in_place_type_t<P>) noexcept {
    ptr_ = std::addressof(storage<P>.value);
  }
  explicit operator bool() const noexcept { return static_cast<bool>(ptr_); }
  const M& operator*() const noexcept { return *ptr_; }

private:
  meta_ptr<M> ptr_;

  struct holder {
    template <class P>
    constexpr explicit holder(std::in_place_type_t<P> tag) : value(tag) {}

    union {
      M value;
    };
  };

  template <class P>
  static inline const holder storage{std::in_place_type<P>};
};

template <class M>
struct inline_meta_storage {
  template <class M2>
    requires(std::is_nothrow_convertible_v<const M2&, const M&>)
  inline_meta_storage& operator=(const inline_meta_storage<M2>& rhs) noexcept {
    value_ = *rhs;
    return *this;
  }
  template <class M2>
    requires(std::is_nothrow_convertible_v<const M2&, const M&>)
  inline_meta_storage& operator=(const static_meta_storage<M2>& rhs) noexcept {
    value_ = *rhs;
    return *this;
  }
  void reset() noexcept { value_.reset(); }
  template <class P>
  void reset(std::in_place_type_t<P>) noexcept {
    std::destroy_at(std::addressof(value_));
    std::construct_at(std::addressof(value_), std::in_place_type<P>);
  }
  explicit operator bool() const noexcept { return static_cast<bool>(value_); }
  const M& operator*() const noexcept { return value_; }

private:
  M value_;
};

} // namespace detail

struct compact_metadata {
  template <class F>
  using invoker = detail::code_ptr<F>;

  template <class M>
  using storage = std::conditional_t<sizeof(M) <= sizeof(void*),
                                     detail::inline_meta_storage<M>,
                                     detail::static_meta_storage<M>>;
};

struct inline_metadata {
  template <class F>
  using invoker = detail::code_ptr<F>;

  template <class M>
  using storage = detail::inline_meta_storage<M>;
};

} // namespace pro::inline v5

#endif // MSFT_PROXY_V5_DETAIL_METADATA_POLICY_H_
