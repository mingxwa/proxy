// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#include "utils.h"
#include <cstddef>
#include <gtest/gtest.h>
#include <memory>
#include <proxy/proxy.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>

namespace proxy_facets_tests_detail {

namespace facets = pro::facets;
using CL = pro::constraint_level;

PRO_DEF_MEM_DISPATCH(MemSize, Size);
PRO_DEF_MEM_DISPATCH(MemClear, Clear);
PRO_DEF_MEM_DISPATCH(MemUseCount, use_count);

using SizeOverload = std::size_t() const;
using ClearOverload = void();
using UseCountOverload = long() const noexcept;
using StringOverload = std::string();

template <class D, class O>
using IndirectConv = pro::detail::conv_impl<false, D, O>;
template <class D, class O>
using DirectConv = pro::detail::conv_impl<true, D, O>;
template <class R>
using IndirectRefl = pro::detail::refl_impl<false, R>;
template <class R>
using DirectRefl = pro::detail::refl_impl<true, R>;

inline constexpr std::size_t kDefaultSize = 2 * sizeof(void*);
inline constexpr std::size_t kDefaultAlign = alignof(void*);
inline constexpr std::size_t kMaxAlign = alignof(std::max_align_t);

// A facade is fully described by the eight ProFacade properties, so spelling
// all of them out proves both that a facet fills its own slot and that it
// leaves every other slot alone.
template <class F, class Ss, class Cs, class Rs, std::size_t MaxSize,
          std::size_t MaxAlign, CL Copyability, CL Relocatability,
          CL Destructibility>
concept denotes_facade =
    pro::facade<F> && std::is_same_v<typename F::super_types, Ss> &&
    std::is_same_v<typename F::convention_types, Cs> &&
    std::is_same_v<typename F::reflection_types, Rs> &&
    F::max_size == MaxSize && F::max_align == MaxAlign &&
    F::copyability == Copyability && F::relocatability == Relocatability &&
    F::destructibility == Destructibility;

// Every facet reduces to a canonical primitive, which makes it the natural
// yardstick for "these two spellings mean the same thing".
template <class L, class R>
concept same_facet = std::is_same_v<pro::detail::facet_primitive_t<L>,
                                    pro::detail::facet_primitive_t<R>>;

template <class D, class... Os>
concept convention_well_formed =
    requires { typename facets::convention<D, Os...>; };
template <class D, class... Os>
concept direct_convention_well_formed =
    requires { typename facets::direct_convention<D, Os...>; };
template <class R>
concept reflection_well_formed = requires { typename facets::reflection<R>; };
template <class F>
concept super_well_formed = requires { typename facets::super<F>; };
template <CL Level>
concept copyability_well_formed =
    requires { typename facets::copyability<Level>; };
template <std::size_t Size, std::size_t Align>
concept layout_well_formed = requires { typename facets::layout<Size, Align>; };
template <class... Fs>
concept pack_well_formed = requires { typename facets::pack<Fs...>; };

// A facade that owes nothing to the facets machinery, for the cases that need
// to tell facades and facets apart.
struct HandWrittenFacade {
  using super_types = std::tuple<>;
  using convention_types = std::tuple<IndirectConv<MemSize, SizeOverload>>;
  using reflection_types = std::tuple<>;
  static constexpr std::size_t max_size = sizeof(void*);
  static constexpr std::size_t max_align = alignof(void*);
  static constexpr CL copyability = CL::nontrivial;
  static constexpr CL relocatability = CL::nothrow;
  static constexpr CL destructibility = CL::nothrow;
};
static_assert(pro::facade<HandWrittenFacade>);

struct DerivedConvention : facets::convention<MemSize, SizeOverload> {};
struct TwiceDerived : DerivedConvention {};
struct NotAFacet {};
struct TwoFacets : facets::layout<32>, facets::copyability<CL::none> {};
struct SameLevelTwice : facets::copyability<CL::nothrow>,
                        facets::relocatability<CL::nothrow> {};

// --- Which types are facets ------------------------------------------------

static_assert(facets::facet<facets::pack<>>);
static_assert(facets::facet<facets::super<HandWrittenFacade>>);
static_assert(facets::facet<facets::convention<MemSize, SizeOverload>>);
static_assert(
    facets::facet<facets::indirect_convention<MemSize, SizeOverload>>);
static_assert(facets::facet<facets::direct_convention<MemSize, SizeOverload>>);
static_assert(facets::facet<facets::reflection<utils::RttiReflector>>);
static_assert(facets::facet<facets::indirect_reflection<utils::RttiReflector>>);
static_assert(facets::facet<facets::direct_reflection<utils::RttiReflector>>);
static_assert(facets::facet<facets::copyability<CL::nothrow>>);
static_assert(facets::facet<facets::relocatability<CL::nothrow>>);
static_assert(facets::facet<facets::destructibility<CL::nothrow>>);
static_assert(facets::facet<facets::layout<32>>);
// Inheriting from a facet, at any depth, yields a facet
static_assert(facets::facet<DerivedConvention>);
static_assert(facets::facet<TwiceDerived>);
// Anything else is not
static_assert(!facets::facet<void>);
static_assert(!facets::facet<int>);
static_assert(!facets::facet<int&>);
static_assert(!facets::facet<int[3]>);
static_assert(!facets::facet<NotAFacet>);
static_assert(!facets::facet<HandWrittenFacade>);
// Nor is a type that inherits two facets, even two of the same level
static_assert(!facets::facet<TwoFacets>);
static_assert(!facets::facet<SameLevelTwice>);

// --- What each facet normalizes to -----------------------------------------

static_assert(
    std::is_same_v<
        pro::detail::facet_primitive_t<
            facets::convention<MemSize, SizeOverload>>,
        pro::detail::convention_facet_primitive<false, MemSize, SizeOverload>>);
static_assert(
    std::is_same_v<
        pro::detail::facet_primitive_t<
            facets::direct_convention<MemSize, SizeOverload>>,
        pro::detail::convention_facet_primitive<true, MemSize, SizeOverload>>);
static_assert(
    std::is_same_v<
        pro::detail::facet_primitive_t<
            facets::reflection<utils::RttiReflector>>,
        pro::detail::reflection_facet_primitive<false, utils::RttiReflector>>);
static_assert(
    std::is_same_v<
        pro::detail::facet_primitive_t<
            facets::direct_reflection<utils::RttiReflector>>,
        pro::detail::reflection_facet_primitive<true, utils::RttiReflector>>);
static_assert(std::is_same_v<
              pro::detail::facet_primitive_t<facets::copyability<CL::nothrow>>,
              pro::detail::copyability_facet_primitive<CL::nothrow>>);
static_assert(
    std::is_same_v<pro::detail::facet_primitive_t<facets::layout<32, 8>>,
                   pro::detail::layout_facet_primitive<32, 8>>);
static_assert(std::is_same_v<
              pro::detail::facet_primitive_t<facets::super<HandWrittenFacade>>,
              pro::detail::super_facet_primitive<HandWrittenFacade>>);

// `convention` and `reflection` mean their indirect flavors
static_assert(same_facet<facets::convention<MemSize, SizeOverload>,
                         facets::indirect_convention<MemSize, SizeOverload>>);
static_assert(same_facet<facets::reflection<utils::RttiReflector>,
                         facets::indirect_reflection<utils::RttiReflector>>);
// A derived facet normalizes to the same primitive as the facet it derives from
static_assert(
    same_facet<TwiceDerived, facets::convention<MemSize, SizeOverload>>);
// A multi-overload convention means a pack of single-overload conventions
static_assert(
    same_facet<
        facets::convention<MemSize, SizeOverload, std::size_t() &>,
        facets::pack<facets::indirect_convention<MemSize, SizeOverload>,
                     facets::indirect_convention<MemSize, std::size_t() &>>>);
// `layout` derives its alignment from its size, capped at the widest scalar
static_assert(std::is_same_v<pro::detail::facet_primitive_t<facets::layout<12>>,
                             pro::detail::layout_facet_primitive<12, 4>>);
static_assert(
    std::is_same_v<pro::detail::facet_primitive_t<facets::layout<64>>,
                   pro::detail::layout_facet_primitive<64, kMaxAlign>>);
// The three constraint facets stay distinct despite carrying the same level
static_assert(!same_facet<facets::copyability<CL::nothrow>,
                          facets::relocatability<CL::nothrow>>);
static_assert(!same_facet<facets::relocatability<CL::nothrow>,
                          facets::destructibility<CL::nothrow>>);

// --- What facade a pack denotes --------------------------------------------

// An empty pack denotes the default facade
static_assert(denotes_facade<facets::pack<>, std::tuple<>, std::tuple<>,
                             std::tuple<>, kDefaultSize, kDefaultAlign,
                             CL::none, CL::trivial, CL::nothrow>);
// Each facet fills its own slot and disturbs nothing else
static_assert(
    denotes_facade<
        facets::pack<facets::convention<MemSize, SizeOverload>>, std::tuple<>,
        std::tuple<IndirectConv<MemSize, SizeOverload>>, std::tuple<>,
        kDefaultSize, kDefaultAlign, CL::none, CL::trivial, CL::nothrow>);
static_assert(denotes_facade<
              facets::pack<facets::direct_convention<MemSize, SizeOverload>>,
              std::tuple<>, std::tuple<DirectConv<MemSize, SizeOverload>>,
              std::tuple<>, kDefaultSize, kDefaultAlign, CL::none, CL::trivial,
              CL::nothrow>);
static_assert(
    denotes_facade<facets::pack<facets::reflection<utils::RttiReflector>>,
                   std::tuple<>, std::tuple<>,
                   std::tuple<IndirectRefl<utils::RttiReflector>>, kDefaultSize,
                   kDefaultAlign, CL::none, CL::trivial, CL::nothrow>);
static_assert(denotes_facade<
              facets::pack<facets::direct_reflection<utils::RttiReflector>>,
              std::tuple<>, std::tuple<>,
              std::tuple<DirectRefl<utils::RttiReflector>>, kDefaultSize,
              kDefaultAlign, CL::none, CL::trivial, CL::nothrow>);
static_assert(denotes_facade<facets::pack<facets::layout<32, 8>>, std::tuple<>,
                             std::tuple<>, std::tuple<>, 32u, 8u, CL::none,
                             CL::trivial, CL::nothrow>);
static_assert(
    denotes_facade<facets::pack<facets::copyability<CL::nontrivial>>,
                   std::tuple<>, std::tuple<>, std::tuple<>, kDefaultSize,
                   kDefaultAlign, CL::nontrivial, CL::trivial, CL::nothrow>);
static_assert(
    denotes_facade<facets::pack<facets::relocatability<CL::nontrivial>>,
                   std::tuple<>, std::tuple<>, std::tuple<>, kDefaultSize,
                   kDefaultAlign, CL::none, CL::nontrivial, CL::nothrow>);
static_assert(
    denotes_facade<facets::pack<facets::destructibility<CL::nontrivial>>,
                   std::tuple<>, std::tuple<>, std::tuple<>, kDefaultSize,
                   kDefaultAlign, CL::none, CL::trivial, CL::nontrivial>);

// --- How repeated facets merge ---------------------------------------------

// The layout narrows to the tightest requirement, whichever order it is given
static_assert(facets::pack<facets::layout<64>, facets::layout<32>>::max_size ==
              32u);
static_assert(facets::pack<facets::layout<32>, facets::layout<64>>::max_size ==
              32u);
static_assert(
    facets::pack<facets::layout<64, 8>, facets::layout<64, 4>>::max_align ==
    4u);
// The constraints widen to the strongest requirement, whichever order
static_assert(facets::pack<facets::copyability<CL::nontrivial>,
                           facets::copyability<CL::trivial>>::copyability ==
              CL::trivial);
static_assert(facets::pack<facets::copyability<CL::trivial>,
                           facets::copyability<CL::nontrivial>>::copyability ==
              CL::trivial);
// Conventions and reflections accumulate in order, and duplicates are kept
static_assert(
    denotes_facade<facets::pack<facets::convention<MemSize, SizeOverload>,
                                facets::convention<MemClear, ClearOverload>,
                                facets::convention<MemSize, SizeOverload>>,
                   std::tuple<>,
                   std::tuple<IndirectConv<MemSize, SizeOverload>,
                              IndirectConv<MemClear, ClearOverload>,
                              IndirectConv<MemSize, SizeOverload>>,
                   std::tuple<>, kDefaultSize, kDefaultAlign, CL::none,
                   CL::trivial, CL::nothrow>);
static_assert(denotes_facade<
              facets::pack<facets::direct_reflection<utils::RttiReflector>,
                           facets::reflection<utils::RttiReflector>>,
              std::tuple<>, std::tuple<>,
              std::tuple<DirectRefl<utils::RttiReflector>,
                         IndirectRefl<utils::RttiReflector>>,
              kDefaultSize, kDefaultAlign, CL::none, CL::trivial, CL::nothrow>);
// A multi-overload convention expands in the order its overloads were written
static_assert(denotes_facade<
              facets::pack<facets::convention<
                  MemSize, SizeOverload, std::size_t() &, std::size_t() &&>>,
              std::tuple<>,
              std::tuple<IndirectConv<MemSize, SizeOverload>,
                         IndirectConv<MemSize, std::size_t() &>,
                         IndirectConv<MemSize, std::size_t() &&>>,
              std::tuple<>, kDefaultSize, kDefaultAlign, CL::none, CL::trivial,
              CL::nothrow>);

// --- Nesting: a pack is both a facade and a pack of facets ------------------

// Nested packs flatten, at any depth and in any position
static_assert(
    same_facet<
        facets::pack<facets::pack<facets::convention<MemSize, SizeOverload>>,
                     facets::layout<32>>,
        facets::pack<facets::convention<MemSize, SizeOverload>,
                     facets::layout<32>>>);
static_assert(same_facet<
              facets::pack<facets::pack<facets::pack<facets::layout<32>>,
                                        facets::copyability<CL::nothrow>>,
                           facets::relocatability<CL::nothrow>>,
              facets::pack<facets::layout<32>, facets::copyability<CL::nothrow>,
                           facets::relocatability<CL::nothrow>>>);
// A nested pack contributes what it specified, never the defaults it would
// have applied had it been used as a facade on its own
static_assert(facets::pack<facets::layout<32>>::max_size == 32u);
static_assert(facets::pack<facets::pack<facets::layout<32>>>::max_size == 32u);
static_assert(
    facets::pack<facets::convention<MemSize, SizeOverload>>::max_size ==
    kDefaultSize);
static_assert(
    denotes_facade<
        facets::pack<facets::pack<facets::convention<MemSize, SizeOverload>>,
                     facets::layout<32>>,
        std::tuple<>, std::tuple<IndirectConv<MemSize, SizeOverload>>,
        std::tuple<>, 32u, kMaxAlign, CL::none, CL::trivial, CL::nothrow>);
// A facade defined by deriving from a pack keeps both roles
struct Sized : facets::pack<facets::convention<MemSize, SizeOverload>,
                            facets::layout<32>> {};
static_assert(pro::facade<Sized>);
static_assert(facets::facet<Sized>);
static_assert(same_facet<facets::pack<Sized>,
                         facets::pack<facets::convention<MemSize, SizeOverload>,
                                      facets::layout<32>>>);

// --- super names a facade instead of flattening it --------------------------

// `super` adds the facade itself and merges the properties it settled on
static_assert(denotes_facade<facets::pack<facets::super<Sized>>,
                             std::tuple<Sized>, std::tuple<>, std::tuple<>, 32u,
                             kMaxAlign, CL::none, CL::trivial, CL::nothrow>);
// which is exactly what nesting the same facade as a pack does not do
static_assert(denotes_facade<facets::pack<Sized>, std::tuple<>,
                             std::tuple<IndirectConv<MemSize, SizeOverload>>,
                             std::tuple<>, 32u, kMaxAlign, CL::none,
                             CL::trivial, CL::nothrow>);
// `super` accepts any facade, not just one built out of facets
static_assert(denotes_facade<facets::pack<facets::super<HandWrittenFacade>>,
                             std::tuple<HandWrittenFacade>, std::tuple<>,
                             std::tuple<>, sizeof(void*), alignof(void*),
                             CL::nontrivial, CL::nothrow, CL::nothrow>);

// --- Ill-formed facet arguments are rejected --------------------------------

static_assert(convention_well_formed<MemSize, SizeOverload>);
static_assert(direct_convention_well_formed<MemSize, SizeOverload>);
static_assert(!convention_well_formed<MemSize>);
static_assert(!direct_convention_well_formed<MemSize>);
static_assert(!convention_well_formed<MemSize, int>);
static_assert(!convention_well_formed<MemSize, SizeOverload, int>);
static_assert(reflection_well_formed<utils::RttiReflector>);
static_assert(!reflection_well_formed<void>);
static_assert(!reflection_well_formed<int&>);
static_assert(super_well_formed<HandWrittenFacade>);
static_assert(!super_well_formed<int>);
static_assert(!super_well_formed<NotAFacet>);
static_assert(copyability_well_formed<CL::none>);
static_assert(copyability_well_formed<CL::trivial>);
static_assert(!copyability_well_formed<static_cast<CL>(-1)>);
static_assert(!copyability_well_formed<static_cast<CL>(4)>);
static_assert(layout_well_formed<32, 8>);
static_assert(!layout_well_formed<0, 1>);  // size must be positive
static_assert(!layout_well_formed<12, 8>); // size must be a multiple of align
static_assert(!layout_well_formed<32, 3>); // align must be a power of two
static_assert(pack_well_formed<facets::layout<32>>);
static_assert(!pack_well_formed<int>);
static_assert(!pack_well_formed<facets::layout<32>, NotAFacet>);

// --- Fixtures for the runtime tests -----------------------------------------

struct Rect {
  std::size_t Size() const { return width_ * height_; }
  void Clear() { width_ = height_ = 0u; }
  friend std::string to_string(const Rect& self) {
    return std::to_string(self.width_) + "x" + std::to_string(self.height_);
  }

  std::size_t width_ = 6u;
  std::size_t height_ = 7u;
};

struct Shape
    : facets::pack<
          facets::super<Sized>, facets::convention<MemClear, ClearOverload>,
          facets::convention<utils::spec::FreeToString, StringOverload>,
          facets::direct_convention<MemUseCount, UseCountOverload>,
          facets::reflection<utils::RttiReflector>,
          facets::direct_reflection<utils::RttiReflector>,
          facets::layout<2 * sizeof(void*)>, facets::copyability<CL::nothrow>,
          facets::relocatability<CL::nontrivial>,
          facets::destructibility<CL::nontrivial>> {};

struct Copyable : facets::pack<facets::convention<MemSize, SizeOverload>,
                               facets::direct_reflection<utils::RttiReflector>,
                               facets::copyability<CL::nontrivial>> {};

template <std::size_t Padding>
struct PaddedPtr {
  using element_type = Rect;
  Rect& operator*() const noexcept { return *target_; }

  Rect* target_;
  char padding_[Padding];
};

struct Slim : facets::pack<facets::convention<MemSize, SizeOverload>,
                           facets::layout<sizeof(void*)>> {};
struct Roomy : facets::pack<facets::convention<MemSize, SizeOverload>,
                            facets::layout<4 * sizeof(void*)>> {};

// A convention the target does not implement makes it non-proxiable
static_assert(pro::proxiable<Rect*, Sized>);
static_assert(!pro::proxiable<int*, Sized>);
// and so does a layout the pointer does not fit into
static_assert(pro::proxiable<PaddedPtr<3 * sizeof(void*)>, Roomy>);
static_assert(!pro::proxiable<PaddedPtr<3 * sizeof(void*)>, Slim>);
// as does a copyability the pointer cannot honor
static_assert(pro::proxiable<Rect*, Copyable>);
static_assert(!pro::proxiable<std::unique_ptr<Rect>, Copyable>);

} // namespace proxy_facets_tests_detail

namespace detail = proxy_facets_tests_detail;

TEST(ProxyFacetsTests, TestIndirectConvention) {
  detail::Rect rect;
  pro::proxy<detail::Sized> p = &rect;
  ASSERT_EQ(p->Size(), 42u);
}

TEST(ProxyFacetsTests, TestSuperConvention) {
  pro::proxy<detail::Shape> p = std::make_shared<detail::Rect>();
  ASSERT_EQ(p->Size(), 42u);
  ASSERT_EQ(ToString(*p), "6x7");
  p->Clear();
  ASSERT_EQ(p->Size(), 0u);
}

TEST(ProxyFacetsTests, TestDirectConvention) {
  auto rect = std::make_shared<detail::Rect>();
  pro::proxy<detail::Shape> p = rect;
  ASSERT_EQ(p.use_count(), 2);
}

TEST(ProxyFacetsTests, TestReflection) {
  pro::proxy<detail::Shape> p = std::make_shared<detail::Rect>();
  ASSERT_STREQ(p->GetTypeName(), typeid(detail::Rect).name());
  ASSERT_STREQ(p.GetTypeName(), typeid(std::shared_ptr<detail::Rect>).name());
}

TEST(ProxyFacetsTests, TestCopyability) {
  detail::Rect rect;
  pro::proxy<detail::Copyable> p1 = &rect;
  auto p2 = p1;
  ASSERT_EQ(p2->Size(), 42u);
  ASSERT_STREQ(p2.GetTypeName(), typeid(detail::Rect*).name());
}

TEST(ProxyFacetsTests, TestView) {
  detail::Rect rect;
  pro::proxy_view<detail::Sized> v = &rect;
  ASSERT_EQ(v->Size(), 42u);
}
