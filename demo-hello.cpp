// demo-hello.cpp                                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution26/execution.hpp>
#include "demo-lazy.hpp"
#include <iostream>

namespace ex = beman::execution26;

// ----------------------------------------------------------------------------

int main() {
    return std::get<0>(*ex::sync_wait([]->demo::lazy<int> {
        std::cout << "Hello, world!\n";
        co_return co_await ex::just(0);
    }()));
}
