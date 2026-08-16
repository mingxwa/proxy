// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#include "utils.h"
#include <gtest/gtest.h>
#include <memory_resource>
#include <optional>
#include <proxy/proxy.h>

namespace box_tests_detail {

enum LifetimeModelType { kNone, kInplace, kWide, kCompact };

struct LifetimeModelReflector {
  LifetimeModelReflector() = default;
  template <class T>
  constexpr explicit LifetimeModelReflector(
      std::in_place_type_t<pro::detail::inplace_ptr<T>>) noexcept
      : Type(LifetimeModelType::kInplace) {}
  template <class T, class Alloc>
  constexpr explicit LifetimeModelReflector(
      std::in_place_type_t<pro::detail::wide_ptr<T, Alloc>>) noexcept
      : Type(LifetimeModelType::kWide) {}
  template <class T, class Alloc>
  constexpr explicit LifetimeModelReflector(
      std::in_place_type_t<pro::detail::compact_ptr<T, Alloc>>) noexcept
      : Type(LifetimeModelType::kCompact) {}
  template <class T>
  constexpr explicit LifetimeModelReflector(std::in_place_type_t<T>) noexcept
      : Type(LifetimeModelType::kNone) {}

  template <class Self, class R>
  struct accessor {
    LifetimeModelType GetLifetimeType() const noexcept {
      const LifetimeModelReflector& refl =
          reflect<R>(static_cast<const Self&>(*this));
      return refl.Type;
    }
  };

  LifetimeModelType Type;
};

struct TestLargeStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_relocation<pro::constraint_level::nontrivial>    //
      ::support_copy<pro::constraint_level::nontrivial>          //
      ::add_direct_reflection<LifetimeModelReflector>            //
      ::build {};

struct TestSmallStringable : pro::facade_builder               //
                             ::add_facade<TestLargeStringable> //
                             ::restrict_layout<sizeof(void*)>  //
                             ::build {};

PRO_DEF_MEM_DISPATCH(MemSize, Size);

struct TestSized : pro::facade_builder                                     //
                   ::add_convention<MemSize, std::size_t() const>          //
                   ::support_relocation<pro::constraint_level::nontrivial> //
                   ::support_copy<pro::constraint_level::nontrivial>       //
                   ::build {};

struct TestSizedStringable
    : pro::facade_builder                                        //
      ::add_facade<TestSized>                                    //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_relocation<pro::constraint_level::nontrivial>    //
      ::support_copy<pro::constraint_level::nontrivial>          //
      ::build {};

struct TestRttiStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_copy<pro::constraint_level::nontrivial>          //
      ::add_skill<pro::skills::rtti>                             //
      ::build {};

class Widget {
public:
  explicit Widget(int id) noexcept : id_(id) {}

  std::size_t Size() const { return static_cast<std::size_t>(id_); }
  friend std::string to_string(const Widget& self) {
    return "Widget " + std::to_string(self.id_);
  }

private:
  int id_;
};

static_assert(
    std::is_convertible_v<pro::box<TestSizedStringable>, pro::box<TestSized>>);
static_assert(
    !std::is_convertible_v<pro::box<TestSized>, pro::box<TestSizedStringable>>);
static_assert(!std::is_constructible_v<pro::box<TestSized>,
                                       pro::box<TestLargeStringable>>);
static_assert(std::is_same_v<pro::box<TestSized>::facade_type, TestSized>);
static_assert(
    std::is_convertible_v<pro::box<TestLargeStringable>&,
                          pro::proxy_indirect_accessor<TestLargeStringable>&>);
static_assert(std::is_convertible_v<
              const pro::box<TestLargeStringable>&,
              const pro::proxy_indirect_accessor<TestLargeStringable>&>);

} // namespace box_tests_detail

namespace detail = box_tests_detail;

TEST(BoxTests, TestDefaultConstruction) {
  pro::box<detail::TestLargeStringable> b;
  ASSERT_FALSE(b.has_value());
  ASSERT_FALSE(static_cast<bool>(b));
  ASSERT_TRUE(b == std::nullopt);
}

TEST(BoxTests, TestNulloptConstruction) {
  pro::box<detail::TestLargeStringable> b{std::nullopt};
  ASSERT_FALSE(b.has_value());
  ASSERT_TRUE(b == std::nullopt);
}

TEST(BoxTests, TestConstruction_FromValue) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  utils::LifetimeTracker::Session session{&tracker};
  expected_ops.emplace_back(1,
                            utils::LifetimeOperationType::kValueConstruction);
  {
    pro::box<detail::TestLargeStringable> b{session};
    ASSERT_TRUE(b.has_value());
    ASSERT_TRUE(static_cast<bool>(b));
    ASSERT_EQ(ToString(b), "Session 2");
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kCopyConstruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestConstruction_InPlace) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    ASSERT_TRUE(b.has_value());
    ASSERT_EQ(ToString(b), "Session 1");
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestConstruction_InPlaceInitializerList) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b{
        std::in_place_type<utils::LifetimeTracker::Session>,
        {1, 2, 3},
        &tracker};
    ASSERT_TRUE(b.has_value());
    ASSERT_EQ(ToString(b), "Session 1");
    expected_ops.emplace_back(
        1, utils::LifetimeOperationType::kInitializerListConstruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestConstruction_AllocatorArg_FromValue) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  utils::LifetimeTracker::Session session{&tracker};
  expected_ops.emplace_back(1,
                            utils::LifetimeOperationType::kValueConstruction);
  {
    std::pmr::unsynchronized_pool_resource memory_pool;
    pro::box<detail::TestLargeStringable> b{
        std::allocator_arg, std::pmr::polymorphic_allocator<>{&memory_pool},
        session};
    ASSERT_TRUE(b.has_value());
    ASSERT_EQ(ToString(b), "Session 2");
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kCopyConstruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestConstruction_AllocatorArg_InPlace) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    std::pmr::unsynchronized_pool_resource memory_pool;
    pro::box<detail::TestLargeStringable> b{
        std::allocator_arg, std::pmr::polymorphic_allocator<>{&memory_pool},
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    ASSERT_TRUE(b.has_value());
    ASSERT_EQ(ToString(b), "Session 1");
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestConstruction_AllocatorArg_InPlaceInitializerList) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    std::pmr::unsynchronized_pool_resource memory_pool;
    pro::box<detail::TestSmallStringable> b{
        std::allocator_arg,
        std::pmr::polymorphic_allocator<>{&memory_pool},
        std::in_place_type<utils::LifetimeTracker::Session>,
        {1, 2, 3},
        &tracker};
    ASSERT_TRUE(b.has_value());
    ASSERT_EQ(ToString(b), "Session 1");
    expected_ops.emplace_back(
        1, utils::LifetimeOperationType::kInitializerListConstruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestLifetimeModel_Inplace) {
  utils::LifetimeTracker tracker;
  pro::box<detail::TestLargeStringable> b{
      std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
  ASSERT_EQ(b.release().GetLifetimeType(), detail::LifetimeModelType::kInplace);
}

TEST(BoxTests, TestLifetimeModel_Wide) {
  utils::LifetimeTracker tracker;
  pro::box<detail::TestSmallStringable> b{
      std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
  ASSERT_EQ(b.release().GetLifetimeType(), detail::LifetimeModelType::kWide);
}

TEST(BoxTests, TestLifetimeModel_AllocatorArg_Wide) {
  utils::LifetimeTracker tracker;
  std::pmr::unsynchronized_pool_resource memory_pool;
  pro::box<detail::TestLargeStringable> b{
      std::allocator_arg, std::pmr::polymorphic_allocator<>{&memory_pool},
      std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
  ASSERT_EQ(b.release().GetLifetimeType(), detail::LifetimeModelType::kWide);
}

TEST(BoxTests, TestLifetimeModel_AllocatorArg_Compact) {
  utils::LifetimeTracker tracker;
  std::pmr::unsynchronized_pool_resource memory_pool;
  pro::box<detail::TestSmallStringable> b{
      std::allocator_arg, std::pmr::polymorphic_allocator<>{&memory_pool},
      std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
  ASSERT_EQ(b.release().GetLifetimeType(), detail::LifetimeModelType::kCompact);
}

TEST(BoxTests, TestLifetime_Copy) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b1{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    auto b2 = b1;
    ASSERT_TRUE(b1.has_value());
    ASSERT_EQ(ToString(b1), "Session 1");
    ASSERT_TRUE(b2.has_value());
    ASSERT_EQ(ToString(b2), "Session 2");
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kCopyConstruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestLifetime_Move) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b1{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    auto b2 = std::move(b1);
    ASSERT_FALSE(b1.has_value());
    ASSERT_TRUE(b2.has_value());
    ASSERT_EQ(ToString(b2), "Session 2");
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kMoveConstruction);
    expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestLifetime_Reset) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  pro::box<detail::TestLargeStringable> b{
      std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
  expected_ops.emplace_back(1,
                            utils::LifetimeOperationType::kValueConstruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  b.reset();
  ASSERT_FALSE(b.has_value());
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestAssignment_FromValue) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    utils::LifetimeTracker::Session session{&tracker};
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kValueConstruction);
    b = session;
    ASSERT_EQ(ToString(b), "Session 4");
    expected_ops.emplace_back(3,
                              utils::LifetimeOperationType::kCopyConstruction);
    expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
    expected_ops.emplace_back(4,
                              utils::LifetimeOperationType::kMoveConstruction);
    expected_ops.emplace_back(3, utils::LifetimeOperationType::kDestruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  expected_ops.emplace_back(4, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestAssignment_Nullopt) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  pro::box<detail::TestLargeStringable> b{
      std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
  expected_ops.emplace_back(1,
                            utils::LifetimeOperationType::kValueConstruction);
  b = std::nullopt;
  ASSERT_FALSE(b.has_value());
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestAssignment_Copy) {
  pro::box<detail::TestLargeStringable> b1{detail::Widget{1}};
  pro::box<detail::TestLargeStringable> b2;
  b2 = b1;
  ASSERT_TRUE(b1.has_value());
  ASSERT_TRUE(b2.has_value());
  ASSERT_EQ(ToString(b1), "Widget 1");
  ASSERT_EQ(ToString(b2), "Widget 1");
}

TEST(BoxTests, TestAssignment_Move) {
  pro::box<detail::TestLargeStringable> b1{detail::Widget{1}};
  pro::box<detail::TestLargeStringable> b2;
  b2 = std::move(b1);
  ASSERT_FALSE(b1.has_value());
  ASSERT_TRUE(b2.has_value());
  ASSERT_EQ(ToString(b2), "Widget 1");
}

TEST(BoxTests, TestEmplace) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    utils::LifetimeTracker::Session& session =
        b.emplace<utils::LifetimeTracker::Session>(&tracker);
    expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kValueConstruction);
    ASSERT_EQ(to_string(session), "Session 2");
    ASSERT_EQ(ToString(b), "Session 2");
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestEmplace_InitializerList) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b;
    b.emplace<utils::LifetimeTracker::Session>({1, 2, 3}, &tracker);
    expected_ops.emplace_back(
        1, utils::LifetimeOperationType::kInitializerListConstruction);
    ASSERT_EQ(ToString(b), "Session 1");
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestEmplaceAlloc) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    std::pmr::unsynchronized_pool_resource memory_pool;
    pro::box<detail::TestLargeStringable> b;
    b.emplace_alloc<utils::LifetimeTracker::Session>(
        std::pmr::polymorphic_allocator<>{&memory_pool}, &tracker);
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    ASSERT_EQ(ToString(b), "Session 1");
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    ASSERT_EQ(b.release().GetLifetimeType(), detail::LifetimeModelType::kWide);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestEmplaceAlloc_InitializerList) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    std::pmr::unsynchronized_pool_resource memory_pool;
    pro::box<detail::TestSmallStringable> b;
    b.emplace_alloc<utils::LifetimeTracker::Session>(
        std::pmr::polymorphic_allocator<>{&memory_pool}, {1, 2, 3}, &tracker);
    expected_ops.emplace_back(
        1, utils::LifetimeOperationType::kInitializerListConstruction);
    ASSERT_EQ(ToString(b), "Session 1");
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
    ASSERT_EQ(b.release().GetLifetimeType(),
              detail::LifetimeModelType::kCompact);
  }
  expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestSwap) {
  pro::box<detail::TestLargeStringable> b1{detail::Widget{1}};
  pro::box<detail::TestLargeStringable> b2;
  b1.swap(b2);
  ASSERT_FALSE(b1.has_value());
  ASSERT_TRUE(b2.has_value());
  ASSERT_EQ(ToString(b2), "Widget 1");
  swap(b1, b2);
  ASSERT_TRUE(b1.has_value());
  ASSERT_FALSE(b2.has_value());
  ASSERT_EQ(ToString(b1), "Widget 1");
}

TEST(BoxTests, TestRelease) {
  utils::LifetimeTracker tracker;
  std::vector<utils::LifetimeOperation> expected_ops;
  {
    pro::box<detail::TestLargeStringable> b{
        std::in_place_type<utils::LifetimeTracker::Session>, &tracker};
    expected_ops.emplace_back(1,
                              utils::LifetimeOperationType::kValueConstruction);
    pro::proxy<detail::TestLargeStringable> p = b.release();
    ASSERT_FALSE(b.has_value());
    ASSERT_TRUE(p.has_value());
    ASSERT_EQ(ToString(*p), "Session 2");
    expected_ops.emplace_back(2,
                              utils::LifetimeOperationType::kMoveConstruction);
    expected_ops.emplace_back(1, utils::LifetimeOperationType::kDestruction);
    ASSERT_TRUE(tracker.GetOperations() == expected_ops);
  }
  expected_ops.emplace_back(2, utils::LifetimeOperationType::kDestruction);
  ASSERT_TRUE(tracker.GetOperations() == expected_ops);
}

TEST(BoxTests, TestIndirectAccessorConversion) {
  pro::box<detail::TestLargeStringable> b{detail::Widget{7}};
  pro::proxy_indirect_accessor<detail::TestLargeStringable>& ia = b;
  ASSERT_EQ(ToString(ia), "Widget 7");
}

TEST(BoxTests, TestSuperFacadeAccessor) {
  pro::box<detail::TestSizedStringable> b{detail::Widget{42}};
  ASSERT_EQ(b.Size(), 42u);
  ASSERT_EQ(ToString(b), "Widget 42");
}

TEST(BoxTests, TestFacadeConversion_Copy) {
  pro::box<detail::TestSizedStringable> b1{detail::Widget{42}};
  pro::box<detail::TestSized> b2 = b1;
  ASSERT_TRUE(b1.has_value());
  ASSERT_TRUE(b2.has_value());
  ASSERT_EQ(b2.Size(), 42u);
}

TEST(BoxTests, TestFacadeConversion_Move) {
  pro::box<detail::TestSizedStringable> b1{detail::Widget{42}};
  pro::box<detail::TestSized> b2 = std::move(b1);
  ASSERT_FALSE(b1.has_value());
  ASSERT_TRUE(b2.has_value());
  ASSERT_EQ(b2.Size(), 42u);
}

TEST(BoxTests, TestFacadeConversion_Assignment) {
  pro::box<detail::TestSizedStringable> b1{detail::Widget{42}};
  pro::box<detail::TestSized> b2;
  b2 = b1;
  ASSERT_TRUE(b2.has_value());
  ASSERT_EQ(b2.Size(), 42u);
  pro::box<detail::TestSized> b3;
  b3 = std::move(b1);
  ASSERT_FALSE(b1.has_value());
  ASSERT_TRUE(b3.has_value());
  ASSERT_EQ(b3.Size(), 42u);
}

TEST(BoxTests, TestIndirectReflection) {
  pro::box<detail::TestRttiStringable> b{detail::Widget{3}};
  ASSERT_EQ(proxy_typeid(b), typeid(detail::Widget));
  ASSERT_EQ(to_string(proxy_cast<detail::Widget>(b)), "Widget 3");
  ASSERT_EQ(proxy_cast<int>(&b), nullptr);
}
