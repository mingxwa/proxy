// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#include "utils.h"
#include <gtest/gtest.h>
#include <array>
#include <memory>
#include <memory_resource>
#include <optional>
#include <proxy/proxy.h>
#include <string>
#include <utility>

namespace proxy_box_tests_detail {

enum LifetimeModelType { kNone, kInplace, kWide, kCompact };

// A direct reflection that records which owning pointer model the underlying
// `proxy` selected. `box` itself only exposes indirect accessors, so this is
// inspected through `box::release()`.
struct LifetimeModelReflector {
  template <class T>
  constexpr explicit LifetimeModelReflector(
      std::in_place_type_t<pro::detail::inplace_ptr<T>>)
      : Type(LifetimeModelType::kInplace) {}
  template <class T, class Alloc>
  constexpr explicit LifetimeModelReflector(
      std::in_place_type_t<pro::detail::wide_ptr<T, Alloc>>)
      : Type(LifetimeModelType::kWide) {}
  template <class T, class Alloc>
  constexpr explicit LifetimeModelReflector(
      std::in_place_type_t<pro::detail::compact_ptr<T, Alloc>>)
      : Type(LifetimeModelType::kCompact) {}
  template <class T>
  constexpr explicit LifetimeModelReflector(std::in_place_type_t<T>)
      : Type(LifetimeModelType::kNone) {}

  template <class Self, class R>
  struct accessor {
    LifetimeModelType GetLifetimeType() const noexcept {
      return reflect<R>(static_cast<const Self&>(*this)).Type;
    }
  };

  LifetimeModelType Type;
};

// An indirect reflection that records the size of the contained value type. It
// is exposed by `box` directly because it is indirect.
struct SizeReflector {
  template <class T>
  constexpr explicit SizeReflector(std::in_place_type_t<T>) : Size(sizeof(T)) {}

  template <class Self, class R>
  struct accessor {
    std::size_t GetContainedSize() const noexcept {
      return reflect<R>(static_cast<const Self&>(*this)).Size;
    }
  };

  std::size_t Size;
};

struct TestLargeStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string() const> //
      ::support_copy<pro::constraint_level::nontrivial>          //
      ::support_relocation<pro::constraint_level::nontrivial>    //
      ::add_direct_reflection<LifetimeModelReflector>            //
      ::add_indirect_reflection<SizeReflector>                   //
      ::build {};

struct TestSmallStringable : pro::facade_builder               //
                             ::add_facade<TestLargeStringable> //
                             ::restrict_layout<sizeof(void*)>  //
                             ::build {};

// Move-only facade: the default copyability is `none`.
struct TestMovableStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string() const> //
      ::build {};

struct SmallValue {
  int value;
  friend std::string to_string(const SmallValue& self) {
    return std::to_string(self.value);
  }
};

struct LargeValue {
  std::array<int, 16> padding;
  int value;
  explicit LargeValue(int v) : padding{}, value(v) {}
  LargeValue(std::initializer_list<int> il, int v) : padding{}, value(v) {
    for (int x : il) {
      value += x;
    }
  }
  friend std::string to_string(const LargeValue& self) {
    return std::to_string(self.value);
  }
};

// `box<F>` is layout-compatible with `proxy<F>`; it merely wraps one.
static_assert(sizeof(pro::box<TestLargeStringable>) ==
              sizeof(pro::proxy<TestLargeStringable>));
static_assert(sizeof(pro::box<TestSmallStringable>) ==
              sizeof(pro::proxy<TestSmallStringable>));

// `box` mirrors the special-member-function support of the underlying `proxy`.
static_assert(std::is_copy_constructible_v<pro::box<TestLargeStringable>>);
static_assert(std::is_move_constructible_v<pro::box<TestLargeStringable>>);
static_assert(std::is_nothrow_default_constructible_v<pro::box<TestLargeStringable>>);
static_assert(!std::is_copy_constructible_v<pro::box<TestMovableStringable>>);
static_assert(std::is_move_constructible_v<pro::box<TestMovableStringable>>);

template <pro::facade F>
std::string IndirectToString(const pro::proxy_indirect_accessor<F>& ia) {
  return ToString(ia);
}

TEST(ProxyBoxTests, TestDefault) {
  pro::box<TestLargeStringable> b;
  ASSERT_FALSE(b.has_value());
  ASSERT_FALSE(static_cast<bool>(b));
  ASSERT_TRUE(b == std::nullopt);
}

TEST(ProxyBoxTests, TestNullopt) {
  pro::box<TestLargeStringable> b = std::nullopt;
  ASSERT_FALSE(b.has_value());
  ASSERT_TRUE(b == std::nullopt);
}

TEST(ProxyBoxTests, TestConstructFromValue_Inplace) {
  pro::box<TestLargeStringable> b = SmallValue{123};
  ASSERT_TRUE(b.has_value());
  ASSERT_TRUE(static_cast<bool>(b));
  ASSERT_EQ(ToString(b), "123");
  ASSERT_EQ(b.GetContainedSize(), sizeof(SmallValue));
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kInplace);
}

TEST(ProxyBoxTests, TestConstructFromValue_Wide) {
  pro::box<TestLargeStringable> b = LargeValue{7};
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(ToString(b), "7");
  ASSERT_EQ(b.GetContainedSize(), sizeof(LargeValue));
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kWide);
}

TEST(ProxyBoxTests, TestConstructFromValue_SmallFacadeFallsBackToWide) {
  pro::box<TestSmallStringable> b = LargeValue{7};
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(ToString(b), "7");
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kWide);
}

TEST(ProxyBoxTests, TestConstructInPlaceType) {
  pro::box<TestLargeStringable> b{std::in_place_type<SmallValue>, 5};
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(ToString(b), "5");
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kInplace);
}

TEST(ProxyBoxTests, TestConstructInPlaceTypeInitializerList) {
  pro::box<TestLargeStringable> b{std::in_place_type<LargeValue>, {1, 2, 3}, 10};
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(ToString(b), "16");
}

TEST(ProxyBoxTests, TestConstructAllocatorArg_FromValue) {
  pro::box<TestLargeStringable> b{std::allocator_arg, std::allocator<void>{},
                                  LargeValue{9}};
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(ToString(b), "9");
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kWide);
}

TEST(ProxyBoxTests, TestConstructAllocatorArg_InPlaceType) {
  pro::box<TestLargeStringable> b{std::allocator_arg, std::allocator<void>{},
                                  std::in_place_type<LargeValue>, 11};
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(ToString(b), "11");
}

TEST(ProxyBoxTests, TestConstructAllocatorArg_InPlaceTypeInitializerList) {
  std::pmr::unsynchronized_pool_resource pool;
  std::pmr::polymorphic_allocator<> alloc{&pool};
  // A stateful allocator does not fit alongside the value in a slim facade, so
  // the compact pointer model is selected.
  pro::box<TestSmallStringable> b{std::allocator_arg, alloc,
                                  std::in_place_type<LargeValue>, {4, 5}, 1};
  ASSERT_TRUE(b.has_value());
  ASSERT_EQ(ToString(b), "10");
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kCompact);
}

TEST(ProxyBoxTests, TestAssignFromValue) {
  pro::box<TestLargeStringable> b;
  b = SmallValue{1};
  ASSERT_EQ(ToString(b), "1");
  b = SmallValue{2};
  ASSERT_EQ(ToString(b), "2");
}

TEST(ProxyBoxTests, TestAssignNullopt) {
  pro::box<TestLargeStringable> b = SmallValue{1};
  ASSERT_TRUE(b.has_value());
  b = std::nullopt;
  ASSERT_FALSE(b.has_value());
}

TEST(ProxyBoxTests, TestReset) {
  pro::box<TestLargeStringable> b = SmallValue{1};
  ASSERT_TRUE(b.has_value());
  b.reset();
  ASSERT_FALSE(b.has_value());
}

TEST(ProxyBoxTests, TestEmplace) {
  pro::box<TestLargeStringable> b;
  SmallValue& v = b.emplace<SmallValue>(42);
  ASSERT_EQ(v.value, 42);
  ASSERT_EQ(ToString(b), "42");
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kInplace);
}

TEST(ProxyBoxTests, TestEmplaceInitializerList) {
  pro::box<TestLargeStringable> b;
  LargeValue& v = b.emplace<LargeValue>({1, 2, 3}, 4);
  ASSERT_EQ(v.value, 10);
  ASSERT_EQ(ToString(b), "10");
}

TEST(ProxyBoxTests, TestEmplaceAlloc) {
  std::pmr::unsynchronized_pool_resource pool;
  std::pmr::polymorphic_allocator<> alloc{&pool};
  pro::box<TestSmallStringable> b;
  LargeValue& v = b.emplace_alloc<LargeValue>(alloc, 3);
  ASSERT_EQ(v.value, 3);
  ASSERT_EQ(ToString(b), "3");
  ASSERT_EQ(b.release().GetLifetimeType(), LifetimeModelType::kCompact);
}

TEST(ProxyBoxTests, TestRelease) {
  pro::box<TestLargeStringable> b = SmallValue{8};
  pro::proxy<TestLargeStringable> p = b.release();
  ASSERT_FALSE(b.has_value());
  ASSERT_TRUE(p.has_value());
  ASSERT_EQ(ToString(*p), "8");
}

TEST(ProxyBoxTests, TestConvertToIndirectAccessor) {
  pro::box<TestLargeStringable> b = SmallValue{6};
  const pro::box<TestLargeStringable>& cb = b;
  ASSERT_EQ(IndirectToString<TestLargeStringable>(b), "6");
  ASSERT_EQ(IndirectToString<TestLargeStringable>(cb), "6");
}

TEST(ProxyBoxTests, TestMemberSwap) {
  pro::box<TestLargeStringable> b1 = SmallValue{1};
  pro::box<TestLargeStringable> b2 = SmallValue{2};
  b1.swap(b2);
  ASSERT_EQ(ToString(b1), "2");
  ASSERT_EQ(ToString(b2), "1");
}

TEST(ProxyBoxTests, TestFreeSwap) {
  pro::box<TestLargeStringable> b1 = SmallValue{1};
  pro::box<TestLargeStringable> b2;
  using std::swap;
  swap(b1, b2);
  ASSERT_FALSE(b1.has_value());
  ASSERT_EQ(ToString(b2), "1");
}

TEST(ProxyBoxTests, TestCopyValueSemantics) {
  pro::box<TestLargeStringable> b1 = SmallValue{1};
  pro::box<TestLargeStringable> b2 = b1;
  b1 = SmallValue{2};
  ASSERT_EQ(ToString(b1), "2");
  ASSERT_EQ(ToString(b2), "1"); // b2 is an independent copy
}

TEST(ProxyBoxTests, TestMoveSemantics) {
  pro::box<TestLargeStringable> b1 = SmallValue{1};
  pro::box<TestLargeStringable> b2 = std::move(b1);
  ASSERT_FALSE(b1.has_value());
  ASSERT_EQ(ToString(b2), "1");
}

TEST(ProxyBoxTests, TestLifetime_Construction) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<TestLargeStringable> b{std::in_place_type<utils::LifetimeTracker::Session>,
                                    &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    ASSERT_EQ(ToString(b), "Session 1");
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(ProxyBoxTests, TestLifetime_Copy) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<TestLargeStringable> b1{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    auto b2 = b1;
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kCopyConstruction);
    ASSERT_EQ(ToString(b1), "Session 1");
    ASSERT_EQ(ToString(b2), "Session 2");
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(ProxyBoxTests, TestLifetime_Move) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<TestLargeStringable> b1{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    auto b2 = std::move(b1);
    ASSERT_FALSE(b1.has_value());
    ASSERT_EQ(ToString(b2), "Session 2");
    // The contained value is held in-place, so relocation move-constructs a new
    // value and destroys the moved-from one.
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kMoveConstruction);
    expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

} // namespace proxy_box_tests_detail
