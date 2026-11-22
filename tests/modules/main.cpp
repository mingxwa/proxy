#include <cstddef>
#include <gtest/gtest.h>
#include <iostream>
#include <new>
#include <utility>

import proxy.v5;
import foo;
import foo_impl;

auto user(pro::proxy<Foo> p) { return p->GetFoo(); }

struct ModuleTestArena {
  std::size_t allocated = 0u;
};

void* operator new(std::size_t size, ModuleTestArena& arena) {
  arena.allocated += size;
  return ::operator new(size);
}

struct Allocator
    : pro::facade_builder::add_convention<pro::operator_dispatch<"new">,
                                          void*(std::size_t)>::build {};

TEST(ProxyModuleSupportTests, TestBasic) {
  MyFoo foo;
  ASSERT_EQ(user(&foo), 42);
}

TEST(ProxyModuleSupportTests, TestRepeatedSpecialization) {
  MyFoo foo;
  pro::proxy<Foo> p = &foo;
  ASSERT_EQ(user(std::move(p)), 42);
}

TEST(ProxyModuleSupportTests, TestPlacementNew) {
  ModuleTestArena arena;
  pro::proxy<Allocator> p = &arena;
  int* i = new (*p) int{42};
  ASSERT_EQ(arena.allocated, sizeof(int));
  ASSERT_EQ(*i, 42);
  ::operator delete(i);
}
