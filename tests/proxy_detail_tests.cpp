// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#include <proxy/proxy.h>

namespace proxy_detail_tests_detail {

struct Base {
  int v;
};
struct Derived : Base {};

static_assert(pro::detail::explicitly_convertible<int, int>);
static_assert(pro::detail::explicitly_convertible<long, int>);
static_assert(!pro::detail::explicitly_convertible<int, int&&>);
static_assert(!pro::detail::explicitly_convertible<int, const int&>);
static_assert(pro::detail::explicitly_convertible<int&&, int&&>);
static_assert(pro::detail::explicitly_convertible<int&&, const int&>);
static_assert(!pro::detail::explicitly_convertible<long&&, int&&>);
static_assert(!pro::detail::explicitly_convertible<long, int&&>);
static_assert(pro::detail::explicitly_convertible<Derived&, Base&>);
static_assert(pro::detail::explicitly_convertible<Derived&, const Base&>);
static_assert(!pro::detail::explicitly_convertible<Derived&, Base&&>);
static_assert(!pro::detail::explicitly_convertible<const Derived&, Base&>);
static_assert(pro::detail::explicitly_convertible<const Derived&, const Base&>);
static_assert(pro::detail::explicitly_convertible<Derived, Base>);
static_assert(!pro::detail::explicitly_convertible<Derived, Base&&>);
static_assert(!pro::detail::explicitly_convertible<Base&, Derived&>);

// Stand-ins for the meta protocol: converting to a meta is all that the
// reduction and the lookup path depend on.
struct MetaA {};
struct MetaB {};
struct MetaC {};
struct MetaX {};
template <class... Ms>
struct MetaAggregate {
  template <class T>
    requires((std::is_convertible_v<const Ms&, const T&> || ...))
  operator const T&() const noexcept; // never called
};

template <class A, class B>
constexpr bool contains_v = std::is_convertible_v<const A&, const B&>;

static_assert(contains_v<MetaA, MetaA>);
static_assert(contains_v<MetaAggregate<MetaA>, MetaA>);
static_assert(contains_v<MetaAggregate<MetaAggregate<MetaA>>, MetaA>);
static_assert(!contains_v<MetaAggregate<MetaA>, MetaB>);
static_assert(!contains_v<MetaA, MetaAggregate<MetaA>>);

template <class T, class... Us>
using FirstContaining = pro::detail::recursive_reduction_t<
    pro::detail::reduction_t<pro::detail::first_containing_reduction, T>, void,
    Us...>;

static_assert(std::is_void_v<FirstContaining<MetaA>>);
static_assert(std::is_void_v<FirstContaining<MetaA, MetaB>>);
static_assert(std::is_same_v<FirstContaining<MetaA, MetaA>, MetaA>);
static_assert(
    std::is_same_v<FirstContaining<MetaA, MetaB, MetaAggregate<MetaA>>,
                   MetaAggregate<MetaA>>);
static_assert(std::is_same_v<FirstContaining<MetaA, MetaAggregate<MetaA, MetaB>,
                                             MetaAggregate<MetaA, MetaC>>,
                             MetaAggregate<MetaA, MetaB>>);

template <class... Ts>
using Reduce = pro::detail::unique_types_t<std::tuple<Ts...>>;

// Independent metas are kept in order
static_assert(std::is_same_v<Reduce<>, std::tuple<>>);
static_assert(std::is_same_v<Reduce<MetaA, MetaB>, std::tuple<MetaA, MetaB>>);
// A duplicate resolves to the leftmost occurrence
static_assert(std::is_same_v<Reduce<MetaA, MetaA>, std::tuple<MetaA>>);
// A meta subsumed by a later aggregate is replaced by that aggregate
static_assert(std::is_same_v<Reduce<MetaA, MetaAggregate<MetaA, MetaB>>,
                             std::tuple<MetaAggregate<MetaA, MetaB>>>);
// ... and one subsumed by an aggregate that was already kept is dropped
static_assert(std::is_same_v<Reduce<MetaAggregate<MetaA, MetaB>, MetaA>,
                             std::tuple<MetaAggregate<MetaA, MetaB>>>);
// The replacement happens in place: the aggregate takes the position the
// subsumed meta was declared at, ahead of anything declared after it
static_assert(std::is_same_v<Reduce<MetaA, MetaX, MetaAggregate<MetaA, MetaB>,
                                    MetaAggregate<MetaA, MetaC>>,
                             std::tuple<MetaAggregate<MetaA, MetaB>, MetaX,
                                        MetaAggregate<MetaA, MetaC>>>);
// Nested aggregates collapse to the outermost one
static_assert(
    std::is_same_v<Reduce<MetaA, MetaAggregate<MetaA>,
                          MetaAggregate<MetaAggregate<MetaA>, MetaB>>,
                   std::tuple<MetaAggregate<MetaAggregate<MetaA>, MetaB>>>);
// A diamond keeps both aggregates: neither one contains the other, so the
// shared meta stays reachable through the leftmost of them
static_assert(
    std::is_same_v<
        Reduce<MetaA, MetaAggregate<MetaA, MetaB>, MetaAggregate<MetaA, MetaC>>,
        std::tuple<MetaAggregate<MetaA, MetaB>, MetaAggregate<MetaA, MetaC>>>);

// The property every reduced list satisfies, and which keeps conversion to a
// contained meta unambiguous: each element is contained by exactly one element
// of the list, namely itself.
template <class Ts>
struct independence_check;
template <class... Ts>
struct independence_check<std::tuple<Ts...>> {
  template <class T>
  static constexpr int containing_count =
      (static_cast<int>(contains_v<Ts, T>) + ... + 0);
  static constexpr bool value = (... && (containing_count<Ts> == 1));
};
template <class Ts>
constexpr bool is_independent_v = independence_check<Ts>::value;

static_assert(!is_independent_v<std::tuple<MetaA, MetaA>>);
static_assert(!is_independent_v<std::tuple<MetaA, MetaAggregate<MetaA>>>);
static_assert(is_independent_v<Reduce<MetaA, MetaB, MetaC>>);
static_assert(is_independent_v<Reduce<MetaA, MetaA>>);
static_assert(is_independent_v<Reduce<MetaA, MetaAggregate<MetaA, MetaB>,
                                      MetaAggregate<MetaA, MetaC>>>);
static_assert(is_independent_v<Reduce<MetaA, MetaX, MetaAggregate<MetaA, MetaB>,
                                      MetaAggregate<MetaA, MetaC>>>);
static_assert(is_independent_v<Reduce<MetaAggregate<MetaA>, MetaA, MetaB,
                                      MetaAggregate<MetaAggregate<MetaA>>>>);

} // namespace proxy_detail_tests_detail
