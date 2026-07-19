// Copyright (c) 2022-2026 Microsoft Corporation.
// Copyright (c) 2026-Present Next Gen C++ Foundation.
// Licensed under the MIT License.

#include <proxy/proxy.h>

#if __cpp_impl_reflection >= 202506L

#include <gtest/gtest.h>
#include <string>
#include <utility>

namespace proxy_mem_dispatch_tests_detail {

struct Stringable : pro::facade_builder //
                    ::add_convention<pro::mem_dispatch<"ToString">,
                                     std::string() const> //
                    ::build {};

struct Widget {
  std::string ToString() const { return "Widget"; }
};

TEST(ProxyMemDispatchTests, TestBasicInvocation) {
  Widget w;
  pro::proxy<Stringable> p = &w;
  ASSERT_EQ(p->ToString(), "Widget");
}

struct NotStringable {};
static_assert(!pro::proxiable<NotStringable*, Stringable>);

struct Adder : pro::facade_builder //
               ::add_convention<pro::mem_dispatch<"Add">, int(int),
                                double(double)> //
               ::build {};

struct Calculator {
  int Add(int v) { return v + 1; }
  double Add(double v) { return v + .5; }
};

TEST(ProxyMemDispatchTests, TestOverloadResolution) {
  Calculator c;
  pro::proxy<Adder> p = &c;
  ASSERT_EQ(p->Add(2), 3);
  ASSERT_EQ(p->Add(2.), 2.5);
}

struct DefaultArgCallable
    : pro::facade_builder                                                 //
      ::add_convention<pro::mem_dispatch<"Mul">, int(int), int(int, int)> //
      ::build {};

struct Multiplier {
  int Mul(int a, int b = 3) { return a * b; }
};

TEST(ProxyMemDispatchTests, TestDefaultArgument) {
  Multiplier m;
  pro::proxy<DefaultArgCallable> p = &m;
  ASSERT_EQ(p->Mul(5), 15);
  ASSERT_EQ(p->Mul(5, 2), 10);
}

struct NamedFacade : pro::facade_builder //
                     ::add_convention<pro::mem_dispatch<"GetName">,
                                      std::string() const> //
                     ::build {};

struct StaticallyNamed {
  static std::string GetName() { return "Static"; }
};

TEST(ProxyMemDispatchTests, TestStaticMember) {
  StaticallyNamed s;
  pro::proxy<NamedFacade> p = &s;
  ASSERT_EQ(p->GetName(), "Static");
}

struct DynamicallyNamedBase {
  std::string GetName() const { return "Inherited"; }
};
struct DynamicallyNamedDerived : DynamicallyNamedBase {};

TEST(ProxyMemDispatchTests, TestInheritedMember) {
  DynamicallyNamedDerived d;
  pro::proxy<NamedFacade> p = &d;
  ASSERT_EQ(p->GetName(), "Inherited");
}

struct Renamed : pro::facade_builder //
                 ::add_convention<pro::mem_dispatch<"ToString", "Str">,
                                  std::string() const> //
                 ::build {};

TEST(ProxyMemDispatchTests, TestRenamedAccessor) {
  Widget w;
  pro::proxy<Renamed> p = &w;
  ASSERT_EQ(p->Str(), "Widget");
}

struct NoexceptCallable : pro::facade_builder //
                          ::add_convention<pro::mem_dispatch<"Value">,
                                           int() const noexcept> //
                          ::build {};

struct NoexceptValue {
  int Value() const noexcept { return 42; }
};

struct ThrowingValue {
  int Value() const { return 0; }
};

TEST(ProxyMemDispatchTests, TestNoexcept) {
  NoexceptValue v;
  pro::proxy<NoexceptCallable> p = &v;
  static_assert(noexcept(p->Value()));
  static_assert(!pro::proxiable<ThrowingValue*, NoexceptCallable>);
  ASSERT_EQ(p->Value(), 42);
}

struct QualifiedCallable
    : pro::facade_builder //
      ::add_convention<pro::mem_dispatch<"Describe">, std::string() &,
                       std::string() &&> //
      ::build {};

struct Qualified {
  std::string Describe() & { return "lvalue"; }
  std::string Describe() && { return "rvalue"; }
};

TEST(ProxyMemDispatchTests, TestQualifiedConvention) {
  pro::proxy<QualifiedCallable> p =
      pro::make_proxy<QualifiedCallable, Qualified>();
  ASSERT_EQ(p->Describe(), "lvalue");
  ASSERT_EQ(std::move(*p).Describe(), "rvalue");
}

struct ConstOverloaded {
  std::string Get() { return "mutable"; }
  std::string Get() const { return "const"; }
};

struct ConstOverloadedCallable
    : pro::facade_builder //
      ::add_convention<pro::mem_dispatch<"Get">, std::string(),
                       std::string() const> //
      ::build {};

TEST(ProxyMemDispatchTests, TestConstOverloadResolution) {
  pro::proxy<ConstOverloadedCallable> p =
      pro::make_proxy<ConstOverloadedCallable, ConstOverloaded>();
  ASSERT_EQ(p->Get(), "mutable");
  ASSERT_EQ(std::as_const(*p).Get(), "const");
}

struct WeakStringable
    : pro::facade_builder //
      ::add_convention<pro::weak_dispatch<pro::mem_dispatch<"ToString">>,
                       std::string() const> //
      ::build {};

TEST(ProxyMemDispatchTests, TestWeakDispatch) {
  NotStringable n;
  pro::proxy<WeakStringable> p = &n;
  bool exception_thrown = false;
  try {
    p->ToString();
  } catch (const pro::not_implemented&) {
    exception_thrown = true;
  }
  ASSERT_TRUE(exception_thrown);
}

TEST(ProxyMemDispatchTests, TestDirectInvocation) {
  Widget w;
  ASSERT_EQ(pro::mem_dispatch<"ToString">{}(w), "Widget");
  static_assert(
      !std::is_invocable_v<pro::mem_dispatch<"ToString">, NotStringable&>);
}

struct Poisoned {
  int Run(int) = delete;
  int Run(double) { return 1; }
};

TEST(ProxyMemDispatchTests, TestDeletedOverload) {
  // A deleted member participates in overload resolution: selecting it
  // invalidates the call, exactly as `self.Run(...)` would.
  static_assert(!std::is_invocable_v<pro::mem_dispatch<"Run">, Poisoned&, int>);
  Poisoned p;
  ASSERT_EQ(pro::mem_dispatch<"Run">{}(p, 2.), 1);
}

struct HidingBase {
  int Val() const { return 1; }
};
struct HidingDerived : HidingBase {
  int Val(int) const { return 2; }
};

TEST(ProxyMemDispatchTests, TestNameHiding) {
  // The derived declaration hides all base overloads, as if there were no
  // using-declaration.
  static_assert(
      !std::is_invocable_v<pro::mem_dispatch<"Val">, const HidingDerived&>);
  HidingDerived d;
  ASSERT_EQ(pro::mem_dispatch<"Val">{}(d, 0), 2);
}

struct AbstractBase {
  virtual std::string Name() const = 0;
  virtual ~AbstractBase() = default;
};
struct Override : AbstractBase {
  std::string Name() const override { return "Override"; }
};

TEST(ProxyMemDispatchTests, TestVirtualOverride) {
  // The override hides the pure declaration it overrides.
  Override o;
  static_assert(std::is_invocable_r_v<std::string, pro::mem_dispatch<"Name">,
                                      const Override&>);
  ASSERT_EQ(pro::mem_dispatch<"Name">{}(o), "Override");
  AbstractBase& b = o;
  ASSERT_EQ(pro::mem_dispatch<"Name">{}(b), "Override"); // virtual dispatch
}

struct ExplicitObject {
  int Weigh(this const ExplicitObject&, int v) { return v * 3; }
};

TEST(ProxyMemDispatchTests, TestExplicitObjectMember) {
  ExplicitObject e;
  ASSERT_EQ(pro::mem_dispatch<"Weigh">{}(e, 5), 15);
  static_assert(
      !std::is_invocable_v<pro::mem_dispatch<"Weigh">, ExplicitObject&, void*>);
}

struct SiblingLhs {
  int Tag(int) const { return 1; }
};
struct SiblingRhs {
  int Tag(int) const { return 2; }
  int Tag(double) const { return 3; }
};
struct Siblings : SiblingLhs, SiblingRhs {};

TEST(ProxyMemDispatchTests, TestSiblingBases) {
  // Sibling bases merge as if disambiguated with using-declarations. A
  // signature declared by both is ambiguous: the call neither resolves nor
  // falls back to another overload.
  static_assert(
      !std::is_invocable_v<pro::mem_dispatch<"Tag">, const Siblings&, int>);
  Siblings s;
  ASSERT_EQ(pro::mem_dispatch<"Tag">{}(s, 0.), 3);
}

struct MixedFnBase {
  int Mix() const { return 0; }
};
struct MixedFieldBase {
  struct {
    int operator()() const { return 0; }
  } Mix;
};
struct MixedMembers : MixedFnBase, MixedFieldBase {};
static_assert(!std::is_invocable_v<pro::mem_dispatch<"Mix">, MixedMembers&>);

struct DupFieldLhs {
  struct {
    int operator()() const { return 1; }
  } Dup;
};
struct DupFieldRhs {
  struct {
    int operator()() const { return 2; }
  } Dup;
};
struct DupFields : DupFieldLhs, DupFieldRhs {};
static_assert(!std::is_invocable_v<pro::mem_dispatch<"Dup">, DupFields&>);

struct CallableMember {
  struct {
    int operator()(int v) const { return v * 2; }
  } Fn;
};

struct FieldCallable : pro::facade_builder //
                       ::add_convention<pro::mem_dispatch<"Fn">,
                                        int(int) const> //
                       ::build {};

TEST(ProxyMemDispatchTests, TestCallableDataMember) {
  CallableMember c;
  pro::proxy<FieldCallable> p = &c;
  ASSERT_EQ(p->Fn(21), 42);
}

struct Pingable : pro::facade_builder //
                  ::add_convention<pro::mem_dispatch<"Ping">,
                                   int() const> //
                  ::build {};

struct Pinger {
  int Ping() const { return 7; }
};

TEST(ProxyMemDispatchTests, TestSelfComposition) {
  // The accessor generated by mem_dispatch is itself a callable data
  // member; the dispatch must find it like a member function.
  Pinger obj;
  pro::proxy<Pingable> p = &obj;
  static_assert(
      std::is_invocable_r_v<int, pro::mem_dispatch<"Ping">, decltype(*p)>);
  ASSERT_EQ(pro::mem_dispatch<"Ping">{}(*p), 7);
}

} // namespace proxy_mem_dispatch_tests_detail

#endif // __cpp_impl_reflection >= 202506L
