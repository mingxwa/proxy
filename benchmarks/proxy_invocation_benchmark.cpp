// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <concepts>

#include <benchmark/benchmark.h>

#include "proxy_invocation_benchmark_context.h"

namespace {

template <std::invocable F>
class ContextualFixture : public benchmark::Fixture {
 public:
  void SetUp(::benchmark::State&) override { Context = F{}(); }

  decltype(F{}()) Context;
};
#define CONTEXTUAL_BENCHMARK(___NAME, ___F) BENCHMARK_TEMPLATE_F(ContextualFixture, ___NAME, decltype([] { return ___F(); }))

CONTEXTUAL_BENCHMARK(BM_SmallObjectInvocationViaProxy, GenerateSmallObjectInvocationProxyTestData)(benchmark::State& st) {
  for (auto _ : st) {
    for (auto& p : this->Context) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

CONTEXTUAL_BENCHMARK(BM_SmallObjectInvocationViaVirtualFunction, GenerateSmallObjectInvocationVirtualFunctionTestData)(benchmark::State& st) {
  for (auto _ : st) {
    for (auto& p : this->Context) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

CONTEXTUAL_BENCHMARK(BM_LargeObjectInvocationViaProxy, GenerateLargeObjectInvocationProxyTestData)(benchmark::State& st) {
  for (auto _ : st) {
    for (auto& p : this->Context) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

CONTEXTUAL_BENCHMARK(BM_LargeObjectInvocationViaVirtualFunction, GenerateLargeObjectInvocationVirtualFunctionTestData)(benchmark::State& st) {
  for (auto _ : st) {
    for (auto& p : this->Context) {
      int result = p->Fun();
      benchmark::DoNotOptimize(result);
    }
  }
}

}  // namespace
