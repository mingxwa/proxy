// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V5_BOX_H_
#define MSFT_PROXY_V5_BOX_H_

#include "proxy.h"

#if __STDC_HOSTED__

#include <initializer_list>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace pro::inline v5 {

template <facade F, class MP = compact_metadata>
class box;
template <facade F, class MP = compact_metadata>
class box_view;

namespace detail {

struct box_helper {
  template <class B>
  static decltype(auto) get_proxy(B&& b) noexcept {
    return (std::forward<B>(b).p_);
  }
};

template <class Self, class F, class MP>
using hosted_accessor_t =
    specialization_t<accessors_t,
                     typename proxy_traits<F, MP>::indirect_access_groups, Self,
                     F, MP>;

template <class T>
concept box_value = !specialization_of<std::decay_t<T>, box> &&
                    !specialization_of<std::decay_t<T>, box_view> &&
                    !specialization_of<std::decay_t<T>, std::in_place_type_t> &&
                    std::is_constructible_v<std::decay_t<T>, T>;

} // namespace detail

template <facade F, class MP>
class box : public detail::hosted_accessor_t<box<F, MP>, F, MP> {
  friend struct detail::box_helper;

public:
  using facade_type = F;
  using metadata_policy_type = MP;

  box() noexcept = default;
  box(std::nullopt_t) noexcept {}
  box(const box&) = default;
  box(box&&) = default;
  template <facade F2>
  box(const box<F2, MP>& rhs) noexcept(
      std::is_nothrow_convertible_v<const proxy<F2, MP>&, proxy<F, MP>>)
    requires(std::is_convertible_v<const proxy<F2, MP>&, proxy<F, MP>>)
      : p_(detail::box_helper::get_proxy(rhs)) {}
  template <facade F2>
  box(box<F2, MP>&& rhs) noexcept(
      std::is_nothrow_convertible_v<proxy<F2, MP>, proxy<F, MP>>)
    requires(std::is_convertible_v<proxy<F2, MP>, proxy<F, MP>>)
      : p_(detail::box_helper::get_proxy(std::move(rhs))) {}
  template <class T>
  constexpr box(T&& val)
    requires(detail::box_value<T>)
      : p_(std::in_place_type<detail::owned_ptr<F, MP, std::decay_t<T>>>,
           std::allocator<void>{}, std::forward<T>(val)) {}
  template <class T, class... Args>
  constexpr explicit box(std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>)
      : p_(std::in_place_type<detail::owned_ptr<F, MP, T>>,
           std::allocator<void>{}, std::forward<Args>(args)...) {}
  template <class T, class U, class... Args>
  constexpr explicit box(std::in_place_type_t<T>, std::initializer_list<U> il,
                         Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
      : p_(std::in_place_type<detail::owned_ptr<F, MP, T>>,
           std::allocator<void>{}, il, std::forward<Args>(args)...) {}
  template <class Alloc, class T>
  constexpr explicit box(std::allocator_arg_t, const Alloc& alloc, T&& val)
    requires(
        !detail::specialization_of<std::decay_t<T>, std::in_place_type_t> &&
        std::is_constructible_v<std::decay_t<T>, T>)
      : p_(std::in_place_type<
               detail::allocated_ptr<F, MP, std::decay_t<T>, Alloc>>,
           alloc, std::forward<T>(val)) {}
  template <class Alloc, class T, class... Args>
  constexpr explicit box(std::allocator_arg_t, const Alloc& alloc,
                         std::in_place_type_t<T>, Args&&... args)
    requires(std::is_constructible_v<T, Args...>)
      : p_(std::in_place_type<detail::allocated_ptr<F, MP, T, Alloc>>, alloc,
           std::forward<Args>(args)...) {}
  template <class Alloc, class T, class U, class... Args>
  constexpr explicit box(std::allocator_arg_t, const Alloc& alloc,
                         std::in_place_type_t<T>, std::initializer_list<U> il,
                         Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...>)
      : p_(std::in_place_type<detail::allocated_ptr<F, MP, T, Alloc>>, alloc,
           il, std::forward<Args>(args)...) {}
  box& operator=(std::nullopt_t) noexcept(
      std::is_nothrow_assignable_v<proxy<F, MP>&, std::nullptr_t>)
    requires(std::is_assignable_v<proxy<F, MP>&, std::nullptr_t>)
  {
    p_ = nullptr;
    return *this;
  }
  box& operator=(const box&) = default;
  box& operator=(box&&) = default;
  template <facade F2>
  box& operator=(const box<F2, MP>& rhs) noexcept(
      std::is_nothrow_assignable_v<proxy<F, MP>&, const proxy<F2, MP>&>)
    requires(std::is_assignable_v<proxy<F, MP>&, const proxy<F2, MP>&>)
  {
    p_ = detail::box_helper::get_proxy(rhs);
    return *this;
  }
  template <facade F2>
  box& operator=(box<F2, MP>&& rhs) noexcept(
      std::is_nothrow_assignable_v<proxy<F, MP>&, proxy<F2, MP>>)
    requires(std::is_assignable_v<proxy<F, MP>&, proxy<F2, MP>>)
  {
    p_ = detail::box_helper::get_proxy(std::move(rhs));
    return *this;
  }
  template <class T>
  constexpr box& operator=(T&& val)
    requires(detail::box_value<T> &&
             std::is_assignable_v<proxy<F, MP>&, proxy<F, MP>>)
  {
    p_ = proxy<F, MP>{
        std::in_place_type<detail::owned_ptr<F, MP, std::decay_t<T>>>,
        std::allocator<void>{}, std::forward<T>(val)};
    return *this;
  }
  ~box() = default;

  bool has_value() const noexcept { return p_.has_value(); }
  explicit operator bool() const noexcept { return p_.has_value(); }
  void reset() noexcept(std::is_nothrow_destructible_v<proxy<F, MP>>)
    requires(std::is_destructible_v<proxy<F, MP>>)
  {
    p_.reset();
  }
  void swap(box& rhs) noexcept(std::is_nothrow_swappable_v<proxy<F, MP>>)
    requires(std::is_swappable_v<proxy<F, MP>>)
  {
    p_.swap(rhs.p_);
  }
  template <class T, class... Args>
  constexpr T& emplace(Args&&... args)
    requires(std::is_constructible_v<T, Args...> &&
             std::is_destructible_v<proxy<F, MP>>)
  {
    return *p_.template emplace<detail::owned_ptr<F, MP, T>>(
        std::allocator<void>{}, std::forward<Args>(args)...);
  }
  template <class T, class U, class... Args>
  constexpr T& emplace(std::initializer_list<U> il, Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
             std::is_destructible_v<proxy<F, MP>>)
  {
    return *p_.template emplace<detail::owned_ptr<F, MP, T>>(
        std::allocator<void>{}, il, std::forward<Args>(args)...);
  }
  template <class T, class Alloc, class... Args>
  constexpr T& emplace_alloc(const Alloc& alloc, Args&&... args)
    requires(std::is_constructible_v<T, Args...> &&
             std::is_destructible_v<proxy<F, MP>>)
  {
    return *p_.template emplace<detail::allocated_ptr<F, MP, T, Alloc>>(
        alloc, std::forward<Args>(args)...);
  }
  template <class T, class Alloc, class U, class... Args>
  constexpr T& emplace_alloc(const Alloc& alloc, std::initializer_list<U> il,
                             Args&&... args)
    requires(std::is_constructible_v<T, std::initializer_list<U>&, Args...> &&
             std::is_destructible_v<proxy<F, MP>>)
  {
    return *p_.template emplace<detail::allocated_ptr<F, MP, T, Alloc>>(
        alloc, il, std::forward<Args>(args)...);
  }
  proxy<F, MP>
      release() noexcept(std::is_nothrow_move_constructible_v<proxy<F, MP>> &&
                         std::is_nothrow_destructible_v<proxy<F, MP>>)
    requires(std::is_move_constructible_v<proxy<F, MP>> &&
             std::is_destructible_v<proxy<F, MP>>)
  {
    if constexpr (F::copyability == lifetime_requirement::trivial) {
      proxy<F, MP> result = p_;
      p_.reset();
      return result;
    } else {
      return std::move(p_);
    }
  }
  operator proxy_indirect_accessor<F, MP>&() & noexcept { return *p_; }
  operator const proxy_indirect_accessor<F, MP>&() const& noexcept {
    return *p_;
  }
  operator proxy_indirect_accessor<F, MP>&&() && noexcept {
    return *std::move(p_);
  }
  operator const proxy_indirect_accessor<F, MP>&&() const&& noexcept {
    return *std::move(p_);
  }

  friend void swap(box& lhs,
                   box& rhs) noexcept(std::is_nothrow_swappable_v<proxy<F, MP>>)
    requires(std::is_swappable_v<proxy<F, MP>>)
  {
    lhs.swap(rhs);
  }
  friend bool operator==(const box& lhs, std::nullopt_t) noexcept {
    return !lhs.has_value();
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(box& b, Args&&... args) {
    return invoke<D, S>(*b.p_, std::forward<Args>(args)...);
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(const box& b, Args&&... args) {
    return invoke<D, S>(*b.p_, std::forward<Args>(args)...);
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(box&& b, Args&&... args) {
    return invoke<D, S>(*std::move(b.p_), std::forward<Args>(args)...);
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(const box&& b, Args&&... args) {
    return invoke<D, S>(*std::move(b.p_), std::forward<Args>(args)...);
  }
  template <class M>
  friend const M& reflect(const box& b) noexcept {
    return reflect<M>(*b.p_);
  }

private:
  proxy<F, MP> p_;
};

template <facade F, class MP>
class box_view
    : public detail::hosted_accessor_t<box_view<F, MP>, view_facade<F>, MP> {
  friend struct detail::box_helper;

public:
  using facade_type = F;
  using metadata_policy_type = MP;

  box_view() noexcept = default;
  box_view(std::nullopt_t) noexcept {}
  box_view(const box_view&) = default;
  template <facade F2>
  box_view(box<F2, MP>& b) noexcept
    requires(std::is_convertible_v<proxy<F2, MP>&, proxy_view<F, MP>>)
      : p_(detail::box_helper::get_proxy(b)) {}
  template <facade F2>
  box_view(const box_view<F2, MP>& rhs) noexcept
    requires(
        !std::is_same_v<F, F2> &&
        std::is_convertible_v<const proxy_view<F2, MP>&, proxy_view<F, MP>>)
      : p_(detail::box_helper::get_proxy(rhs)) {}
  box_view& operator=(const box_view&) = default;
  ~box_view() = default;

  bool has_value() const noexcept { return p_.has_value(); }
  explicit operator bool() const noexcept { return p_.has_value(); }
  void reset() noexcept { p_.reset(); }
  void swap(box_view& rhs) noexcept { p_.swap(rhs.p_); }
  operator proxy_indirect_accessor<view_facade<F>, MP>&() & noexcept {
    return *p_;
  }
  operator const proxy_indirect_accessor<view_facade<F>, MP>&()
      const& noexcept {
    return *p_;
  }
  operator proxy_indirect_accessor<view_facade<F>, MP>&&() && noexcept {
    return *std::move(p_);
  }
  operator const proxy_indirect_accessor<view_facade<F>, MP>&&()
      const&& noexcept {
    return *std::move(p_);
  }

  friend bool operator==(const box_view& lhs, std::nullopt_t) noexcept {
    return !lhs.has_value();
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(box_view& v, Args&&... args) {
    return invoke<D, S>(*v.p_, std::forward<Args>(args)...);
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(const box_view& v, Args&&... args) {
    return invoke<D, S>(*v.p_, std::forward<Args>(args)...);
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(box_view&& v, Args&&... args) {
    return invoke<D, S>(*std::move(v.p_), std::forward<Args>(args)...);
  }
  template <class D, class S, class... Args>
  friend detail::ret_t<S> invoke(const box_view&& v, Args&&... args) {
    return invoke<D, S>(*std::move(v.p_), std::forward<Args>(args)...);
  }
  template <class M>
  friend const M& reflect(const box_view& v) noexcept {
    return reflect<M>(*v.p_);
  }

private:
  proxy_view<F, MP> p_;
};

} // namespace pro::inline v5

#endif // __STDC_HOSTED__

#endif // MSFT_PROXY_V5_BOX_H_
