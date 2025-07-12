// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <benchmark/benchmark.h>

#include "proxy_invocation_benchmark_context.h"

#if defined(_MSC_VER)
#define FORCE_NOINLINE __declspec(noinline)
#elif defined(__clang__) || defined(__GNUC__) || defined(__NVCOMPILER)
#define FORCE_NOINLINE __attribute__((noinline))
#else
#error Failed to detect compiler support for FORCE_NOINLINE.
#endif

namespace {

template <class T>
struct InvocationTestCrtpBase {
  FORCE_NOINLINE int Fun() const {
    return static_cast<const T&>(*this).FunImpl();
  }
};
template <int TypeSeries>
class CrtpSmallImplCrtp
    : public InvocationTestCrtpBase<CrtpSmallImplCrtp<TypeSeries>> {
public:
  CrtpSmallImplCrtp() = default;
  explicit CrtpSmallImplCrtp(int seed) noexcept : seed_(seed) {}
  CrtpSmallImplCrtp(const CrtpSmallImplCrtp&) noexcept = default;
  int FunImpl() const noexcept { return seed_ ^ (TypeSeries + 1); }

private:
  int seed_;
};
template <int TypeSeries>
class CrtpLargeImplCrtp
    : public InvocationTestCrtpBase<CrtpLargeImplCrtp<TypeSeries>> {
public:
  CrtpLargeImplCrtp() = default;
  explicit CrtpLargeImplCrtp(int seed) noexcept : seed_(seed) {}
  CrtpLargeImplCrtp(const CrtpLargeImplCrtp&) noexcept = default;
  int FunImpl() const noexcept { return seed_ ^ (TypeSeries + 1); }

private:
  void* padding_[5]{};
  int seed_;
};
template <template <int> class T, class Is>
struct CrtpTupleTraits;
template <template <int> class T, int... Is>
struct CrtpTupleTraits<T, std::integer_sequence<int, Is...>> {
  using type = std::tuple<T<Is>...>;
};
using CrtpSmallImplTuple = typename CrtpTupleTraits<
    CrtpSmallImplCrtp, std::make_integer_sequence<int, TypeSeriesCount>>::type;
using CrtpLargeImplTuple = typename CrtpTupleTraits<
    CrtpLargeImplCrtp, std::make_integer_sequence<int, TypeSeriesCount>>::type;

std::vector<CrtpSmallImplTuple> GenerateSmallObjectInvocationCrtpTestData() {
  std::vector<CrtpSmallImplTuple> result(TestDataSize / TypeSeriesCount);
  for (int i = 0; i < static_cast<int>(result.size()); ++i) {
    std::apply(
        [&]<int... Is>(CrtpSmallImplCrtp<Is>&... vs) {
          ((vs = CrtpSmallImplCrtp<Is>(i * TypeSeriesCount + Is)), ...);
        },
        result[i]);
  }
  return result;
}

std::vector<CrtpLargeImplTuple> GenerateLargeObjectInvocationCrtpTestData() {
  std::vector<CrtpLargeImplTuple> result(TestDataSize / TypeSeriesCount);
  for (int i = 0; i < static_cast<int>(result.size()); ++i) {
    std::apply(
        [&]<int... Is>(CrtpLargeImplCrtp<Is>&... vs) {
          ((vs = CrtpLargeImplCrtp<Is>(i * TypeSeriesCount + Is)), ...);
        },
        result[i]);
  }
  return result;
}

void BM_SmallObjectInvocationViaProxy(benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationProxyTestData();
  std::vector<pro::proxy_view<InvocationTestFacade>> views(data.begin(),
                                                           data.end());
  for (auto _ : state) {
    for (auto& p : views) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_SmallObjectInvocationViaProxy_Shared(benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationProxyTestData_Shared();
  std::vector<pro::proxy_view<InvocationTestFacade>> views(data.begin(),
                                                           data.end());
  for (auto _ : state) {
    for (auto& p : views) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_SmallObjectInvocationViaProxyView(benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationProxyTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_SmallObjectInvocationViaVirtualFunction(benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationVirtualFunctionTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_SmallObjectInvocationViaVirtualFunction_Shared(
    benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationVirtualFunctionTestData_Shared();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_SmallObjectInvocationViaCrtp(benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationCrtpTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      std::apply([](auto&... vs) { (benchmark::DoNotOptimize(vs.Fun()), ...); },
                 p);
    }
  }
}

void BM_LargeObjectInvocationViaProxy(benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationProxyTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_LargeObjectInvocationViaProxy_Shared(benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationProxyTestData_Shared();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_LargeObjectInvocationViaProxyView(benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationProxyTestData();
  std::vector<pro::proxy_view<InvocationTestFacade>> views(data.begin(),
                                                           data.end());
  for (auto _ : state) {
    for (auto& p : views) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_LargeObjectInvocationViaVirtualFunction(benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationVirtualFunctionTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_LargeObjectInvocationViaVirtualFunction_Shared(
    benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationVirtualFunctionTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_LargeObjectInvocationViaCrtp(benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationCrtpTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      std::apply([](auto&... vs) { (benchmark::DoNotOptimize(vs.Fun()), ...); },
                 p);
    }
  }
}

BENCHMARK(BM_SmallObjectInvocationViaProxy);
BENCHMARK(BM_SmallObjectInvocationViaProxy_Shared);
BENCHMARK(BM_SmallObjectInvocationViaProxyView);
BENCHMARK(BM_SmallObjectInvocationViaVirtualFunction);
BENCHMARK(BM_SmallObjectInvocationViaVirtualFunction_Shared);
BENCHMARK(BM_SmallObjectInvocationViaCrtp);
BENCHMARK(BM_LargeObjectInvocationViaProxy);
BENCHMARK(BM_LargeObjectInvocationViaProxy_Shared);
BENCHMARK(BM_LargeObjectInvocationViaProxyView);
BENCHMARK(BM_LargeObjectInvocationViaVirtualFunction);
BENCHMARK(BM_LargeObjectInvocationViaVirtualFunction_Shared);
BENCHMARK(BM_LargeObjectInvocationViaCrtp);

} // namespace
