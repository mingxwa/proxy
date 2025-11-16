// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <benchmark/benchmark.h>

#include "proxy_invocation_benchmark_context.h"

namespace {

void BM_SmallObjectInvocationViaProxy(benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationProxyTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_SmallObjectInvocationViaProxy_Observer(benchmark::State& state) {
  auto observer = GenerateSmallObjectInvocationProxyTestData_Observer();
  auto data = observer->ObserveTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_SmallObjectInvocationViaProxy_Shared(benchmark::State& state) {
  auto data = GenerateSmallObjectInvocationProxyTestData_Shared();
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

void BM_SmallObjectInvocationViaVirtualFunction_Observer(benchmark::State& state) {
  auto observer = GenerateSmallObjectInvocationVirtualFunctionTestData_Observer();
  auto data = observer->ObserveTestData();
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

void BM_LargeObjectInvocationViaProxy(benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationProxyTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_LargeObjectInvocationViaProxy_Observer(benchmark::State& state) {
  auto observer = GenerateLargeObjectInvocationProxyTestData_Observer();
  auto data = observer->ObserveTestData();
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

void BM_LargeObjectInvocationViaVirtualFunction(benchmark::State& state) {
  auto data = GenerateLargeObjectInvocationVirtualFunctionTestData();
  for (auto _ : state) {
    for (auto& p : data) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

void BM_LargeObjectInvocationViaVirtualFunction(benchmark::State& state) {
  auto observer = GenerateLargeObjectInvocationVirtualFunctionTestData_Observer();
  auto data = observer->ObserveTestData();
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

BENCHMARK(BM_SmallObjectInvocationViaProxy);
BENCHMARK(BM_SmallObjectInvocationViaProxy_Observer);
BENCHMARK(BM_SmallObjectInvocationViaProxy_Shared);
BENCHMARK(BM_SmallObjectInvocationViaVirtualFunction);
BENCHMARK(BM_SmallObjectInvocationViaVirtualFunction_Observer);
BENCHMARK(BM_SmallObjectInvocationViaVirtualFunction_Shared);
BENCHMARK(BM_LargeObjectInvocationViaProxy);
BENCHMARK(BM_LargeObjectInvocationViaProxy_Observer);
BENCHMARK(BM_LargeObjectInvocationViaProxy_Shared);
BENCHMARK(BM_LargeObjectInvocationViaVirtualFunction);
BENCHMARK(BM_LargeObjectInvocationViaVirtualFunction_Observer);
BENCHMARK(BM_LargeObjectInvocationViaVirtualFunction_Shared);

} // namespace
