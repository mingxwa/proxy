// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <gtest/gtest.h>
#include <fmt/format.h>
#include <fmt/xchar.h>
#include "proxy.h"

namespace proxy_format_tests_details {

struct NonFormattable : pro::facade_builder::build {};

static_assert(!std::is_default_constructible_v<fmt::formatter<pro::proxy_indirect_accessor<NonFormattable>, char>>);
static_assert(!std::is_default_constructible_v<fmt::formatter<pro::proxy_indirect_accessor<NonFormattable>, wchar_t>>);

struct Formattable : pro::facade_builder
    ::support_format
    ::support_wformat
    ::build {};

static_assert(std::is_default_constructible_v<fmt::formatter<pro::proxy_indirect_accessor<Formattable>, char>>);
static_assert(std::is_default_constructible_v<fmt::formatter<pro::proxy_indirect_accessor<Formattable>, wchar_t>>);

}  // namespace proxy_format_tests_details

namespace details = proxy_format_tests_details;

TEST(ProxyFormatTests_Fmtlib, TestFormat_Null) {
  pro::proxy<details::Formattable> p;
  bool exception_thrown = false;
  try {
    std::ignore = fmt::format("{}", *p);
  } catch (const fmt::format_error&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}

TEST(ProxyFormatTests_Fmtlib, TestFormat_Value) {
  int v = 123;
  pro::proxy<details::Formattable> p = &v;
  ASSERT_EQ(fmt::format("{}", *p), "123");
  ASSERT_EQ(fmt::format("{:*<6}", *p), "123***");
}

TEST(ProxyFormatTests_Fmtlib, TestWformat_Null) {
  pro::proxy<details::Formattable> p;
  bool exception_thrown = false;
  try {
    std::ignore = fmt::format(L"{}", *p);
  } catch (const fmt::format_error&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}

TEST(ProxyFormatTests_Fmtlib, TestWformat_Value) {
  int v = 123;
  pro::proxy<details::Formattable> p = &v;
  ASSERT_EQ(fmt::format(L"{}", *p), L"123");
  ASSERT_EQ(fmt::format(L"{:*<6}", *p), L"123***");
}
