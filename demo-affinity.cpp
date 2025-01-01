// demo-affinity.cpp                                                  -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution26/execution.hpp>
#include "demo-any_scheduler.hpp"
#include "demo-task.hpp"
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

struct non_affine: demo::default_context {
    static constexpr bool scheduler_affine{false};
};

int main() {
    demo::thread_pool pool;
    ex::sync_wait(ex::just() | ex::then([]()noexcept{ std::cout << "main:" << fmt_id << "\n"; }));
    ex::sync_wait(ex::schedule(pool.get_scheduler()) | ex::then([]()noexcept{ std::cout << "pool:" << fmt_id << "\n"; }));
    ex::sync_wait(ex::schedule(demo::any_scheduler(pool.get_scheduler())) | ex::then([]()noexcept{ std::cout << "any: " << fmt_id << "\n"; }));
    ex::sync_wait([]()->demo::task<void> { std::cout << "coro:" << fmt_id << "\n"; co_return; }());
    std::cout << "scheduler affine:\n";
    ex::sync_wait([](auto& pool)->demo::task<void> {
        std::cout << "cor1:" << fmt_id << "\n";
        co_await (ex::schedule(pool.get_scheduler()) | ex::then([]{ std::cout << "then:" << fmt_id << "\n"; }));
        std::cout << "cor2:" << fmt_id << "\n";
        }(pool));
    std::cout << "not scheduler affine:\n";
    ex::sync_wait([](auto& pool)->demo::task<void, non_affine> {
        std::cout << "cor1:" << fmt_id << "\n";
        co_await (ex::schedule(pool.get_scheduler()) | ex::then([]{ std::cout << "then:" << fmt_id << "\n"; }));
        std::cout << "cor2:" << fmt_id << "\n";
        }(pool));
}