// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_DETAIL_BOX_H_
#define MSFT_PROXY_V4_DETAIL_BOX_H_

#include <initializer_list>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "core.h"
#include "proxy_creation.h"

#if __STDC_HOSTED__
#include <optional>

namespace pro::inline v4 {

template <facade F>
class box;

namespace detail {

struct incomplete;
struct incomplete_ptr : std::unique_ptr<incomplete> {
  incomplete_ptr(incomplete_ptr&&) noexcept;
  incomplete_ptr(const incomplete_ptr&);
};

template <class... Cs>
using lifetime_conv_types =
    composite_t<std::tuple<>,
                std::conditional_t<
                    Cs::is_direct && !std::is_same_v<typename Cs::dispatch_type,
                                                     substitution_dispatch>,
                    Cs, void>...>;
template <class... Rs>
using lifetime_refl_types =
    composite_t<std::tuple<>, std::conditional_t<Rs::is_direct, Rs, void>...>;
template <class F>
struct lifetime_facade
    : facade_impl<
          specialization_t<lifetime_conv_types, typename F::convention_types>,
          specialization_t<lifetime_refl_types, typename F::reflection_types>,
          F::max_size, F::max_align, F::copyability, F::relocatability,
          F::destructibility> {};

template <class F, class... Cs>
struct box_conv_traits_impl {
  using conv_accessor = composite_t<composite_accessor<>,
                                    conv_accessor_t<box<F>, Cs, F, false>...>;
};
template <class F, class... Rs>
struct box_refl_traits_impl {
  using refl_accessor =
      composite_t<composite_accessor<>, refl_accessor_t<box<F>, Rs, false>...>;
};
template <class F>
struct box_traits
    : specialization_t<box_conv_traits_impl, typename F::convention_types, F>,
      specialization_t<box_refl_traits_impl, typename F::reflection_types, F> {
  using accessor = composite_t<typename box_traits::conv_accessor,
                               typename box_traits::refl_accessor>;
};

} // namespace detail

template <>
struct is_bitwise_trivially_relocatable<detail::incomplete_ptr>
    : std::true_type {};

template <facade F>
class box : public detail::box_traits<F>::accessor {
  static_assert(proxiable<detail::incomplete_ptr, detail::lifetime_facade<F>>,
                "facade type not eligible for boxing");

  template <facade F2>
  friend class box;

public:
  using facade_type = F;

  box() noexcept = default;
  box(std::nullopt_t) noexcept {}
  box(const box&) = default;
  box(box&&) = default;
  template <facade F2>
  box(const box<F2>& rhs) noexcept(
      std::is_nothrow_convertible_v<proxy<F>, const proxy<F2>&>)
    requires(std::is_convertible_v<proxy<F>, const proxy<F2>&>)
      : p_(rhs.p_) {}
  template <facade F2>
  box(box<F2>&& rhs) noexcept(std::is_nothrow_convertible_v<proxy<F>, proxy<F2>>)
    requires(std::is_convertible_v<proxy<F>, proxy<F2>>)
      : p_(std::move(rhs.p_)) {}
  template <class T>
  constexpr box(T&& val)
    requires(
        !detail::specialization_of<std::decay_t<T>, box> &&
        !detail::specialization_of<std::decay_t<T>, std::in_place_type_t> &&
        std::is_constructible_v<std::decay_t<T>, T>)
      : p_(std::in_place_type<detail::owned_ptr<F, std::decay_t<T>>>,
           std::allocator<void>{}, std::forward<T>(val)) {}
  template <class T, class... Args>
  constexpr explicit box(std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>)
      : p_(std::in_place_type<detail::owned_ptr<F, T>>, std::allocator<void>{},
           std::forward<Args>(args)...) {}
  template <class T, class U, class... Args>
  constexpr explicit box(std::in_place_type_t<T>, std::initializer_list<U> il,
                         Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
      : p_(std::in_place_type<detail::owned_ptr<F, T>>, std::allocator<void>{},
           il, std::forward<Args>(args)...) {}
  template <class Alloc, class T>
  constexpr explicit box(std::allocator_arg_t, const Alloc& alloc, T&& val)
    requires(
        !detail::specialization_of<std::decay_t<T>, std::in_place_type_t> &&
        std::is_constructible_v<std::decay_t<T>, T>)
      : p_(std::in_place_type<detail::allocated_ptr<F, std::decay_t<T>, Alloc>>,
           alloc, std::forward<T>(val)) {}
  template <class Alloc, class T, class... Args>
  constexpr explicit box(std::allocator_arg_t, const Alloc& alloc,
                         std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>)
      : p_(std::in_place_type<detail::allocated_ptr<F, T, Alloc>>, alloc,
           std::forward<Args>(args)...) {}
  template <class Alloc, class T, class U, class... Args>
  constexpr explicit box(std::allocator_arg_t, const Alloc& alloc,
                         std::in_place_type_t<T>, std::initializer_list<U> il,
                         Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
      : p_(std::in_place_type<detail::allocated_ptr<F, T, Alloc>>, alloc, il,
           std::forward<Args>(args)...) {}
  box& operator=(std::nullopt_t) noexcept(
      std::is_nothrow_assignable_v<proxy<F>, std::nullptr_t>)
    requires(std::is_assignable_v<proxy<F>, std::nullptr_t>)
  {
    p_ = nullptr;
    return *this;
  }
  box& operator=(const box&) = default;
  box& operator=(box&&) = default;
  template <facade F2>
  box& operator=(const box<F2>& rhs) noexcept(
      std::is_nothrow_assignable_v<proxy<F>, const proxy<F2>&>)
    requires(std::is_assignable_v<proxy<F>, const proxy<F2>&>)
  {
    p_ = rhs.p_;
    return *this;
  }
  template <facade F2>
  box& operator=(box<F2>&& rhs) noexcept(
      std::is_nothrow_assignable_v<proxy<F>, proxy<F2>>)
    requires(std::is_assignable_v<proxy<F>, proxy<F2>>)
  {
    p_ = std::move(rhs.p_);
    return *this;
  }
  template <class T>
  constexpr box& operator=(T&& val)
    requires(!detail::specialization_of<std::decay_t<T>, box> &&
             std::is_constructible_v<std::decay_t<T>, T> &&
             std::is_destructible_v<proxy<F>>)
  {
    p_ = make_proxy<F>(std::forward<T>(val));
    return *this;
  }
  ~box() = default;

  bool has_value() const noexcept { return p_.has_value(); }
  explicit operator bool() const noexcept { return p_.has_value(); }
  void reset() noexcept(std::is_nothrow_destructible_v<proxy<F>>)
    requires(std::is_destructible_v<proxy<F>>)
  {
    p_.reset();
  }
  void swap(box& rhs) noexcept(std::is_nothrow_swappable_v<proxy<F>>)
    requires(std::is_swappable_v<proxy<F>>)
  {
    p_.swap(rhs.p_);
  }
  template <class T, class... Args>
  constexpr T& emplace(Args&&... args)
    requires(std::is_constructible_v<T, Args...> &&
             std::is_destructible_v<proxy<F>>)
  {
    return *p_.template emplace<detail::owned_ptr<F, T>>(
        std::allocator<void>{}, std::forward<Args>(args)...);
  }
  template <class T, class U, class... Args>
  constexpr T& emplace(std::initializer_list<U> il, Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
             std::is_destructible_v<proxy<F>>)
  {
    return *p_.template emplace<detail::owned_ptr<F, T>>(
        std::allocator<void>{}, il, std::forward<Args>(args)...);
  }
  template <class T, class Alloc, class... Args>
  constexpr T& emplace_alloc(const Alloc& alloc, Args&&... args)
    requires(std::is_constructible_v<T, Args...> &&
             std::is_destructible_v<proxy<F>>)
  {
    return *p_.template emplace<detail::allocated_ptr<F, T, Alloc>>(
        alloc, std::forward<Args>(args)...);
  }
  template <class T, class Alloc, class U, class... Args>
  constexpr T& emplace_alloc(const Alloc& alloc, std::initializer_list<U> il,
                             Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
             std::is_destructible_v<proxy<F>>)
  {
    return *p_.template emplace<detail::allocated_ptr<F, T, Alloc>>(
        alloc, il, std::forward<Args>(args)...);
  }
  proxy<F> release() noexcept(std::is_nothrow_move_constructible_v<proxy<F>>) {
    return std::move(p_);
  }
  operator proxy_indirect_accessor<F>&() & noexcept { return *p_; }
  operator const proxy_indirect_accessor<F>&() const& noexcept { return *p_; }
  operator proxy_indirect_accessor<F>&&() && noexcept { return *std::move(p_); }
  operator const proxy_indirect_accessor<F>&&() const&& noexcept {
    return *std::move(p_);
  }

  friend void swap(box& lhs,
                   box& rhs) noexcept(std::is_nothrow_swappable_v<proxy<F>>)
    requires(std::is_swappable_v<proxy<F>>)
  {
    lhs.swap(rhs);
  }
  friend bool operator==(const box& lhs, std::nullopt_t) noexcept {
    return !lhs.has_value();
  }
  template <class D, class O, class... Args>
  friend decltype(auto) invoke(box& v, Args&&... args) {
    return invoke<D, O>(*v.p_, std::forward<Args>(args)...);
  }
  template <class D, class O, class... Args>
  friend decltype(auto) invoke(const box& v, Args&&... args) {
    return invoke<D, O>(*v.p_, std::forward<Args>(args)...);
  }
  template <class D, class O, class... Args>
  friend decltype(auto) invoke(box&& v, Args&&... args) {
    return invoke<D, O>(*std::move(v.p_), std::forward<Args>(args)...);
  }
  template <class D, class O, class... Args>
  friend decltype(auto) invoke(const box&& v, Args&&... args) {
    return invoke<D, O>(*std::move(v.p_), std::forward<Args>(args)...);
  }
  template <class R>
  friend const R& reflect(const box& v) noexcept {
    return reflect<R>(*v.p_);
  }

private:
  proxy<F> p_;
};

} // namespace pro::inline v4
#endif // __STDC_HOSTED__

#endif // MSFT_PROXY_V4_DETAIL_BOX_H_
