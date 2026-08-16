#include <gtest/gtest.h>
#include <iostream>
#include <utility>

import proxy.v4;
import foo;
import foo_impl;

auto user(pro::proxy<Foo> p) { return p->GetFoo(); }

TEST(ProxyModuleSupportTests, TestBasic) {
  MyFoo foo;
  ASSERT_EQ(user(&foo), 42);
}

TEST(ProxyModuleSupportTests, TestBox) {
  pro::box<Foo> b{std::in_place_type<MyFoo>};
  ASSERT_EQ(b.GetFoo(), 42);
  pro::proxy<Foo> p = b.release();
  ASSERT_EQ(p->GetFoo(), 42);
}
