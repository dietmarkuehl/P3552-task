// demo-affine.cpp                                                    -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution26/execution.hpp>
#include "demo-any_scheduler.hpp"
#include "demo-thread_pool.hpp"
#include <iostream>
#include <cassert>

namespace ex = beman::execution26;

// ----------------------------------------------------------------------------

namespace {
    struct test_receiver {
        using receiver_concept = ex::receiver_t;

        auto set_value(auto&&...) && noexcept {}
        auto set_error(auto&&) && noexcept {}
        auto set_stopped() && noexcept {}
    };
    static_assert(ex::receiver<test_receiver>);
}

std::ostream& fmt_id(std::ostream& out) { return out << std::this_thread::get_id(); }

int main() {
    demo::thread_pool pool;
    ex::sync_wait(ex::just() | ex::then([]{ std::cout << "main:" << fmt_id << "\n"; }));
    ex::sync_wait(ex::schedule(pool.get_scheduler()) | ex::then([]{ std::cout << "pool:" << fmt_id << "\n"; }));
    ex::sync_wait(ex::schedule(demo::any_scheduler(pool.get_scheduler())) | ex::then([]{ std::cout << "any: " << fmt_id << "\n"; }));
}
