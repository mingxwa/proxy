#include <gtest/gtest.h>
#include <iostream>
#include <utility>

import proxy.v5;
import foo;
import foo_impl;

auto user(pro::proxy<Foo> p) { return p->GetFoo(); }

TEST(ProxyModuleSupportTests, TestBasic) {
  MyFoo foo;
  ASSERT_EQ(user(&foo), 42);
}

TEST(ProxyModuleSupportTests, TestRepeatedSpecialization) {
  MyFoo foo;
  pro::proxy<Foo> p = &foo;
  ASSERT_EQ(user(std::move(p)), 42);
}

TEST(ProxyModuleSupportTests, TestBox) {
  pro::box<Foo> b{std::in_place_type<MyFoo>};
  ASSERT_EQ(b.GetFoo(), 42);
  pro::proxy<Foo> p = b.release();
  ASSERT_EQ(p->GetFoo(), 42);
}

TEST(ProxyModuleSupportTests, TestBoxView) {
  struct ViewableFoo : pro::facade_builder               //
                       ::add_facade<Foo>                 //
                       ::add_skill<pro::skills::as_view> //
                       ::build {};
  pro::box<ViewableFoo> b{std::in_place_type<MyFoo>};
  pro::box_view<ViewableFoo> v = b;
  ASSERT_EQ(v.GetFoo(), 42);
  pro::box_view<Foo> v2 = b;
  ASSERT_EQ(v2.GetFoo(), 42);
}
