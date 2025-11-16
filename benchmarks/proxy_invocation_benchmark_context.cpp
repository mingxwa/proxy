// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "proxy_invocation_benchmark_context.h"

namespace {

constexpr int TestDataSize = 1000000;
constexpr int TypeSeriesCount = 100;

template <int TypeSeries>
class NonIntrusiveSmallImpl {
public:
  explicit NonIntrusiveSmallImpl(int seed) noexcept : seed_(seed) {}
  NonIntrusiveSmallImpl(const NonIntrusiveSmallImpl&) noexcept = default;
  int Fun() const noexcept { return seed_ ^ (TypeSeries + 1); }

private:
  int seed_;
};

template <int TypeSeries>
class NonIntrusiveLargeImpl {
public:
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

template <template <int> class T, class U, class Vs>
struct TestDataUnitObserverImpl;
template <template <int> class T, class U, int... Vs>
struct TestDataUnitObserverImpl<T, U, std::integer_sequence<int, Vs...>> {
  explicit TestDataUnitObserverImpl(int seed_base) noexcept
      : Value((seed_base + Vs)...) {}

  std::vector<U> ObserveTestData() const { return {&std::get<Vs>(Value)...}; }

  std::tuple<T<Vs>...> Value;
};
template <template <int> class T, class U>
struct TestDataUnitObserver
    : TestDataUnitObserverImpl<
          T, U, std::make_integer_sequence<int, TypeSeriesCount>> {
  explicit TestDataUnitObserver(int seed_base) noexcept
      : TestDataUnitObserverImpl<
            T, U, std::make_integer_sequence<int, TypeSeriesCount>>(seed_base) {
  }
};
template <template <int> class T, class U>
class TestDataObserver {
public:
  explicit TestDataObserver() {
    units_.reserve(TestDataSize / TypeSeriesCount);
    for (int i = 0; i < TestDataSize; i += TypeSeriesCount) {
      units_.emplace_back(i);
    }
  }

  std::vector<U> ObserveTestData() const {
    std::vector<U> result;
    result.reserve(TestDataSize);
    for (auto& unit : units_) {
      auto unit_data = unit.ObserveTestData();
      result.insert(result.end(), unit_data.begin(), unit_data.end());
    }
    return result;
  }

private:
  std::vector<TestDataUnitObserver<T, U>> units_;
};

template <int V>
struct IntConstant {};

template <int FromTypeSeries, class T, class F>
void FillTestData(std::vector<T>& data, const F& generator) {
  if constexpr (FromTypeSeries < TypeSeriesCount) {
    for (int i = FromTypeSeries; i < TestDataSize; i += TypeSeriesCount) {
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

} // namespace

std::vector<pro::proxy<InvocationTestFacade>>
    GenerateSmallObjectInvocationProxyTestData() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return pro::make_proxy<InvocationTestFacade,
                               NonIntrusiveSmallImpl<TypeSeries>>(seed);
      });
}
pro::proxy<TestDataObserverFacade<pro::proxy_view<InvocationTestFacade>>>
    GenerateSmallObjectInvocationProxyTestData_Observer() {
  return pro::make_proxy<
      TestDataObserverFacade<pro::proxy_view<InvocationTestFacade>>,
      TestDataObserver<NonIntrusiveSmallImpl,
                       pro::proxy_view<InvocationTestFacade>>>();
}
std::vector<pro::proxy<InvocationTestFacade>>
    GenerateSmallObjectInvocationProxyTestData_Shared() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return pro::make_proxy_shared<InvocationTestFacade,
                                      NonIntrusiveSmallImpl<TypeSeries>>(seed);
      });
}
std::vector<std::unique_ptr<InvocationTestBase>>
    GenerateSmallObjectInvocationVirtualFunctionTestData() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return std::unique_ptr<InvocationTestBase>{
            new IntrusiveSmallImpl<TypeSeries>(seed)};
      });
}
pro::proxy<TestDataObserverFacade<const InvocationTestBase*>>
    GenerateSmallObjectInvocationVirtualFunctionTestData_Observer() {
  return pro::make_proxy<
      TestDataObserverFacade<const InvocationTestBase*>,
      TestDataObserver<IntrusiveSmallImpl, const InvocationTestBase*>>();
}
std::vector<std::shared_ptr<InvocationTestBase>>
    GenerateSmallObjectInvocationVirtualFunctionTestData_Shared() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return std::shared_ptr<InvocationTestBase>{
            std::make_shared<IntrusiveSmallImpl<TypeSeries>>(seed)};
      });
}
std::vector<pro::proxy<InvocationTestFacade>>
    GenerateLargeObjectInvocationProxyTestData() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return pro::make_proxy<InvocationTestFacade,
                               NonIntrusiveLargeImpl<TypeSeries>>(seed);
      });
}
pro::proxy<TestDataObserverFacade<pro::proxy_view<InvocationTestFacade>>>
    GenerateLargeObjectInvocationProxyTestData_Observer() {
  return pro::make_proxy<
      TestDataObserverFacade<pro::proxy_view<InvocationTestFacade>>,
      TestDataObserver<NonIntrusiveLargeImpl,
                       pro::proxy_view<InvocationTestFacade>>>();
}
std::vector<pro::proxy<InvocationTestFacade>>
    GenerateLargeObjectInvocationProxyTestData_Shared() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return pro::make_proxy_shared<InvocationTestFacade,
                                      NonIntrusiveLargeImpl<TypeSeries>>(seed);
      });
}
std::vector<std::unique_ptr<InvocationTestBase>>
    GenerateLargeObjectInvocationVirtualFunctionTestData() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return std::unique_ptr<InvocationTestBase>{
            new IntrusiveLargeImpl<TypeSeries>(seed)};
      });
}
pro::proxy<TestDataObserverFacade<const InvocationTestBase*>>
    GenerateLargeObjectInvocationVirtualFunctionTestData_Observer() {
  return pro::make_proxy<
      TestDataObserverFacade<const InvocationTestBase*>,
      TestDataObserver<IntrusiveLargeImpl, const InvocationTestBase*>>();
}
std::vector<std::shared_ptr<InvocationTestBase>>
    GenerateLargeObjectInvocationVirtualFunctionTestData_Shared() {
  return GenerateTestData(
      []<int TypeSeries>(IntConstant<TypeSeries>, int seed) {
        return std::shared_ptr<InvocationTestBase>{
            std::make_shared<IntrusiveLargeImpl<TypeSeries>>(seed)};
      });
}
