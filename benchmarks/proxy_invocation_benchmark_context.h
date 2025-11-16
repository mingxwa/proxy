// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <memory>
#include <vector>

#include <proxy/proxy.h>

PRO_DEF_MEM_DISPATCH(MemObserveTestData, ObserveTestData);

template <class T>
struct TestDataObserverFacade
    : pro::facade_builder                                          //
      ::add_convention<MemObserveTestData, std::vector<T>() const> //
      ::build {};

PRO_DEF_MEM_DISPATCH(MemFun, Fun);

struct InvocationTestFacade : pro::facade_builder                   //
                              ::add_convention<MemFun, int() const> //
                              ::build {};

struct InvocationTestBase {
  virtual int Fun() const = 0;
  virtual ~InvocationTestBase() = default;
};

std::vector<pro::proxy<InvocationTestFacade>>
    GenerateSmallObjectInvocationProxyTestData();
pro::proxy<TestDataObserverFacade<pro::proxy_view<InvocationTestFacade>>>
    GenerateSmallObjectInvocationProxyTestData_Observer();
std::vector<pro::proxy<InvocationTestFacade>>
    GenerateSmallObjectInvocationProxyTestData_Shared();
std::vector<std::unique_ptr<InvocationTestBase>>
    GenerateSmallObjectInvocationVirtualFunctionTestData();
pro::proxy<TestDataObserverFacade<const InvocationTestBase*>>
    GenerateSmallObjectInvocationVirtualFunctionTestData_Observer();
std::vector<std::shared_ptr<InvocationTestBase>>
    GenerateSmallObjectInvocationVirtualFunctionTestData_Shared();
std::vector<pro::proxy<InvocationTestFacade>>
    GenerateLargeObjectInvocationProxyTestData();
pro::proxy<TestDataObserverFacade<pro::proxy_view<InvocationTestFacade>>>
    GenerateLargeObjectInvocationProxyTestData_Observer();
std::vector<pro::proxy<InvocationTestFacade>>
    GenerateLargeObjectInvocationProxyTestData_Shared();
std::vector<std::unique_ptr<InvocationTestBase>>
    GenerateLargeObjectInvocationVirtualFunctionTestData();
pro::proxy<TestDataObserverFacade<const InvocationTestBase*>>
    GenerateLargeObjectInvocationVirtualFunctionTestData_Observer();
std::vector<std::shared_ptr<InvocationTestBase>>
    GenerateLargeObjectInvocationVirtualFunctionTestData_Shared();
