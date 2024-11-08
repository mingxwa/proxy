// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "proxy_invocation_benchmark_context.h"

namespace {

constexpr int TestDataSize = 1000000;

template <int V>
struct IntConstant {};

template <int FromTypeSeries, class T, class F>
void FillTestData(std::vector<T>& data, const F& generator) {
  if constexpr (FromTypeSeries < details::TypeSeriesCount) {
    for (int i = FromTypeSeries; i < TestDataSize; i += details::TypeSeriesCount) {
      data[i] = generator(IntConstant<FromTypeSeries>{}, i);
    }
    FillTestData<FromTypeSeries + 1>(data, generator);
  }
}

template <class F>
auto GenerateTestData(const F& generator) {
  std::vector<decltype(generator(IntConstant<0>{}, 0))> result(TestDataSize);
  FillTestData<0>(result, generator);
  return result;
}

}  // namespace

std::vector<pro::proxy<InvocationTestFacade>> GenerateSmallObjectInvocationProxyTestData() {
  return GenerateTestData([]<int TypeSeries>(IntConstant<TypeSeries>, int seed)
      { return pro::make_proxy<InvocationTestFacade, details::NonIntrusiveSmallImpl<TypeSeries>>(seed); });
}
std::vector<std::unique_ptr<InvocationTestBase>> GenerateSmallObjectInvocationVirtualFunctionTestData() {
  return GenerateTestData([]<int TypeSeries>(IntConstant<TypeSeries>, int seed)
      { return std::unique_ptr<InvocationTestBase>{new details::IntrusiveSmallImpl<TypeSeries>(seed)}; });
}
std::vector<details::VariantType> GenerateSmallObjectInvocationVariantTestData() {
  return GenerateTestData([]<int TypeSeries>(IntConstant<TypeSeries>, int seed) -> details::VariantType
      { return details::NonIntrusiveSmallImpl<TypeSeries>{seed}; });
}
std::vector<pro::proxy<InvocationTestFacade>> GenerateLargeObjectInvocationProxyTestData() {
  return GenerateTestData([]<int TypeSeries>(IntConstant<TypeSeries>, int seed)
      { return pro::make_proxy<InvocationTestFacade, details::NonIntrusiveLargeImpl<TypeSeries>>(seed); });
}
std::vector<std::unique_ptr<InvocationTestBase>> GenerateLargeObjectInvocationVirtualFunctionTestData() {
  return GenerateTestData([]<int TypeSeries>(IntConstant<TypeSeries>, int seed)
      { return std::unique_ptr<InvocationTestBase>{new details::IntrusiveLargeImpl<TypeSeries>(seed)}; });
}
std::vector<details::VariantType> GenerateLargeObjectInvocationVariantTestData() {
  return GenerateTestData([]<int TypeSeries>(IntConstant<TypeSeries>, int seed) -> details::VariantType
      { return details::NonIntrusiveLargeImpl<TypeSeries>{seed}; });
}
