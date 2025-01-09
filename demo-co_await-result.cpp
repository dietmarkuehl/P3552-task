// demo-co_await-result.cpp                                           -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "demo-lazy.hpp"
#include <beman/execution26/execution.hpp>
#include <iostream>
#include <cassert>

namespace ex = beman::execution26;

// ----------------------------------------------------------------------------

int main() {
    auto o = ex::sync_wait([]->demo::lazy<> {
        co_await ex::just();                               // void
        std::cout << "after co_await ex::just()\n";
        auto v = co_await ex::just(42);                    // int
        assert(v == 42);
        auto[i, b, c] = co_await ex::just(17, true, 'c');   // tuple<int, bool, char>
        assert(i == 17 && b == true && c == 'c');
        try {
            co_await ex::just_error(-1);                   // exception
            assert(nullptr == "never reached");
        } catch (int e) { assert(e == -1); }
        std::cout << "about to cancel\n";
        try { co_await ex::just_stopped(); } catch(...) {} // cancel: never resumed
        assert(nullptr == "never reached");
    }());
    assert(not o);
    std::cout << "done\n";
}
