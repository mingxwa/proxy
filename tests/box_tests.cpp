// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#include "utils.h"
#include <cmath>
#include <gtest/gtest.h>
#include <memory_resource>
#include <optional>
#include <proxy/box.h>

PRO_ENABLE_FREE(Area);

namespace box_tests_detail {

enum LifetimeModelType { kNone, kInplace, kWide, kCompact };

struct LifetimeModelMetadata {
  LifetimeModelMetadata() = default;
  template <class T>
  constexpr explicit LifetimeModelMetadata(
      std::in_place_type_t<pro::detail::inplace_ptr<T>>) noexcept
      : Type(LifetimeModelType::kInplace) {}
  template <class T, class Alloc>
  constexpr explicit LifetimeModelMetadata(
      std::in_place_type_t<pro::detail::wide_ptr<T, Alloc>>) noexcept
      : Type(LifetimeModelType::kWide) {}
  template <class T, class Alloc>
  constexpr explicit LifetimeModelMetadata(
      std::in_place_type_t<pro::detail::compact_ptr<T, Alloc>>) noexcept
      : Type(LifetimeModelType::kCompact) {}
  template <class T>
  constexpr explicit LifetimeModelMetadata(std::in_place_type_t<T>) noexcept
      : Type(LifetimeModelType::kNone) {}

  template <class Self, class M>
  struct accessor {
    LifetimeModelType GetLifetimeType() const noexcept {
      const LifetimeModelMetadata& metadata =
          reflect<M>(static_cast<const Self&>(*this));
      return metadata.Type;
    }
  };

  LifetimeModelType Type;
};

struct TestLargeStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_relocation<pro::lifetime_requirement::supported> //
      ::support_copy<pro::lifetime_requirement::supported>       //
      ::add_direct_reflection<LifetimeModelMetadata>             //
      ::build {};

struct TestSmallStringable : pro::facade_builder               //
                             ::add_facade<TestLargeStringable> //
                             ::restrict_layout<sizeof(void*)>  //
                             ::build {};

PRO_DEF_MEM_DISPATCH(MemSize, Size);

struct TestSized : pro::facade_builder                                        //
                   ::add_convention<MemSize, std::size_t() const>             //
                   ::support_relocation<pro::lifetime_requirement::supported> //
                   ::support_copy<pro::lifetime_requirement::supported>       //
                   ::build {};

struct TestSizedStringable
    : pro::facade_builder                                        //
      ::add_facade<TestSized>                                    //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_relocation<pro::lifetime_requirement::supported> //
      ::support_copy<pro::lifetime_requirement::supported>       //
      ::build {};

struct TestRttiStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_copy<pro::lifetime_requirement::supported>       //
      ::add_skill<pro::skills::rtti>                             //
      ::build {};

struct TestTriviallyCopyableStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_copy<pro::lifetime_requirement::trivial>         //
      ::build {};

struct TestImmovableStringable
    : pro::facade_builder                                        //
      ::add_convention<utils::spec::FreeToString, std::string()> //
      ::support_relocation<pro::lifetime_requirement::none>      //
      ::build {};

#ifdef PRO5D_HAS_FORMAT
struct TestFormattable : pro::facade_builder               //
                         ::add_skill<pro::skills::format>  //
                         ::add_skill<pro::skills::as_view> //
                         ::build {};
#endif // PRO5D_HAS_FORMAT

template <class F, class MP>
using AreEqualSignature = bool(const pro::proxy_indirect_accessor<F, MP>& rhs,
                               double eps) const;

template <class T, pro::facade F, class MP>
bool AreEqualImpl(const T& lhs, const pro::proxy_indirect_accessor<F, MP>& rhs,
                  double eps) {
  return lhs.AreEqual(proxy_cast<const T&>(rhs), eps);
}

PRO_DEF_FREE_AS_MEM_DISPATCH(MemAreEqual, AreEqualImpl, AreEqual);

struct TestEqualableQuantity
    : pro::facade_builder            //
      ::add_skill<pro::skills::rtti> //
      ::add_convention<MemAreEqual,
                       pro::proxy_dependent_signature<AreEqualSignature>> //
      ::build {};

class Point {
public:
  Point(double x, double y) : x_(x), y_(y) {}
  bool AreEqual(const Point& rhs, double eps) const {
    return std::abs(x_ - rhs.x_) < eps && std::abs(y_ - rhs.y_) < eps;
  }

private:
  double x_, y_;
};

struct Square {
  int side;
};

int Area(const Square& square) noexcept { return square.side * square.side; }

struct TestShape
    : pro::facade_builder                                                //
      ::add_convention<pro::free_dispatch<"Area">, int() const noexcept> //
      ::build {};

struct TestViewable
    : pro::facade_builder                                              //
      ::add_convention<pro::operator_dispatch<"+=">, void(int)>        //
      ::add_convention<utils::spec::FreeToString, std::string() const> //
      ::support_copy<pro::lifetime_requirement::supported>             //
      ::add_skill<pro::skills::as_view>                                //
      ::build {};

struct TestViewableSuper
    : pro::facade_builder                                              //
      ::add_convention<utils::spec::FreeToString, std::string() const> //
      ::add_skill<pro::skills::as_view>                                //
      ::build {};

struct TestViewableFromSuper
    : pro::facade_builder                                           //
      ::add_facade<TestViewableSuper>                               //
      ::add_convention<pro::operator_dispatch<"+">, int(int) const> //
      ::build {};

struct TestUnviewableSuper
    : pro::facade_builder                                              //
      ::add_convention<utils::spec::FreeToString, std::string() const> //
      ::build {};

struct TestViewableToSuper
    : pro::facade_builder                                           //
      ::add_facade<TestUnviewableSuper>                             //
      ::add_convention<pro::operator_dispatch<"+">, int(int) const> //
      ::add_skill<pro::skills::as_view>                             //
      ::build {};

struct TestRttiViewable : pro::facade_builder               //
                          ::add_skill<pro::skills::rtti>    //
                          ::add_skill<pro::skills::as_view> //
                          ::build {};

template <class T>
concept Releasable = requires(T v) { v.release(); };

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
static_assert(std::is_same_v<pro::box<TestSized>::metadata_policy_type,
                             pro::compact_metadata>);
static_assert(
    std::is_convertible_v<pro::box<TestLargeStringable>&,
                          pro::proxy_indirect_accessor<TestLargeStringable>&>);
static_assert(std::is_convertible_v<
              const pro::box<TestLargeStringable>&,
              const pro::proxy_indirect_accessor<TestLargeStringable>&>);
static_assert(sizeof(pro::box<TestLargeStringable>) ==
              sizeof(pro::proxy<TestLargeStringable>));
static_assert(alignof(pro::box<TestLargeStringable>) ==
              alignof(pro::proxy<TestLargeStringable>));
static_assert(std::is_copy_constructible_v<pro::box<TestLargeStringable>>);
static_assert(std::is_move_constructible_v<pro::box<TestLargeStringable>>);
static_assert(!std::is_copy_constructible_v<pro::box<utils::spec::Stringable>>);
static_assert(std::is_move_constructible_v<pro::box<utils::spec::Stringable>>);
static_assert(
    std::is_nothrow_default_constructible_v<pro::box<utils::spec::Stringable>>);
static_assert(std::is_constructible_v<pro::box<TestImmovableStringable>, int>);
static_assert(!std::is_move_constructible_v<pro::box<TestImmovableStringable>>);
static_assert(!std::is_assignable_v<pro::box<TestImmovableStringable>&, int>);
static_assert(Releasable<pro::box<TestLargeStringable>>);
static_assert(!Releasable<pro::box<TestImmovableStringable>>);

static_assert(sizeof(pro::box<TestLargeStringable, pro::inline_metadata>) ==
              sizeof(pro::proxy<TestLargeStringable, pro::inline_metadata>));
static_assert(
    std::is_same_v<decltype(std::declval<pro::box<TestLargeStringable,
                                                  pro::inline_metadata>&>()
                                .release()),
                   pro::proxy<TestLargeStringable, pro::inline_metadata>>);
static_assert(!std::is_constructible_v<
              pro::box<TestLargeStringable, pro::inline_metadata>,
              pro::box<TestLargeStringable>>);

static_assert(std::is_nothrow_convertible_v<pro::box<TestViewable>&,
                                            pro::box_view<TestViewable>>);
static_assert(!std::is_convertible_v<pro::box<TestViewable>,
                                     pro::box_view<TestViewable>>);
static_assert(!std::is_convertible_v<const pro::box<TestViewable>&,
                                     pro::box_view<TestViewable>>);
static_assert(!std::is_constructible_v<pro::box_view<TestLargeStringable>,
                                       pro::box<TestLargeStringable>&>);
static_assert(!std::is_constructible_v<pro::box<TestViewable>,
                                       pro::box_view<TestViewable>>);
static_assert(std::is_convertible_v<pro::box<TestViewableToSuper>&,
                                    pro::box_view<TestUnviewableSuper>>);
static_assert(std::is_convertible_v<pro::box_view<TestViewableFromSuper>,
                                    pro::box_view<TestViewableSuper>>);
static_assert(!std::is_convertible_v<pro::box_view<TestViewableSuper>,
                                     pro::box_view<TestViewableFromSuper>>);
static_assert(
    std::is_same_v<pro::box_view<TestViewable>::facade_type, TestViewable>);
static_assert(sizeof(pro::box_view<TestViewable>) ==
              sizeof(pro::proxy_view<TestViewable>));
#if defined(PRO5D_HAS_PAC) || defined(__NVCOMPILER)
static_assert(
    std::is_nothrow_copy_constructible_v<pro::box_view<TestViewable>>);
#else
static_assert(
    std::is_trivially_copy_constructible_v<pro::box_view<TestViewable>>);
#endif // PRO5D_HAS_PAC || __NVCOMPILER
static_assert(std::is_trivially_destructible_v<pro::box_view<TestViewable>>);
static_assert(std::is_convertible_v<
              pro::box_view<TestViewable>&,
              pro::proxy_indirect_accessor<pro::view_facade<TestViewable>>&>);

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

TEST(BoxTests, TestRelease_TriviallyCopyable) {
  pro::box<detail::TestTriviallyCopyableStringable> b{std::in_place_type<int>,
                                                      123};
  pro::proxy<detail::TestTriviallyCopyableStringable> p = b.release();
  ASSERT_FALSE(b.has_value());
  ASSERT_TRUE(p.has_value());
  ASSERT_EQ(ToString(*p), "123");
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

TEST(BoxTests, TestFormat) {
#ifdef PRO5D_HAS_FORMAT
  pro::box<detail::TestFormattable> b = 123;
  ASSERT_EQ(std::format("{}", b), "123");
  ASSERT_EQ(std::format("{:*<6}", b), "123***");
#else
  GTEST_SKIP() << "std::format not available";
#endif // PRO5D_HAS_FORMAT
}

TEST(BoxTests, TestMetadataPolicy) {
  pro::box<detail::TestLargeStringable, pro::inline_metadata> b{
      detail::Widget{5}};
  ASSERT_EQ(ToString(b), "Widget 5");
  pro::proxy<detail::TestLargeStringable, pro::inline_metadata> p = b.release();
  ASSERT_EQ(p.GetLifetimeType(), detail::LifetimeModelType::kInplace);
  ASSERT_EQ(ToString(*p), "Widget 5");
}

TEST(BoxTests, TestDependentSignature) {
  pro::box<detail::TestEqualableQuantity> b1{detail::Point{1, 1}};
  pro::box<detail::TestEqualableQuantity> b2{detail::Point{1, 0}};
  pro::box<detail::TestEqualableQuantity> b3{
      detail::Point{1.0000001, 1.0000001}};
  ASSERT_TRUE(b1.AreEqual(b1, 1e-6));
  ASSERT_FALSE(b1.AreEqual(b2, 1e-6));
  ASSERT_TRUE(b1.AreEqual(b3, 1e-6));
}

TEST(BoxTests, TestNamedAccess) {
  pro::box<detail::TestShape> b{detail::Square{3}};
  ASSERT_EQ(Area(b), 9);
}

TEST(BoxViewTests, TestDefaultConstruction) {
  pro::box_view<detail::TestViewable> v;
  ASSERT_FALSE(v.has_value());
  ASSERT_FALSE(static_cast<bool>(v));
  ASSERT_TRUE(v == std::nullopt);
  pro::box_view<detail::TestViewable> v2{std::nullopt};
  ASSERT_FALSE(v2.has_value());
}

TEST(BoxViewTests, TestViewOfBox) {
  pro::box<detail::TestViewable> b{123};
  pro::box_view<detail::TestViewable> v = b;
  ASSERT_TRUE(v.has_value());
  v += 3;
  ASSERT_EQ(ToString(v), "126");
  ASSERT_EQ(ToString(b), "126");
}

TEST(BoxViewTests, TestViewOfEmptyBox) {
  pro::box<detail::TestViewable> b;
  pro::box_view<detail::TestViewable> v = b;
  ASSERT_FALSE(v.has_value());
}

TEST(BoxViewTests, TestViewOfAllocatedBox) {
  std::pmr::unsynchronized_pool_resource memory_pool;
  pro::box<detail::TestViewable> b{
      std::allocator_arg, std::pmr::polymorphic_allocator<>{&memory_pool},
      std::in_place_type<int>, 7};
  pro::box_view<detail::TestViewable> v = b;
  v += 1;
  ASSERT_EQ(ToString(b), "8");
}

TEST(BoxViewTests, TestViewFromSuperSkill) {
  pro::box<detail::TestViewableFromSuper> b{123};
  pro::box_view<detail::TestViewableFromSuper> v = b;
  ASSERT_EQ(ToString(v), "123");
  ASSERT_EQ(v + 1, 124);
}

TEST(BoxViewTests, TestViewOfSuper) {
  pro::box<detail::TestViewableToSuper> b{123};
  pro::box_view<detail::TestUnviewableSuper> v = b;
  ASSERT_EQ(ToString(v), "123");
  pro::box<detail::TestViewableToSuper> empty;
  pro::box_view<detail::TestUnviewableSuper> v2 = empty;
  ASSERT_FALSE(v2.has_value());
}

TEST(BoxViewTests, TestSuperConversion) {
  pro::box<detail::TestViewableFromSuper> b{123};
  pro::box_view<detail::TestViewableFromSuper> v1 = b;
  pro::box_view<detail::TestViewableSuper> v2 = v1;
  ASSERT_TRUE(v1.has_value());
  ASSERT_EQ(ToString(v2), "123");
  pro::box_view<detail::TestViewableFromSuper> empty;
  pro::box_view<detail::TestViewableSuper> v3 = empty;
  ASSERT_FALSE(v3.has_value());
}

TEST(BoxViewTests, TestCopyAndAssignment) {
  pro::box<detail::TestViewable> b1{1};
  pro::box<detail::TestViewable> b2{2};
  pro::box_view<detail::TestViewable> v1 = b1;
  pro::box_view<detail::TestViewable> v2 = v1;
  ASSERT_EQ(ToString(v2), "1");
  v2 = b2;
  ASSERT_EQ(ToString(v2), "2");
  v2 = std::nullopt;
  ASSERT_FALSE(v2.has_value());
  ASSERT_EQ(ToString(v1), "1");
}

TEST(BoxViewTests, TestResetAndSwap) {
  pro::box<detail::TestViewable> b{1};
  pro::box_view<detail::TestViewable> v1 = b;
  pro::box_view<detail::TestViewable> v2;
  v1.swap(v2);
  ASSERT_FALSE(v1.has_value());
  ASSERT_EQ(ToString(v2), "1");
  v2.reset();
  ASSERT_FALSE(v2.has_value());
  ASSERT_TRUE(b.has_value());
}

TEST(BoxViewTests, TestIndirectAccessorConversion) {
  pro::box<detail::TestViewable> b{7};
  pro::box_view<detail::TestViewable> v = b;
  pro::proxy_indirect_accessor<pro::view_facade<detail::TestViewable>>& ia = v;
  ASSERT_EQ(ToString(ia), "7");
}

TEST(BoxViewTests, TestIndirectReflection) {
  pro::box<detail::TestRttiViewable> b{123};
  pro::box_view<detail::TestRttiViewable> v = b;
  ASSERT_EQ(proxy_typeid(v), typeid(int));
  proxy_cast<int&>(v) = 456;
  ASSERT_EQ(proxy_cast<int>(b), 456);
  ASSERT_EQ(proxy_cast<double>(&v), nullptr);
}

TEST(BoxViewTests, TestFormat) {
#ifdef PRO5D_HAS_FORMAT
  pro::box<detail::TestFormattable> b = 123;
  pro::box_view<detail::TestFormattable> v = b;
  ASSERT_EQ(std::format("{}", v), "123");
  ASSERT_EQ(std::format("{:*<6}", v), "123***");
#else
  GTEST_SKIP() << "std::format not available";
#endif // PRO5D_HAS_FORMAT
}

TEST(BoxViewTests, TestMetadataPolicy) {
  struct ViewableWithInline : pro::facade_builder //
                              ::add_convention<utils::spec::FreeToString,
                                               std::string() const> //
                              ::add_skill<pro::skills::as_view>     //
                              ::build {};
  pro::box<ViewableWithInline, pro::inline_metadata> b{5};
  pro::box_view<ViewableWithInline, pro::inline_metadata> v = b;
  ASSERT_EQ(ToString(v), "5");
  static_assert(!std::is_constructible_v<pro::box_view<ViewableWithInline>,
                                         decltype(b)&>);
}
