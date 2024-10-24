// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <concepts>

#include <benchmark/benchmark.h>

#include "proxy_invocation_benchmark_context.h"

namespace {

std::vector<pro::proxy<InvocationTestFacade>> SmallObjectInvocationProxyTestData;
std::vector<std::unique_ptr<InvocationTestBase>> SmallObjectInvocationVirtualFunctionTestData;
std::vector<pro::proxy<InvocationTestFacade>> LargeObjectInvocationProxyTestData;
std::vector<std::unique_ptr<InvocationTestBase>> LargeObjectInvocationVirtualFunctionTestData;

void BM_SmallObjectInvocationViaProxy(benchmark::State& state) {
  for (auto _ : state) {
    for (auto& p : SmallObjectInvocationProxyTestData) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}
void BM_SmallObjectInvocationViaProxy_Setup(const benchmark::State&) {
  SmallObjectInvocationProxyTestData = GenerateSmallObjectInvocationProxyTestData();
}
void BM_SmallObjectInvocationViaProxy_Teardown(const benchmark::State&) {
  SmallObjectInvocationProxyTestData.clear();
}

void BM_SmallObjectInvocationViaVirtualFunction(benchmark::State& state) {
  for (auto _ : state) {
    for (auto& p : SmallObjectInvocationVirtualFunctionTestData) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}
void BM_SmallObjectInvocationViaVirtualFunction_Setup(const benchmark::State&) {
  SmallObjectInvocationVirtualFunctionTestData = GenerateSmallObjectInvocationVirtualFunctionTestData();
}
void BM_SmallObjectInvocationViaVirtualFunction_Teardown(const benchmark::State&) {
  SmallObjectInvocationVirtualFunctionTestData.clear();
}

void BM_LargeObjectInvocationViaProxy(benchmark::State& state) {
  for (auto _ : state) {
    for (auto& p : LargeObjectInvocationProxyTestData) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}
void BM_LargeObjectInvocationViaProxy_Setup(const benchmark::State&) {
  LargeObjectInvocationProxyTestData = GenerateLargeObjectInvocationProxyTestData();
}
void BM_LargeObjectInvocationViaProxy_Teardown(const benchmark::State&) {
  LargeObjectInvocationProxyTestData.clear();
}

void BM_LargeObjectInvocationViaVirtualFunction(benchmark::State& state) {
  for (auto _ : state) {
    for (auto& p : LargeObjectInvocationVirtualFunctionTestData) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}
void BM_LargeObjectInvocationViaVirtualFunction_Setup(const benchmark::State&) {
  LargeObjectInvocationVirtualFunctionTestData = GenerateLargeObjectInvocationVirtualFunctionTestData();
}
void BM_LargeObjectInvocationViaVirtualFunction_Teardown(const benchmark::State&) {
  LargeObjectInvocationVirtualFunctionTestData.clear();
}

BENCHMARK(BM_SmallObjectInvocationViaProxy)->Setup(BM_SmallObjectInvocationViaProxy_Setup)->Teardown(BM_SmallObjectInvocationViaProxy_Teardown);
BENCHMARK(BM_SmallObjectInvocationViaVirtualFunction)->Setup(BM_SmallObjectInvocationViaVirtualFunction_Setup)->Teardown(BM_SmallObjectInvocationViaVirtualFunction_Teardown);
BENCHMARK(BM_LargeObjectInvocationViaProxy)->Setup(BM_LargeObjectInvocationViaProxy_Setup)->Teardown(BM_LargeObjectInvocationViaProxy_Teardown);
BENCHMARK(BM_LargeObjectInvocationViaVirtualFunction)->Setup(BM_LargeObjectInvocationViaVirtualFunction_Setup)->Teardown(BM_LargeObjectInvocationViaVirtualFunction_Teardown);

}  // namespace
