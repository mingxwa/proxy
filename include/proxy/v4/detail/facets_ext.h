// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#ifndef MSFT_PROXY_V4_DETAIL_FACETS_EXT_H_
#define MSFT_PROXY_V4_DETAIL_FACETS_EXT_H_

#include <cstddef>
#include <type_traits>
#include <utility>

#if __STDC_HOSTED__
#include <functional>
#include <iterator>
#include <ostream>
#endif // __STDC_HOSTED__

#if __cpp_rtti >= 199711L
#include <typeinfo>
#endif // __cpp_rtti >= 199711L

#include "core.h"
#include "dispatch.h"
#include "facade_creation.h"
#include "proxy_creation.h"
#include "skills.h"

namespace pro::inline v4 {

namespace detail {

#if __cpp_rtti >= 199711L
template <class U, class Self>
using rtti_target_t = std::conditional_t<std::is_const_v<Self>, const U*, U*>;

// A direct reflection already records the contained pointer type, and a direct
// proxy stores that pointer in its own storage, so the target is reachable
// without asking the contained value anything. The whole of direct RTTI is
// therefore a comparison against the recorded type_info, and needs no
// convention of its own.
template <class U, class R, class Self>
rtti_target_t<U, Self> rtti_target(Self& self) noexcept {
  if (typeid(U) == *reflect<R>(self).info) [[likely]] {
    return std::launder(
        static_cast<rtti_target_t<U, Self>>(proxy_helper::get_ptr(self)));
  }
  return nullptr;
}
template <class T, class R, qualifier_type Q, class Self>
T rtti_cast(Self& self) {
  static_assert(!std::is_rvalue_reference_v<T>);
  using U = std::decay_t<T>;
  // An rvalue cast consumes the contained value, so a reference to it would
  // dangle by the time the caller sees it.
  if constexpr (std::is_constructible_v<T, add_qualifier_t<U, Q>> &&
                (Q != qualifier_type::rv || !std::is_lvalue_reference_v<T>)) {
    auto* target = rtti_target<U, R>(self);
    if (target != nullptr) [[likely]] {
      if constexpr (Q == qualifier_type::rv) {
        destroying_guard<U> value_guard{target};
        proxy_helper::meta_resetting_guard<typename Self::facade_type>
            meta_guard{self};
        return static_cast<T>(std::move(*target));
      } else {
        return static_cast<T>(*target);
      }
    }
  }
  PRO4D_THROW(bad_proxy_cast{});
}

struct direct_rtti_reflector {
  direct_rtti_reflector() = default;
  template <class P>
  constexpr explicit direct_rtti_reflector(std::in_place_type_t<P>) noexcept
      : info(&typeid(P)) {}

  template <class Self, class R>
  struct accessor {
    friend const std::type_info& proxy_typeid(const Self& self) noexcept {
      return *reflect<R>(self).info;
    }
    template <class T>
    friend T proxy_cast(Self& self) {
      return rtti_cast<T, R, qualifier_type::lv>(self);
    }
    template <class T>
    friend T proxy_cast(const Self& self) {
      return rtti_cast<T, R, qualifier_type::const_lv>(self);
    }
    template <class T>
    friend T proxy_cast(Self&& self) {
      return rtti_cast<T, R, qualifier_type::rv>(self);
    }
    template <class T>
    friend T* proxy_cast(Self* self) noexcept {
      return rtti_target<std::remove_cv_t<T>, R>(*self);
    }
    template <class T>
    friend const T* proxy_cast(const Self* self) noexcept {
      return rtti_target<std::remove_cv_t<T>, R>(*self);
    }
    PRO4D_DEBUG(
        accessor() noexcept { std::ignore = &pro_symbol_guard; }

        private : static inline const std::type_info& pro_symbol_guard(
            const Self& self) { return proxy_typeid(self); })
  };

  const std::type_info* info;
};
#endif // __cpp_rtti >= 199711L

#if __STDC_HOSTED__
using hash_overload = std::size_t() const noexcept;

struct hash_dispatch {
  template <class T>
  PRO4D_STATIC_CALL(std::size_t, const T& self) noexcept
    requires(std::is_nothrow_default_constructible_v<std::hash<T>> &&
             std::is_nothrow_invocable_r_v<std::size_t, const std::hash<T>&,
                                           const T&>)
  {
    return std::hash<T>{}(self);
  }

  template <class P, class D, class... Os>
  struct accessor {
    accessor() = delete;
  };
  template <class P, class D>
  struct accessor<P, D, hash_overload> : enabled_t<std::hash> {};
};
template <class T>
struct hash_impl {
  std::size_t operator()(const T& self) const noexcept {
    return invoke<hash_dispatch, hash_overload>(self);
  }
};

PRO4_DEF_MEM_DISPATCH(range_cursor_deref_dispatch, pro_deref);
PRO4_DEF_MEM_DISPATCH(range_cursor_next_dispatch, pro_next);
PRO4_DEF_MEM_DISPATCH(range_cursor_done_dispatch, pro_done);

// An iterator and the sentinel it is compared against are erased together, so
// the erased iterator only ever has to answer "am I done?" and never has to
// compare two erased values against each other.
template <class I, class S>
class range_cursor {
public:
  range_cursor(I first, S last)
      : it_(std::move(first)), end_(std::move(last)) {}

  decltype(auto) pro_deref() const { return *it_; }
  void pro_next() { ++it_; }
  bool pro_done() const { return it_ == end_; }

private:
  I it_;
  S end_;
};

template <class Ref>
struct erased_cursor_facade
    : make_facade<
          facets::indirect_convention<range_cursor_deref_dispatch, Ref() const>,
          facets::indirect_convention<range_cursor_next_dispatch, void()>,
          facets::indirect_convention<range_cursor_done_dispatch, bool() const>,
          facets::copyability<constraint_level::nontrivial>,
          facets::relocatability<constraint_level::nontrivial>> {};

template <class Ref>
class erased_iterator {
  using cursor_facade = erased_cursor_facade<Ref>;

public:
  using value_type = std::remove_cvref_t<Ref>;
  using difference_type = std::ptrdiff_t;
  using iterator_concept = std::input_iterator_tag;

  erased_iterator() = default;
  template <class I, class S>
    requires(
        proxiable<owned_ptr<cursor_facade, range_cursor<I, S>>, cursor_facade>)
  erased_iterator(I first, S last)
      : impl_(make_proxy<cursor_facade, range_cursor<I, S>>(std::move(first),
                                                            std::move(last))) {}

  Ref operator*() const { return impl_->pro_deref(); }
  erased_iterator& operator++() {
    impl_->pro_next();
    return *this;
  }
  void operator++(int) { ++*this; }
  bool operator==(std::default_sentinel_t) const {
    return !impl_.has_value() || impl_->pro_done();
  }

private:
  proxy<cursor_facade> impl_;
};

// A mutable reference type can only be produced from a non-const operand; every
// other reference type is also reachable through one, so the convention is
// const-qualified whenever it can be.
template <class Ref>
using range_begin_overload_t =
    std::conditional_t<std::is_lvalue_reference_v<Ref> &&
                           !std::is_const_v<std::remove_reference_t<Ref>>,
                       erased_iterator<Ref>(), erased_iterator<Ref>() const>;

template <class Ref>
struct range_begin_dispatch {
  template <class T>
  PRO4D_STATIC_CALL(erased_iterator<Ref>, T&& self)
    requires(requires(T& range) {
      erased_iterator<Ref>{std::ranges::begin(range), std::ranges::end(range)};
    })
  {
    return erased_iterator<Ref>{std::ranges::begin(self),
                                std::ranges::end(self)};
  }

  template <class P, class D, class... Os>
  struct accessor {
    accessor() = delete;
  };
  template <class P, class D>
  struct accessor<P, D, erased_iterator<Ref>()> {
    PRO4D_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(begin)
    erased_iterator<Ref> begin() {
      return invoke<D, erased_iterator<Ref>()>(static_cast<P&>(*this));
    }
    std::default_sentinel_t end() const noexcept { return {}; }
  };
  template <class P, class D>
  struct accessor<P, D, erased_iterator<Ref>() const> {
    PRO4D_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(begin)
    erased_iterator<Ref> begin() const {
      return invoke<D, erased_iterator<Ref>() const>(
          static_cast<const P&>(*this));
    }
    std::default_sentinel_t end() const noexcept { return {}; }
  };
};
#endif // __STDC_HOSTED__

} // namespace detail

namespace facets {

// Restricts the memory layout to single-pointer-size.
struct slim : layout<sizeof(void*), alignof(void*)> {};

// Allows implicit conversion from proxy<F> to proxy_view<F>.
struct viewable
    : direct_convention<
          detail::view_conversion_dispatch,
          facade_aware_overload_t<detail::view_conversion_overload>> {};

// Allows implicit conversion from proxy<F> to weak_proxy<F>.
struct weakable
    : direct_convention<
          detail::weak_conversion_dispatch,
          facade_aware_overload_t<detail::weak_conversion_overload>> {};

// Makes proxy_indirect_accessor<F> invocable with the specified overloads.
template <detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
struct callable : indirect_convention<operator_dispatch<"()">, Os...> {};

#if __cpp_rtti >= 199711L
// Enables proxy_typeid and proxy_cast for proxy_indirect_accessor<F>.
struct indirect_rtti
    : pack<indirect_convention<detail::proxy_cast_dispatch,
                               void(detail::proxy_cast_context) &,
                               void(detail::proxy_cast_context) const&,
                               void(detail::proxy_cast_context) &&>,
           indirect_reflection<detail::proxy_typeid_reflector>> {};

// Enables proxy_typeid and proxy_cast for proxy<F>. Unlike indirect_rtti, this
// adds no convention: the reflection already names the contained pointer type,
// which is all a direct cast has to check.
struct direct_rtti : direct_reflection<detail::direct_rtti_reflector> {};

using rtti = indirect_rtti;
#endif // __cpp_rtti >= 199711L

#ifdef PRO4D_HAS_FORMAT
// Enables std::formatter<proxy_indirect_accessor<F>, char>.
struct formattable
    : indirect_convention<detail::std_format_traits::dispatch,
                          detail::std_format_traits::overload<char>> {};

// Enables std::formatter<proxy_indirect_accessor<F>, wchar_t>.
struct wformattable
    : indirect_convention<detail::std_format_traits::dispatch,
                          detail::std_format_traits::overload<wchar_t>> {};
#endif // PRO4D_HAS_FORMAT

#if __STDC_HOSTED__
// Enables `os << *p` for std::ostream.
struct serializable : indirect_convention<operator_dispatch<"<<", true>,
                                          std::ostream&(std::ostream&) const> {
};

// Enables `os << *p` for std::wostream.
struct wserializable
    : indirect_convention<operator_dispatch<"<<", true>,
                          std::wostream&(std::wostream&) const> {};

// Enables std::hash<proxy_indirect_accessor<F>>.
struct hashable
    : indirect_convention<detail::hash_dispatch, detail::hash_overload> {};

// Makes proxy_indirect_accessor<F> an input range whose reference type is Ref.
template <class Ref>
struct range_like : indirect_convention<detail::range_begin_dispatch<Ref>,
                                        detail::range_begin_overload_t<Ref>> {};
#endif // __STDC_HOSTED__

} // namespace facets

} // namespace pro::inline v4

#if __STDC_HOSTED__
namespace std {

template <class T>
  requires(pro::v4::detail::enabled_for<T, std::hash>)
struct hash<T> : pro::v4::detail::hash_impl<T> {};

} // namespace std
#endif // __STDC_HOSTED__

#if defined(PRO4D_HAS_FORMAT) && __cpp_lib_format_ranges >= 202207L
namespace std {

// How a proxy accessor prints is settled by the formattable facet alone. Left
// to the standard range formatter, a range_like accessor would pick up a
// formatter it never asked for, and would make the one formattable enables
// ambiguous with it.
template <pro::v4::facade F>
constexpr range_format format_kind<pro::v4::proxy_indirect_accessor<F>> =
    range_format::disabled;

} // namespace std
#endif // defined(PRO4D_HAS_FORMAT) && __cpp_lib_format_ranges >= 202207L

#endif // MSFT_PROXY_V4_DETAIL_FACETS_EXT_H_
