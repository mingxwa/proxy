#include <format>
#include <iostream>
#include <gtest/gtest.h>

import proxy;
import foo;
import foo_impl;

auto user(pro::proxy<Foo> p) {
  return p->GetFoo();
}

TEST(ProxyModuleSupportTests, TestBasic) {
  MyFoo foo;
  ASSERT_EQ(user(&foo), 42);
}

TEST(ProxyModuleSupportTests, TestFormattable) {
  auto p = pro::make_proxy<Formattable>(123);
  ASSERT_EQ(std::format("{}", *p), "123");
}
