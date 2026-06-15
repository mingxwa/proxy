// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

// Tests for pointer-authentication (PAC) hardening of `proxy`'s dispatch
// metadata.
//
// `proxy` implements a hand-rolled v-table: `invoker::f_` holds the dispatched
// function pointers and `meta_storage::ptr_` is the v-table pointer. On targets
// whose C++ ABI signs code pointers (Apple arm64e), real virtual functions and
// v-table pointers are protected with ARMv8.3 pointer authentication. When
// `PRO4D_PAC` is on, `proxy` signs its metadata the same way, matching arm64e
// virtual functions with both *address diversity* (the signature depends on the
// storage address) and *type diversity* (a discriminator derived from the
// convention/meta type). The signing intrinsics live directly inside `invoker`
// and `meta_storage`; there is no separate signed-pointer wrapper.
//
// The observable hallmark of address diversity is that copying a non-empty
// proxy re-signs its metadata for the new address, so a copy holds different
// raw bytes than the original while still dispatching correctly. Type diversity
// means signing the same pointer for two different convention types -- even at
// the same address -- yields different signatures. On platforms without PAC the
// copy is bitwise identical. If a target signs code pointers by default but the
// library failed to enable PAC, these tests fail loudly.

#include <cstring>
#include <new>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>
#include <proxy/proxy.h>

namespace proxy_pac_tests_detail {

// Determine, independently of the library's own `PRO4D_PAC` detection, whether
// this target signs code pointers in its ABI (i.e. whether real virtual calls
// are PAC-protected here).
#if defined(__has_feature)
#if __has_feature(ptrauth_calls)
#define PRO_TEST_TARGET_HAS_PAC 1
#endif
#endif
#ifndef PRO_TEST_TARGET_HAS_PAC
#define PRO_TEST_TARGET_HAS_PAC 0
#endif

struct Widget {
  int a = 0;
  int b = 0;
  int GetA() const { return a; }
  int GetB() const { return b; }
  int Sum() const { return a + b; }
  int Diff() const { return a - b; }
};

PRO_DEF_MEM_DISPATCH(MemGetA, GetA);
PRO_DEF_MEM_DISPATCH(MemGetB, GetB);
PRO_DEF_MEM_DISPATCH(MemSum, Sum);
PRO_DEF_MEM_DISPATCH(MemDiff, Diff);

// A single convention with trivial lifetime management: the metadata is a lone
// `invoker` that fits in the small buffer and is therefore embedded *inline* in
// the proxy. This exercises `__ptrauth` on `invoker::f_`.
struct InlineMetaFacade
    : pro::facade_builder                                   //
      ::add_convention<MemGetA, int() const>                //
      ::support_copy<pro::constraint_level::trivial>        //
      ::support_relocation<pro::constraint_level::trivial>  //
      ::support_destruction<pro::constraint_level::trivial> //
      ::restrict_layout<sizeof(void*)>                      //
      ::add_skill<pro::skills::as_view>                     //
      ::build {};

// Many conventions: the metadata no longer fits inline, so it lives in a static
// v-table referenced through a signed pointer. This exercises `__ptrauth` on
// `meta_storage::ptr_`.
struct PointerMetaFacade : pro::facade_builder                    //
                           ::add_convention<MemGetA, int() const> //
                           ::add_convention<MemGetB, int() const> //
                           ::add_convention<MemSum, int() const>  //
                           ::add_convention<MemDiff, int() const> //
                           ::add_skill<pro::skills::as_view>      //
                           ::build {};

// Compile-time contract: with PAC the metadata is address-diversified, so a
// proxy over trivially-copyable storage is nothrow- but not *trivially*-
// copyable. Without PAC it stays trivially copyable. If the target signs code
// pointers yet the library did not enable PAC, fail the build.
//
// The positive (non-PAC) case is asserted via the granular trivial-copy/move
// traits rather than the aggregate `is_trivially_copyable`: nvc++ (EDG)
// mis-reports the aggregate as false for the inline meta, whose `meta_storage`
// has a `using Base::Base;` inherited-constructor base, even though every
// component is trivial and the proxy is bitwise-copyable. The component traits
// (used throughout the rest of the suite) are computed correctly everywhere.
#if PRO4D_PAC
static_assert(
    !std::is_trivially_copy_constructible_v<pro::proxy<InlineMetaFacade>>);
#elif PRO_TEST_TARGET_HAS_PAC
static_assert(false,
              "Target signs code pointers (PAC) but proxy did not enable "
              "pointer-authentication hardening for its dispatch metadata.");
#else
static_assert(
    std::is_trivially_copy_constructible_v<pro::proxy<InlineMetaFacade>>);
static_assert(
    std::is_trivially_move_constructible_v<pro::proxy<InlineMetaFacade>>);
#endif // PRO4D_PAC
static_assert(
    std::is_nothrow_copy_constructible_v<pro::proxy<InlineMetaFacade>>);
static_assert(std::is_trivially_destructible_v<pro::proxy<InlineMetaFacade>>);

template <class T>
bool BytewiseEqual(const T& lhs, const T& rhs) noexcept {
  return std::memcmp(std::addressof(lhs), std::addressof(rhs), sizeof(T)) == 0;
}

// Asserts the relationship between a non-empty proxy and a copy of it placed at
// a different address, validating address diversity on the relevant platform.
template <class T>
void ExpectCopyResigns(const T& original, const T& copy) {
  ASSERT_NE(std::addressof(original), std::addressof(copy));
#if PRO4D_PAC
  // Address diversity re-signs the metadata for the copy's address.
  EXPECT_FALSE(BytewiseEqual(original, copy));
#elif PRO_TEST_TARGET_HAS_PAC
  FAIL() << "Target signs code pointers (PAC) but proxy did not enable "
            "pointer-authentication hardening for its dispatch metadata.";
#else
  // No PAC: the copy is a verbatim bitwise duplicate.
  EXPECT_TRUE(BytewiseEqual(original, copy));
#endif // PRO4D_PAC
}

} // namespace proxy_pac_tests_detail

namespace {

using namespace proxy_pac_tests_detail;

// Inline metadata (`invoker::f_`) via an owning proxy whose v-table is
// embedded.
TEST(ProxyPacTests, InlineMetaProxyCopyResigns) {
  Widget w{3, 4};
  pro::proxy<InlineMetaFacade> p = &w;
  pro::proxy<InlineMetaFacade> q = p;
  // The re-signed copy still authenticates and dispatches correctly.
  EXPECT_EQ(p->GetA(), 3);
  EXPECT_EQ(q->GetA(), 3);
  ExpectCopyResigns(p, q);
}

// Inline metadata via a (trivially copyable) proxy view.
TEST(ProxyPacTests, InlineMetaViewCopyResigns) {
  Widget w{5, 6};
  pro::proxy_view<InlineMetaFacade> v =
      pro::make_proxy_view<InlineMetaFacade>(w);
  pro::proxy_view<InlineMetaFacade> v2 = v;
  EXPECT_EQ(v->GetA(), 5);
  EXPECT_EQ(v2->GetA(), 5);
  ExpectCopyResigns(v, v2);
}

// Pointer metadata (`meta_storage::ptr_`): the v-table is out of line and the
// proxy stores a signed pointer to it.
TEST(ProxyPacTests, PointerMetaViewCopyResigns) {
  Widget w{10, 4};
  pro::proxy_view<PointerMetaFacade> v =
      pro::make_proxy_view<PointerMetaFacade>(w);
  pro::proxy_view<PointerMetaFacade> v2 = v;
  EXPECT_EQ(v->GetA(), 10);
  EXPECT_EQ(v2->Sum(), 14);
  EXPECT_EQ(v2->Diff(), 6);
  ExpectCopyResigns(v, v2);
}

// Assignment must also re-sign for the destination address, and the result must
// remain dispatchable.
TEST(ProxyPacTests, CopyAssignmentResigns) {
  Widget w1{1, 2};
  Widget w2{7, 8};
  pro::proxy_view<PointerMetaFacade> v1 =
      pro::make_proxy_view<PointerMetaFacade>(w1);
  pro::proxy_view<PointerMetaFacade> v2 =
      pro::make_proxy_view<PointerMetaFacade>(w2);
  v2 = v1;
  EXPECT_EQ(v2->GetA(), 1);
  EXPECT_EQ(v2->GetB(), 2);
  ExpectCopyResigns(v1, v2);
}

// A moved-to proxy is re-signed for its own address and stays usable.
TEST(ProxyPacTests, MoveResignsAndDispatches) {
  Widget w{42, 0};
  pro::proxy<InlineMetaFacade> p = &w;
  pro::proxy<InlineMetaFacade> moved = std::move(p);
  EXPECT_TRUE(moved.has_value());
  EXPECT_EQ(moved->GetA(), 42);
}

// Copying or swapping an EMPTY proxy/view must not trap. For a trivially-
// copyable facade the proxy's copy is the defaulted, memberwise one -- it
// copies the metadata with no has_value() guard -- so an empty proxy copies its
// null metadata straight through the signed pointers, whose copy is total.
// Covers both inline metadata (the leading `invoker`) and out-of-line metadata
// (the `meta_storage` pointer), and the empty<->non-empty swap that moves a
// null around.
TEST(ProxyPacTests, EmptyProxyAndViewCopyAndSwap) {
  // Inline metadata, trivially-copyable owning proxy.
  pro::proxy<InlineMetaFacade> p;
  ASSERT_FALSE(p.has_value());
  pro::proxy<InlineMetaFacade> p2 = p; // copy-construct from empty
  EXPECT_FALSE(p2.has_value());
  p2 = p; // copy-assign from empty
  EXPECT_FALSE(p2.has_value());

  // Out-of-line metadata, trivially-copyable view.
  pro::proxy_view<PointerMetaFacade> v;
  ASSERT_FALSE(v.has_value());
  pro::proxy_view<PointerMetaFacade> v2 = v; // copy-construct from empty
  EXPECT_FALSE(v2.has_value());
  v2 = v; // copy-assign from empty
  EXPECT_FALSE(v2.has_value());

  // Swapping empty<->empty and empty<->non-empty must not trap, and the live
  // value must survive (re-signed for its new slot).
  Widget w{1, 2};
  pro::proxy<InlineMetaFacade> nonempty = &w;
  p.swap(p2); // empty <-> empty
  EXPECT_FALSE(p.has_value());
  EXPECT_FALSE(p2.has_value());
  p.swap(nonempty); // empty <-> non-empty
  EXPECT_TRUE(p.has_value());
  EXPECT_FALSE(nonempty.has_value());
  EXPECT_EQ(p->GetA(), 1);
}

#if PRO4D_PAC
// Type diversity is rooted in the discriminator `invoker` derives for its
// signed function pointer: `ptrauth_type_discriminator` of a function type that
// carries the dispatch tag `D` *by value*. That placement is load-bearing: the
// builtin canonicalizes every pointer parameter to a single token, so encoding
// `D` as `D*` would give every dispatch the same discriminator. Guard the
// property the metadata relies on -- two conventions differing only in `D` stay
// distinct, and the discriminator is non-zero. (Runs only where PAC is active.)
TEST(ProxyPacTests, TypeDiscriminatorDiversifiesByDispatchTag) {
  EXPECT_NE(ptrauth_type_discriminator(void (*)(int&, MemGetA)),
            ptrauth_type_discriminator(void (*)(int&, MemGetB)));
  EXPECT_NE(ptrauth_type_discriminator(void (*)(int&, MemGetA)), 0u);
}

// The out-of-line v-table pointer (`meta_storage::ptr_`) must be copyable while
// null: an empty out-of-line `meta_storage` *is* exactly this one signed
// pointer, and its own null is the empty state, so copying an empty meta must
// not authenticate-and-resign a null (which would trap). Unlike the inline
// convention pointers, there is no separate has_value() bit gating this copy. A
// non-null meta is still resigned for its new address and stays usable.
TEST(ProxyPacTests, OutOfLineMetaStorageCopiesNullAndResigns) {
  using MS = pro::detail::facade_traits<PointerMetaFacade>::meta;
  // The out-of-line meta is address-diversified (signed), hence not trivially
  // copyable; this guards that the path under test is the out-of-line one.
  static_assert(!std::is_trivially_copyable_v<MS>);

  // Null source: default-construct (signed null), copy-construct, and copy-
  // assign must not trap, and the result stays empty.
  MS null_src;
  EXPECT_FALSE(null_src.has_value());
  MS null_copy = null_src;
  EXPECT_FALSE(null_copy.has_value());
  MS null_assigned;
  null_assigned = null_src;
  EXPECT_FALSE(null_assigned.has_value());

  // Non-null source: the copy authenticates and resigns for its own address and
  // remains non-empty.
  MS a{std::in_place_type<Widget*>};
  EXPECT_TRUE(a.has_value());
  MS b = a;
  EXPECT_TRUE(b.has_value());
  MS c;
  c = a;
  EXPECT_TRUE(c.has_value());
}
#endif // PRO4D_PAC

} // namespace
