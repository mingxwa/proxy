// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "utils.h"
#include <gtest/gtest.h>
#include <proxy/proxy.h>

namespace proxy_view_tests_details {

template <class F>
using AreEqualOverload = bool(const pro::proxy_indirect_accessor<F>& rhs,
                              double eps) const;

template <class T, pro::facade F>
bool AreEqualImpl(const T& lhs, const pro::proxy_indirect_accessor<F>& rhs,
                  double eps) {
  return lhs.AreEqual(proxy_cast<const T&>(rhs), eps);
}

PRO_DEF_FREE_AS_MEM_DISPATCH(MemAreEqual, AreEqualImpl, AreEqual);

struct EqualableQuantity
    : pro::facade_builder            //
      ::add_skill<pro::skills::rtti> // for proxy_cast
      ::add_convention<MemAreEqual,
                       pro::facade_aware_overload_t<AreEqualOverload>> //
      ::build {};

class Point_2 {
public:
  Point_2(double x, double y) : x_(x), y_(y) {}
  Point_2(const Point_2&) = default;
  Point_2& operator=(const Point_2&) = default;
  bool AreEqual(const Point_2& rhs, double eps) const {
    return std::abs(x_ - rhs.x_) < eps && std::abs(y_ - rhs.y_) < eps;
  }

private:
  double x_, y_;
};

} // namespace proxy_view_tests_details

namespace details = proxy_view_tests_details;

TEST(ProxyViewTests, TestFacadeAware) {
  details::Point_2 v1{1, 1};
  pro::proxy_view<details::EqualableQuantity> p1 =
      pro::make_proxy_view<details::EqualableQuantity>(v1);
}
