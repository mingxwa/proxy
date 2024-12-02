// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include "proxy.h"
#include "utils.h"

namespace proxy_view_tests_details {

struct TestFacade : pro::facade_builder
    ::add_convention<pro::operator_dispatch<"+=">, void(int)>
    ::add_convention<utils::spec::FreeToString, std::string() const>
    ::add_direct_convention<
        pro::proxy_view_dispatch<TestFacade>,
        pro::proxy_view<TestFacade>() noexcept>
    ::build {};

static_assert(!std::is_copy_constructible_v<pro::proxy<TestFacade>>);
static_assert(!std::is_trivially_destructible_v<pro::proxy<TestFacade>>);
static_assert(sizeof(pro::proxy<TestFacade>) == 3 * sizeof(void*));


static_assert(std::is_trivially_copy_constructible_v<pro::proxy_view<TestFacade>>);
static_assert(std::is_trivially_destructible_v<pro::proxy_view<TestFacade>>);
static_assert(sizeof(pro::proxy_view<TestFacade>) == 2 * sizeof(void*));

}  // namespace proxy_view_tests_details

namespace details = proxy_view_tests_details;

TEST(ProxyViewTests, TestViewOfNull) {
  pro::proxy<details::TestFacade> p1;
  pro::proxy_view<details::TestFacade> p2 = p1;
  ASSERT_FALSE(p2.has_value());
}

TEST(ProxyViewTests, TestViewOfOwning) {
  pro::proxy<details::TestFacade> p1 = pro::make_proxy<details::TestFacade>(123);
  pro::proxy_view<details::TestFacade> p2 = p1;
  ASSERT_TRUE(p1.has_value());
  ASSERT_TRUE(p2.has_value());
  *p2 += 3;
  ASSERT_EQ(ToString(*p1), "126");
  ASSERT_EQ(ToString(*p2), "126");
  p1.reset();
  // p2 becomes dangling
}

TEST(ProxyViewTests, TestViewOfNonOwning) {
  int a = 123;
  pro::proxy<details::TestFacade> p1 = &a;
  pro::proxy_view<details::TestFacade> p2 = p1;
  ASSERT_TRUE(p1.has_value());
  ASSERT_TRUE(p2.has_value());
  *p2 += 3;
  ASSERT_EQ(ToString(*p1), "126");
  p1.reset();
  ASSERT_EQ(ToString(*p2), "126");
  ASSERT_EQ(a, 126);
}
