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

static_assert(std::is_same_v<pro::make_facade<facets::viewable>,
                             BuiltWith<pro::skills::as_view>>);
static_assert(std::is_same_v<pro::make_facade<facets::weakable>,
                             BuiltWith<pro::skills::as_weak>>);
static_assert(std::is_same_v<pro::make_facade<facets::castable>,
                             BuiltWith<pro::skills::rtti>>);
static_assert(std::is_same_v<pro::make_facade<facets::indirect_castable>,
                             BuiltWith<pro::skills::indirect_rtti>>);
static_assert(std::is_same_v<pro::make_facade<facets::direct_castable>,
                             BuiltWith<pro::skills::direct_rtti>>);
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

// Unlike the others, direct_castable spends no convention at all: the
// reflection alone performs the cast.
static_assert(
    std::is_same_v<pro::make_facade<facets::direct_castable>::convention_types,
                   std::tuple<>>);
static_assert(
    std::tuple_size_v<
        pro::make_facade<facets::direct_castable>::reflection_types> == 1u);

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

struct Restricted : pro::make_facade<facets::convention<MemSize, SizeOverload>,
                                     facets::layout<sizeof(void*)>> {};

struct Viewable : pro::make_facade<facets::convention<MemSize, SizeOverload>,
                                   facets::viewable> {};

struct Weakable : pro::make_facade<facets::castable, facets::weakable> {};

struct Callable : pro::make_facade<facets::callable<int(int) const>> {};

struct Indexable : pro::make_facade<facets::indexable<int(std::size_t) const>> {
};

struct SizedRange
    : pro::make_facade<facets::input_range<const int&>, facets::sized> {};

struct Comparable : pro::make_facade<facets::equality_comparable> {};

struct Castable : pro::make_facade<facets::castable, facets::direct_castable> {
};

struct IStreamable : pro::make_facade<facets::istreamable> {};

struct WIStreamable : pro::make_facade<facets::wistreamable> {};

struct OStreamable : pro::make_facade<facets::ostreamable> {};

struct WOStreamable : pro::make_facade<facets::wostreamable> {};

struct Hashable : pro::make_facade<facets::hashable> {};

struct Plain : pro::make_facade<> {};

struct MutableRange : pro::make_facade<facets::input_range<int&>> {};

struct ConstRange : pro::make_facade<facets::input_range<const int&>> {};

struct ValueRange : pro::make_facade<facets::input_range<int>> {};

#ifdef PRO4D_HAS_FORMAT
struct Formattable
    : pro::make_facade<facets::formattable, facets::wformattable> {};

// A facade can be both formattable and iterable; the formattable facet decides
// how it prints.
struct FormattableRange
    : pro::make_facade<facets::formattable, facets::input_range<const char&>> {
};
#endif // PRO4D_HAS_FORMAT

// --- What each facet requires of a target -----------------------------------

static_assert(pro::proxiable<Rect*, Restricted>);
static_assert(pro::proxiable<int*, Comparable>);
static_assert(!pro::proxiable<Rect*, Comparable>);
static_assert(pro::proxiable<std::vector<int>*, Indexable>);
static_assert(!pro::proxiable<int*, Indexable>);
static_assert(pro::proxiable<std::vector<int>*, SizedRange>);
static_assert(!pro::proxiable<std::forward_list<int>*, SizedRange>);
static_assert(!pro::proxiable<std::shared_ptr<Rect>, Restricted>);
static_assert(pro::proxiable<int*, OStreamable>);
static_assert(!pro::proxiable<std::vector<int>*, OStreamable>);
static_assert(pro::proxiable<int*, IStreamable>);
// Extraction writes into the target, so a pointer to const cannot supply it.
static_assert(!pro::proxiable<const int*, IStreamable>);
static_assert(pro::proxiable<const int*, OStreamable>);
static_assert(pro::proxiable<int*, Hashable>);
static_assert(!pro::proxiable<std::vector<int>*, Hashable>);
static_assert(pro::proxiable<std::vector<int>*, MutableRange>);
static_assert(pro::proxiable<std::vector<int>*, ConstRange>);
static_assert(pro::proxiable<std::deque<int>*, ConstRange>);
static_assert(pro::proxiable<std::forward_list<int>*, ConstRange>);
static_assert(!pro::proxiable<int*, ConstRange>);
static_assert(!pro::proxiable<std::vector<std::string>*, ConstRange>);
// A range whose elements are prvalues has nothing to take a reference to, so it
// is only erasable by value.
static_assert(pro::proxiable<std::ranges::iota_view<int, int>*, ValueRange>);
static_assert(!pro::proxiable<std::ranges::iota_view<int, int>*, ConstRange>);

// The element is cached in the iterator and read back from there, so a
// by-value reference type has to be copyable.
template <class T>
concept ValidInputRange = requires { typename facets::input_range<T>; };
static_assert(ValidInputRange<int>);
static_assert(ValidInputRange<std::unique_ptr<int>&>);
static_assert(!ValidInputRange<std::unique_ptr<int>>);

// --- The shape the erased range takes ---------------------------------------

static_assert(
    std::ranges::input_range<pro::proxy_indirect_accessor<ConstRange>>);
static_assert(
    std::input_iterator<
        std::ranges::iterator_t<pro::proxy_indirect_accessor<ConstRange>>>);
// Deliberately no stronger than input: forward would require comparing two
// erased iterators, which is exactly what pairing the iterator with its
// sentinel in one cursor exists to avoid.
static_assert(
    !std::forward_iterator<
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

static_assert(
    std::ranges::sized_range<pro::proxy_indirect_accessor<SizedRange>>);
// equality_comparable carries castable with it.
static_assert(std::is_same_v<
              pro::make_facade<facets::equality_comparable>::reflection_types,
              pro::make_facade<facets::castable>::reflection_types>);

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
// A input_range accessor is a range, but is not formatted as one
static_assert(!std::is_default_constructible_v<
              std::formatter<pro::proxy_indirect_accessor<ConstRange>, char>>);
#endif // PRO4D_HAS_FORMAT

} // namespace proxy_facets_ext_tests_detail

namespace detail = proxy_facets_ext_tests_detail;

TEST(ProxyFacetsExtTests, TestLayout) {
  detail::Rect rect;
  pro::proxy<detail::Restricted> p = &rect;
  ASSERT_EQ(p->Size(), 42u);
  ASSERT_LT(sizeof(pro::proxy<detail::Restricted>),
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

TEST(ProxyFacetsExtTests, TestIndexable) {
  std::vector<int> v{10, 20, 30};
  pro::proxy<detail::Indexable> p = &v;
  ASSERT_EQ((*p)[1u], 20);
}

TEST(ProxyFacetsExtTests, TestSized) {
  const std::vector<int> v{1, 2, 3};
  pro::proxy<detail::SizedRange> p = &v;
  ASSERT_EQ((*p).size(), 3u);
  ASSERT_EQ(std::ranges::size(*p), 3u);
  ASSERT_EQ(std::ranges::distance(*p), 3);
}

TEST(ProxyFacetsExtTests, TestEqualityComparable) {
  int a = 1;
  int b = 1;
  int c = 2;
  double d = 1.0;
  pro::proxy<detail::Comparable> p1 = &a;
  pro::proxy<detail::Comparable> p2 = &b;
  pro::proxy<detail::Comparable> p3 = &c;
  pro::proxy<detail::Comparable> p4 = &d;
  ASSERT_TRUE(*p1 == *p2);
  ASSERT_FALSE(*p1 != *p2);
  ASSERT_TRUE(*p1 != *p3);
  ASSERT_TRUE(*p1 != *p4); // a different contained type never compares equal
  ASSERT_EQ(proxy_cast<int>(*p1), 1); // castable came along
}

TEST(ProxyFacetsExtTests, TestIndirectCastable) {
  int v = 123;
  pro::proxy<detail::Castable> p = &v;
  ASSERT_EQ(proxy_typeid(*p), typeid(int));
  ASSERT_EQ(proxy_cast<int>(*p), 123);
  proxy_cast<int&>(*p) = 456;
  ASSERT_EQ(v, 456);
  ASSERT_EQ(proxy_cast<int>(&*p), &v);
  ASSERT_EQ(proxy_cast<double>(&*p), nullptr);
  ASSERT_THROW(proxy_cast<double>(*p), pro::bad_proxy_cast);
}

TEST(ProxyFacetsExtTests, TestDirectCastableTypeid) {
  int v = 123;
  pro::proxy<detail::Castable> p = &v;
  ASSERT_EQ(proxy_typeid(p), typeid(int*));
}

TEST(ProxyFacetsExtTests, TestDirectCastableCast) {
  int v = 123;
  pro::proxy<detail::Castable> p = &v;
  ASSERT_EQ(proxy_cast<int*>(p), &v);
  ASSERT_EQ(proxy_cast<int*&>(p), &v);
  ASSERT_EQ(proxy_cast<int* const&>(p), &v);
  ASSERT_EQ(*proxy_cast<int*>(&p), &v);
  ASSERT_EQ(proxy_cast<double*>(&p), nullptr);
  static_assert(std::is_same_v<decltype(proxy_cast<int*>(&p)), int**>);
  ASSERT_THROW(proxy_cast<double*>(p), pro::bad_proxy_cast);
  ASSERT_TRUE(p.has_value()); // a failed cast leaves the proxy alone
}

TEST(ProxyFacetsExtTests, TestDirectCastableCastConst) {
  int v = 123;
  pro::proxy<detail::Castable> p = &v;
  const pro::proxy<detail::Castable>& cp = p;
  ASSERT_EQ(proxy_cast<int*>(cp), &v);
  ASSERT_EQ(proxy_cast<int* const&>(cp), &v);
  ASSERT_EQ(*proxy_cast<int* const>(&cp), &v);
  static_assert(
      std::is_same_v<decltype(proxy_cast<int* const>(&cp)), int* const*>);
  // A const operand cannot hand out mutable access to what it contains
  ASSERT_EQ(proxy_cast<int*>(&cp), nullptr);
  ASSERT_THROW(proxy_cast<int*&>(cp), pro::bad_proxy_cast);
}

TEST(ProxyFacetsExtTests, TestDirectCastableCastMove) {
  int v = 123;
  pro::proxy<detail::Castable> p = &v;
  ASSERT_EQ(proxy_cast<int*>(std::move(p)), &v);
  ASSERT_FALSE(p.has_value()); // an rvalue cast consumes the proxy
  pro::proxy<detail::Castable> q = &v;
  ASSERT_THROW(proxy_cast<double*>(std::move(q)), pro::bad_proxy_cast);
  ASSERT_FALSE(q.has_value()); // including when it fails
}

TEST(ProxyFacetsExtTests, TestOStreamable) {
  detail::Rect rect;
  pro::proxy<detail::OStreamable> p = &rect;
  std::ostringstream out;
  out << *p;
  ASSERT_EQ(out.str(), "6x7");
}

TEST(ProxyFacetsExtTests, TestWOStreamable) {
  detail::Rect rect;
  pro::proxy<detail::WOStreamable> p = &rect;
  std::wostringstream out;
  out << *p;
  ASSERT_EQ(out.str(), L"6x7");
}

TEST(ProxyFacetsExtTests, TestIStreamable) {
  int v = 0;
  pro::proxy<detail::IStreamable> p = &v;
  std::istringstream in{"123"};
  in >> *p;
  ASSERT_EQ(v, 123);
}

TEST(ProxyFacetsExtTests, TestWIStreamable) {
  int v = 0;
  pro::proxy<detail::WIStreamable> p = &v;
  std::wistringstream in{L"123"};
  in >> *p;
  ASSERT_EQ(v, 123);
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

TEST(ProxyFacetsExtTests, TestRangeLikePrvalueElements) {
  auto range = std::views::iota(1, 4);
  pro::proxy<detail::ValueRange> p = &range;
  std::vector<int> collected;
  for (int x : *p) {
    collected.push_back(x);
  }
  ASSERT_EQ(collected, (std::vector<int>{1, 2, 3}));
}

TEST(ProxyFacetsExtTests, TestRangeLikeIteratorIsIndependent) {
  const std::vector<int> v{1, 2, 3};
  pro::proxy<detail::ConstRange> p = &v;
  auto it = (*p).begin();
  ASSERT_EQ(*it, 1);
  ASSERT_EQ(*it, 1); // reading twice does not advance
  auto copy = it;
  ++it;
  ASSERT_EQ(*it, 2);
  ASSERT_EQ(*copy, 1);
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
