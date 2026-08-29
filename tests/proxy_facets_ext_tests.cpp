// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#include <algorithm>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <gtest/gtest.h>
#include <memory>
#include <ostream>
#include <proxy/proxy.h>
#include <ranges>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_set>
#include <vector>

#ifdef PRO4D_HAS_FORMAT
#include <format>
#endif // PRO4D_HAS_FORMAT

namespace proxy_facets_ext_tests_detail {

namespace facets = pro::facets;

template <template <class> class Skill>
using BuiltWith = typename pro::facade_builder::add_skill<Skill>::build;

// --- The facets describe the same facades the skills built ------------------

static_assert(std::is_same_v<pro::make_facade<facets::slim>,
                             BuiltWith<pro::skills::slim>>);
static_assert(std::is_same_v<pro::make_facade<facets::viewable>,
                             BuiltWith<pro::skills::as_view>>);
static_assert(std::is_same_v<pro::make_facade<facets::weakable>,
                             BuiltWith<pro::skills::as_weak>>);
static_assert(std::is_same_v<pro::make_facade<facets::rtti>,
                             BuiltWith<pro::skills::rtti>>);
static_assert(std::is_same_v<pro::make_facade<facets::indirect_rtti>,
                             BuiltWith<pro::skills::indirect_rtti>>);
#ifdef PRO4D_HAS_FORMAT
static_assert(std::is_same_v<pro::make_facade<facets::formattable>,
                             BuiltWith<pro::skills::format>>);
static_assert(std::is_same_v<pro::make_facade<facets::wformattable>,
                             BuiltWith<pro::skills::wformat>>);
#endif // PRO4D_HAS_FORMAT
static_assert(
    std::is_same_v<pro::make_facade<facets::callable<int(int) const>>,
                   pro::facade_builder::add_convention<
                       pro::operator_dispatch<"()">, int(int) const>::build>);

// direct_rtti is the one that does not: the skill spends three conventions on
// a cast the reflection alone can perform.
static_assert(
    std::tuple_size_v<BuiltWith<pro::skills::direct_rtti>::convention_types> ==
    3u);
static_assert(
    std::is_same_v<pro::make_facade<facets::direct_rtti>::convention_types,
                   std::tuple<>>);
static_assert(std::tuple_size_v<
                  pro::make_facade<facets::direct_rtti>::reflection_types> ==
              1u);

// --- Fixtures ---------------------------------------------------------------

struct Rect {
  std::size_t Size() const { return width_ * height_; }
  int operator()(int scale) const {
    return static_cast<int>(width_ * height_) * scale;
  }
  friend std::ostream& operator<<(std::ostream& out, const Rect& self) {
    return out << self.width_ << "x" << self.height_;
  }
  friend std::wostream& operator<<(std::wostream& out, const Rect& self) {
    return out << self.width_ << L"x" << self.height_;
  }

  std::size_t width_ = 6u;
  std::size_t height_ = 7u;
};

PRO_DEF_MEM_DISPATCH(MemSize, Size);

using SizeOverload = std::size_t() const;

struct Sized : pro::make_facade<facets::convention<MemSize, SizeOverload>> {};

struct Slim : pro::make_facade<facets::convention<MemSize, SizeOverload>,
                               facets::slim> {};

struct Viewable : pro::make_facade<facets::convention<MemSize, SizeOverload>,
                                   facets::viewable> {};

struct Weakable : pro::make_facade<facets::rtti, facets::weakable> {};

struct Callable : pro::make_facade<facets::callable<int(int) const>> {};

struct Rtti : pro::make_facade<facets::rtti, facets::direct_rtti> {};

struct Serializable : pro::make_facade<facets::serializable> {};

struct WSerializable : pro::make_facade<facets::wserializable> {};

struct Hashable : pro::make_facade<facets::hashable> {};

struct Plain : pro::make_facade<> {};

struct MutableRange : pro::make_facade<facets::range_like<int&>> {};

struct ConstRange : pro::make_facade<facets::range_like<const int&>> {};

struct ValueRange : pro::make_facade<facets::range_like<int>> {};

#ifdef PRO4D_HAS_FORMAT
struct Formattable
    : pro::make_facade<facets::formattable, facets::wformattable> {};

// A facade can be both formattable and iterable; the formattable facet decides
// how it prints.
struct FormattableRange
    : pro::make_facade<facets::formattable, facets::range_like<const char&>> {};
#endif // PRO4D_HAS_FORMAT

// --- What each facet requires of a target -----------------------------------

static_assert(pro::proxiable<Rect*, Slim>);
static_assert(!pro::proxiable<std::shared_ptr<Rect>, Slim>);
static_assert(pro::proxiable<int*, Serializable>);
static_assert(!pro::proxiable<std::vector<int>*, Serializable>);
static_assert(pro::proxiable<int*, Hashable>);
static_assert(!pro::proxiable<std::vector<int>*, Hashable>);
static_assert(pro::proxiable<std::vector<int>*, MutableRange>);
static_assert(pro::proxiable<std::vector<int>*, ConstRange>);
static_assert(pro::proxiable<std::deque<int>*, ConstRange>);
static_assert(pro::proxiable<std::forward_list<int>*, ConstRange>);
static_assert(!pro::proxiable<int*, ConstRange>);
static_assert(!pro::proxiable<std::vector<std::string>*, ConstRange>);

// --- The shape the erased range takes ---------------------------------------

static_assert(
    std::ranges::input_range<pro::proxy_indirect_accessor<ConstRange>>);
static_assert(
    std::input_iterator<
        std::ranges::iterator_t<pro::proxy_indirect_accessor<ConstRange>>>);
static_assert(std::is_same_v<std::ranges::range_reference_t<
                                 pro::proxy_indirect_accessor<ConstRange>>,
                             const int&>);
static_assert(
    std::is_same_v<
        std::ranges::range_value_t<pro::proxy_indirect_accessor<ConstRange>>,
        int>);
// A mutable reference type is only reachable from a non-const operand, so only
// that facet gives up const iteration.
static_assert(
    std::ranges::input_range<const pro::proxy_indirect_accessor<ConstRange>&>);
static_assert(
    !std::ranges::range<const pro::proxy_indirect_accessor<MutableRange>&>);

// --- Which std specializations the facets enable ----------------------------

static_assert(std::is_default_constructible_v<
              std::hash<pro::proxy_indirect_accessor<Hashable>>>);
static_assert(!std::is_default_constructible_v<
              std::hash<pro::proxy_indirect_accessor<Plain>>>);
#ifdef PRO4D_HAS_FORMAT
static_assert(std::is_default_constructible_v<
              std::formatter<pro::proxy_indirect_accessor<Formattable>, char>>);
static_assert(
    std::is_default_constructible_v<
        std::formatter<pro::proxy_indirect_accessor<Formattable>, wchar_t>>);
static_assert(!std::is_default_constructible_v<
              std::formatter<pro::proxy_indirect_accessor<Plain>, char>>);
static_assert(
    std::is_default_constructible_v<
        std::formatter<pro::proxy_indirect_accessor<FormattableRange>, char>>);
// A range_like accessor is a range, but is not formatted as one
static_assert(!std::is_default_constructible_v<
              std::formatter<pro::proxy_indirect_accessor<ConstRange>, char>>);
#endif // PRO4D_HAS_FORMAT

} // namespace proxy_facets_ext_tests_detail

namespace detail = proxy_facets_ext_tests_detail;

TEST(ProxyFacetsExtTests, TestSlim) {
  detail::Rect rect;
  pro::proxy<detail::Slim> p = &rect;
  ASSERT_EQ(p->Size(), 42u);
  ASSERT_LT(sizeof(pro::proxy<detail::Slim>),
            sizeof(pro::proxy<detail::Sized>));
}

TEST(ProxyFacetsExtTests, TestViewable) {
  detail::Rect rect;
  pro::proxy<detail::Viewable> p = &rect;
  pro::proxy_view<detail::Viewable> v = p;
  ASSERT_EQ(v->Size(), 42u);
  rect.width_ = 1u;
  ASSERT_EQ(v->Size(), 7u);
}

TEST(ProxyFacetsExtTests, TestWeakable) {
  pro::proxy<detail::Weakable> p1 =
      pro::make_proxy_shared<detail::Weakable>(123);
  pro::weak_proxy<detail::Weakable> p2 = p1;
  pro::proxy<detail::Weakable> p3 = p2.lock();
  ASSERT_TRUE(p3.has_value());
  ASSERT_EQ(proxy_cast<int>(*p3), 123);
  p1.reset();
  p3.reset();
  ASSERT_FALSE(p2.lock().has_value());
}

TEST(ProxyFacetsExtTests, TestCallable) {
  detail::Rect rect;
  pro::proxy<detail::Callable> p = &rect;
  ASSERT_EQ((*p)(2), 84);
}

TEST(ProxyFacetsExtTests, TestIndirectRtti) {
  int v = 123;
  pro::proxy<detail::Rtti> p = &v;
  ASSERT_EQ(proxy_typeid(*p), typeid(int));
  ASSERT_EQ(proxy_cast<int>(*p), 123);
  proxy_cast<int&>(*p) = 456;
  ASSERT_EQ(v, 456);
  ASSERT_EQ(proxy_cast<int>(&*p), &v);
  ASSERT_EQ(proxy_cast<double>(&*p), nullptr);
  ASSERT_THROW(proxy_cast<double>(*p), pro::bad_proxy_cast);
}

TEST(ProxyFacetsExtTests, TestDirectRttiTypeid) {
  int v = 123;
  pro::proxy<detail::Rtti> p = &v;
  ASSERT_EQ(proxy_typeid(p), typeid(int*));
}

TEST(ProxyFacetsExtTests, TestDirectRttiCast) {
  int v = 123;
  pro::proxy<detail::Rtti> p = &v;
  ASSERT_EQ(proxy_cast<int*>(p), &v);
  ASSERT_EQ(proxy_cast<int*&>(p), &v);
  ASSERT_EQ(proxy_cast<int* const&>(p), &v);
  ASSERT_EQ(*proxy_cast<int*>(&p), &v);
  ASSERT_EQ(proxy_cast<double*>(&p), nullptr);
  static_assert(std::is_same_v<decltype(proxy_cast<int*>(&p)), int**>);
  ASSERT_THROW(proxy_cast<double*>(p), pro::bad_proxy_cast);
  ASSERT_TRUE(p.has_value()); // a failed cast leaves the proxy alone
}

TEST(ProxyFacetsExtTests, TestDirectRttiCastConst) {
  int v = 123;
  pro::proxy<detail::Rtti> p = &v;
  const pro::proxy<detail::Rtti>& cp = p;
  ASSERT_EQ(proxy_cast<int*>(cp), &v);
  ASSERT_EQ(proxy_cast<int* const&>(cp), &v);
  ASSERT_EQ(*proxy_cast<int*>(&cp), &v);
  static_assert(std::is_same_v<decltype(proxy_cast<int*>(&cp)), int* const*>);
  // A const operand cannot hand out a mutable reference to what it contains
  ASSERT_THROW(proxy_cast<int*&>(cp), pro::bad_proxy_cast);
}

TEST(ProxyFacetsExtTests, TestDirectRttiCastMove) {
  int v = 123;
  pro::proxy<detail::Rtti> p = &v;
  ASSERT_THROW(proxy_cast<double*>(std::move(p)), pro::bad_proxy_cast);
  ASSERT_TRUE(p.has_value());
  ASSERT_EQ(proxy_cast<int*>(std::move(p)), &v);
  ASSERT_FALSE(p.has_value()); // a successful move cast consumes the proxy
}

TEST(ProxyFacetsExtTests, TestSerializable) {
  detail::Rect rect;
  pro::proxy<detail::Serializable> p = &rect;
  std::ostringstream out;
  out << *p;
  ASSERT_EQ(out.str(), "6x7");
}

TEST(ProxyFacetsExtTests, TestWSerializable) {
  detail::Rect rect;
  pro::proxy<detail::WSerializable> p = &rect;
  std::wostringstream out;
  out << *p;
  ASSERT_EQ(out.str(), L"6x7");
}

TEST(ProxyFacetsExtTests, TestHashable) {
  int v = 123;
  pro::proxy<detail::Hashable> p = &v;
  ASSERT_EQ(std::hash<pro::proxy_indirect_accessor<detail::Hashable>>{}(*p),
            std::hash<int>{}(123));
  std::unordered_set<std::size_t> hashes;
  for (int i = 0; i < 4; ++i) {
    v = i;
    hashes.insert(
        std::hash<pro::proxy_indirect_accessor<detail::Hashable>>{}(*p));
  }
  ASSERT_EQ(hashes.size(), 4u);
}

TEST(ProxyFacetsExtTests, TestRangeLikeMutable) {
  std::vector<int> v{1, 2, 3};
  pro::proxy<detail::MutableRange> p = &v;
  for (int& x : *p) {
    x *= 10;
  }
  ASSERT_EQ(v, (std::vector<int>{10, 20, 30}));
}

TEST(ProxyFacetsExtTests, TestRangeLikeConst) {
  const std::vector<int> v{1, 2, 3};
  pro::proxy<detail::ConstRange> p = &v;
  std::vector<int> collected;
  for (const int& x : *p) {
    collected.push_back(x);
  }
  ASSERT_EQ(collected, v);
  ASSERT_EQ(std::ranges::count(*p, 2), 1);
}

TEST(ProxyFacetsExtTests, TestRangeLikeValueReference) {
  std::vector<int> v{1, 2, 3};
  pro::proxy<detail::ValueRange> p = &v;
  int sum = 0;
  for (int x : *p) {
    sum += x;
  }
  ASSERT_EQ(sum, 6);
}

TEST(ProxyFacetsExtTests, TestRangeLikeSwitchesTarget) {
  std::vector<int> v{1, 2, 3};
  // A deque iterator does not fit the inline storage of the erased cursor
  std::deque<int> d{4, 5};
  // A forward_list is not a common range
  std::forward_list<int> fl{6};
  std::vector<int> collected;
  for (auto&& range :
       {pro::proxy<detail::ConstRange>{&v}, pro::proxy<detail::ConstRange>{&d},
        pro::proxy<detail::ConstRange>{&fl}}) {
    for (const int& x : *range) {
      collected.push_back(x);
    }
  }
  ASSERT_EQ(collected, (std::vector<int>{1, 2, 3, 4, 5, 6}));
}

TEST(ProxyFacetsExtTests, TestRangeLikeEmpty) {
  const std::vector<int> v;
  pro::proxy<detail::ConstRange> p = &v;
  ASSERT_TRUE((*p).begin() == (*p).end());
}

TEST(ProxyFacetsExtTests, TestFormattable) {
#ifdef PRO4D_HAS_FORMAT
  int v = 123;
  pro::proxy<detail::Formattable> p = &v;
  ASSERT_EQ(std::format("{}", *p), "123");
  ASSERT_EQ(std::format("{:*<6}", *p), "123***");
  ASSERT_EQ(std::format(L"{}", *p), L"123");
#else
  GTEST_SKIP() << "std::format not available";
#endif // PRO4D_HAS_FORMAT
}

TEST(ProxyFacetsExtTests, TestFormattableRange) {
#ifdef PRO4D_HAS_FORMAT
  std::string s = "abc";
  pro::proxy<detail::FormattableRange> p = &s;
  // The formattable facet decides how it prints, not the range it also models
  ASSERT_EQ(std::format("{}", *p), "abc");
  std::string collected;
  for (const char& c : *p) {
    collected.push_back(c);
  }
  ASSERT_EQ(collected, "abc");
#else
  GTEST_SKIP() << "std::format not available";
#endif // PRO4D_HAS_FORMAT
}
