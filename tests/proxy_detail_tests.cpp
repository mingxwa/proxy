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

} // namespace proxy_detail_tests_detail
