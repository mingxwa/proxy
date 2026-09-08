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
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#endif // __STDC_HOSTED__

#include "core.h"
#include "dispatch.h"
#include "facade_creation.h"
#include "proxy_creation.h"
#include "skills.h"

namespace pro::inline v4 {

namespace detail {

PRO4_DEF_MEM_DISPATCH(size_dispatch, size);

#if __cpp_rtti >= 199711L
struct equality_dispatch {
  template <class T, class A>
  PRO4D_STATIC_CALL(bool, const T& self, const A& rhs)
    requires(std::equality_comparable<T>)
  {
    return typeid(T) == proxy_typeid(rhs) && self == proxy_cast<const T&>(rhs);
  }
  PRO4D_DEF_ACCESSOR_TEMPLATE(FREE, PRO4D_DEF_FREE_ACCESSOR, operator==)
};
template <class F>
using equality_overload = bool(const proxy_indirect_accessor<F>&) const;
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

PRO4_DEF_MEM_DISPATCH(range_cursor_next_dispatch, pro_next);

template <class Ref>
using range_cache_t = std::optional<std::conditional_t<
    std::is_reference_v<Ref>,
    std::reference_wrapper<std::remove_reference_t<Ref>>, Ref>>;
template <class Cache, class I>
concept range_cacheable = requires(Cache& cache, I& it) { cache.emplace(*it); };

template <class I, class S>
class range_cursor {
public:
  range_cursor(I first, S last)
      : it_(std::move(first)), end_(std::move(last)) {}

  template <class Cache>
    requires(range_cacheable<Cache, I>)
  bool pro_next(Cache& cache) {
    if (++it_ == end_) {
      return false;
    }
    cache.emplace(*it_);
    return true;
  }

private:
  I it_;
  S end_;
};

template <class Ref>
using erased_cursor_facade =
    make_facade<facets::indirect_convention<range_cursor_next_dispatch,
                                            bool(range_cache_t<Ref>&)>,
                facets::copyability<constraint_level::nontrivial>,
                facets::relocatability<constraint_level::nontrivial>>;

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
  erased_iterator(I first, S last) {
    if (first != last) {
      cache_.emplace(*first);
      impl_ = make_proxy<cursor_facade, range_cursor<I, S>>(std::move(first),
                                                            std::move(last));
    }
  }

  Ref operator*() const { return *cache_; }
  erased_iterator& operator++() {
    if (!impl_->pro_next(cache_)) {
      impl_.reset();
    }
    return *this;
  }
  void operator++(int) { ++*this; }
  bool operator==(std::default_sentinel_t) const { return !impl_.has_value(); }

private:
  proxy<cursor_facade> impl_;
  range_cache_t<Ref> cache_;
};

template <class Ref>
using range_begin_overload_t =
    std::conditional_t<std::is_lvalue_reference_v<Ref> &&
                           !std::is_const_v<std::remove_reference_t<Ref>>,
                       erased_iterator<Ref>(), erased_iterator<Ref>() const>;

#define PRO4D_DEF_RANGE_BEGIN_ACCESSOR(oq, pq, ne, ...)                        \
  template <class ProP, class ProD, class ProR>                                \
  struct accessor<ProP, ProD, ProR() oq ne> {                                  \
    PRO4D_GEN_DEBUG_SYMBOL_FOR_MEM_ACCESSOR(begin)                             \
    ProR begin() oq ne {                                                       \
      return invoke<ProD, ProR() oq ne>(static_cast<ProP pq>(*this));          \
    }                                                                          \
    std::default_sentinel_t end() const noexcept {                             \
      return std::default_sentinel;                                            \
    }                                                                          \
  }
template <class Ref>
struct range_begin_dispatch {
  template <class T>
  PRO4D_STATIC_CALL(erased_iterator<Ref>, T&& self)
    requires(requires {
      erased_iterator<Ref>{std::ranges::begin(self), std::ranges::end(self)};
    })
  {
    return erased_iterator<Ref>{std::ranges::begin(self),
                                std::ranges::end(self)};
  }
  PRO4D_DEF_ACCESSOR_TEMPLATE(MEM, PRO4D_DEF_RANGE_BEGIN_ACCESSOR, begin)
};
#undef PRO4D_DEF_RANGE_BEGIN_ACCESSOR
#endif // __STDC_HOSTED__

} // namespace detail

namespace facets {

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

// Makes proxy_indirect_accessor<F> subscriptable with the specified overloads.
template <detail::extended_overload... Os>
  requires(sizeof...(Os) > 0u)
struct indexable : indirect_convention<operator_dispatch<"[]">, Os...> {};

// Enables `(*p).size()`, and std::ranges::size when F is also a range.
struct sized : indirect_convention<detail::size_dispatch, std::size_t() const> {
};

#if __cpp_rtti >= 199711L
// Enables proxy_typeid and proxy_cast for proxy_indirect_accessor<F>.
struct indirect_castable
    : pack<indirect_convention<detail::proxy_cast_dispatch,
                               void(detail::proxy_cast_context) &,
                               void(detail::proxy_cast_context) const&,
                               void(detail::proxy_cast_context) &&>,
           indirect_reflection<detail::proxy_typeid_reflector>> {};

// Enables proxy_typeid and proxy_cast for proxy<F>.
struct direct_castable : direct_reflection<detail::direct_rtti_reflector> {};

using castable = indirect_castable;

// Enables `*p1 == *p2`, comparing the contained values of two proxies.
struct equality_comparable
    : pack<indirect_castable,
           indirect_convention<
               detail::equality_dispatch,
               facade_aware_overload_t<detail::equality_overload>>> {};
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
// Enables `is >> *p` for std::istream.
struct istreamable : indirect_convention<operator_dispatch<">>", true>,
                                         std::istream&(std::istream&)> {};

// Enables `is >> *p` for std::wistream.
struct wistreamable : indirect_convention<operator_dispatch<">>", true>,
                                          std::wistream&(std::wistream&)> {};

// Enables `os << *p` for std::ostream.
struct ostreamable : indirect_convention<operator_dispatch<"<<", true>,
                                         std::ostream&(std::ostream&) const> {};

// Enables `os << *p` for std::wostream.
struct wostreamable
    : indirect_convention<operator_dispatch<"<<", true>,
                          std::wostream&(std::wostream&) const> {};

// Enables std::hash<proxy_indirect_accessor<F>>.
struct hashable
    : indirect_convention<detail::hash_dispatch, detail::hash_overload> {};

// Makes proxy_indirect_accessor<F> an input range yielding T from `*it`, where
// T may be a reference or a value.
template <class T>
  requires(std::is_reference_v<T> || std::is_copy_constructible_v<T>)
struct input_range : indirect_convention<detail::range_begin_dispatch<T>,
                                         detail::range_begin_overload_t<T>> {};
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

// A proxy accessor prints as its formattable facet says, never as a range.
template <pro::v4::facade F>
constexpr range_format format_kind<pro::v4::proxy_indirect_accessor<F>> =
    range_format::disabled;

} // namespace std
#endif // defined(PRO4D_HAS_FORMAT) && __cpp_lib_format_ranges >= 202207L

#endif // MSFT_PROXY_V4_DETAIL_FACETS_EXT_H_
