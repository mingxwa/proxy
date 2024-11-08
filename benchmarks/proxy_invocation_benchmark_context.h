// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <memory>
#include <variant>
#include <vector>

#include "proxy.h"

PRO_DEF_MEM_DISPATCH(MemFun, Fun);

struct InvocationTestFacade : pro::facade_builder
    ::add_convention<MemFun, int() const>
    ::build{};

struct InvocationTestBase {
  virtual int Fun() const = 0;
  virtual ~InvocationTestBase() = default;
};

namespace details {

constexpr int TypeSeriesCount = 100;

template <int TypeSeries>
class NonIntrusiveSmallImpl {
 public:
  NonIntrusiveSmallImpl() noexcept = default;
  explicit NonIntrusiveSmallImpl(int seed) noexcept : seed_(seed) {}
  NonIntrusiveSmallImpl(const NonIntrusiveSmallImpl&) noexcept = default;
  int Fun() const noexcept { return seed_ ^ (TypeSeries + 1); }

 private:
  int seed_;
};

template <int TypeSeries>
class NonIntrusiveLargeImpl {
 public:
  NonIntrusiveLargeImpl() noexcept = default;
  explicit NonIntrusiveLargeImpl(int seed) noexcept : seed_(seed) {}
  NonIntrusiveLargeImpl(const NonIntrusiveLargeImpl&) noexcept = default;
  int Fun() const noexcept { return seed_ ^ (TypeSeries + 1); }

 private:
  void* padding_[5]{};
  int seed_;
};

template <int TypeSeries>
class IntrusiveSmallImpl : public InvocationTestBase {
 public:
  explicit IntrusiveSmallImpl(int seed) noexcept : seed_(seed) {}
  IntrusiveSmallImpl(const IntrusiveSmallImpl&) noexcept = default;
  int Fun() const noexcept override { return seed_ ^ (TypeSeries + 1); }

 private:
  int seed_;
};

template <int TypeSeries>
class IntrusiveLargeImpl : public InvocationTestBase {
 public:
  explicit IntrusiveLargeImpl(int seed) noexcept : seed_(seed) {}
  IntrusiveLargeImpl(const IntrusiveLargeImpl&) noexcept = default;
  int Fun() const noexcept override { return seed_ ^ (TypeSeries + 1); }

 private:
  void* padding_[5]{};
  int seed_;
};

template <template <int> class T, class Is> struct VariantTraits;
template <template <int> class T, int... Is>
struct VariantTraits<T, std::integer_sequence<int, Is...>> : std::type_identity<std::variant<T<Is>...>> {};
template <template <int> class T>
using VariantType = typename VariantTraits<T, std::make_integer_sequence<int, TypeSeriesCount>>::type;

}  // namespace details

std::vector<pro::proxy<InvocationTestFacade>> GenerateSmallObjectInvocationProxyTestData();
std::vector<std::unique_ptr<InvocationTestBase>> GenerateSmallObjectInvocationVirtualFunctionTestData();
std::vector<details::VariantType<details::NonIntrusiveSmallImpl>> GenerateSmallObjectInvocationVariantTestData();
std::vector<pro::proxy<InvocationTestFacade>> GenerateLargeObjectInvocationProxyTestData();
std::vector<std::unique_ptr<InvocationTestBase>> GenerateLargeObjectInvocationVirtualFunctionTestData();
std::vector<details::VariantType<details::NonIntrusiveLargeImpl>> GenerateLargeObjectInvocationVariantTestData();
