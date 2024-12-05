// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// This file contains example code from explicit_conversion_dispatch.md.

#include <iomanip>
#include <iostream>

#include "proxy.h"

struct IntConvertible : pro::facade_builder
    ::add_convention<pro::conversion_dispatch, int() const>
    ::build {};

int main() {
  pro::proxy<DoubleConvertible> p = pro::make_proxy<DoubleConvertible, short>(123);  // p holds an short
  std::cout << std::fixed << std::setprecision(10) << static_cast<int>(*p) << "\n";  // Prints: "123"
}
