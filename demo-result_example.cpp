// demo-result_example.cpp                                            -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "demo-lazy.hpp"
#include <beman/execution26/execution.hpp>
#include <cassert>

namespace ex = beman::execution26;

// ----------------------------------------------------------------------------

int main() {
    ex::sync_wait([]->demo::lazy<> {
        int result = co_await []->demo::lazy<int> { co_return 42; }();
        assert(result == 42);
    }());
}