// demo-uery.cpp                                                     -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution26/execution.hpp>
#include "demo-lazy.hpp"
#include <iostream>
#include <cassert>
#include <cinttypes>

namespace ex = beman::execution26;

constexpr struct get_value_t {
    template <typename Env>
        requires requires(get_value_t const& self, Env const& e){ e.query(self); }
    int operator()(Env const& e) const { return e.query(*this); }
    constexpr auto query(const ::beman::execution26::forwarding_query_t&) const noexcept -> bool { return true; }
} get_value{};

struct simple_context {
    int value{};
    int query(get_value_t const&) const noexcept { return this->value; }
    simple_context(auto&& env): value(get_value(env)) {}
};

struct context {
    template <typename>
    struct env_type {
        int value{};
        env_type(auto const&& env): value(get_value(env) + 3) {} 
    };
    int value{};
    int query(get_value_t const&) const noexcept { return this->value; }
    context(auto& own): value(own.value) {}
};

int main() {
    ex::sync_wait(
        ex::detail::write_env(
            []->demo::lazy<void, simple_context> {
                auto value(co_await ex::read_env(get_value));
                std::cout << "value=" << value << "\n";
            }(),
            ex::detail::make_env(get_value, 42)
        )
    );
    ex::sync_wait(
        ex::detail::write_env(
            []->demo::lazy<void, context> {
                auto value(co_await ex::read_env(get_value));
                std::cout << "value=" << value << "\n";
            }(),
            ex::detail::make_env(get_value, 42)
        )
    );
}