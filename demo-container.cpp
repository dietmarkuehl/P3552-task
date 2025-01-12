// demo-container.cpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "demo-lazy.hpp"
#include <beman/execution26/execution.hpp>
#include <iostream>
#include <vector>
#include <cassert>

namespace ex = beman::execution26;

// ----------------------------------------------------------------------------

int main() {
    std::vector<demo::lazy<>> cont;
    cont.emplace_back([]->demo::lazy<> { co_return; }());
    cont.push_back([]->demo::lazy<> { co_return; }());
}