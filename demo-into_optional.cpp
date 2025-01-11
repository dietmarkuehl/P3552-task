// demo-into_optional.cpp                                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <iostream>
#include <optional>
#include <tuple>
#include <system_error>
#include <beman/execution26/execution.hpp>
#include "demo-into_optional.hpp"
#include "demo-lazy.hpp"

namespace ex = beman::execution26;

template <typename... S>
struct multi_sender {
    using sender_concept = ex::sender_t;
    using completion_signatures = ex::completion_signatures<S...>;

    template <typename Receiver>
    auto connect(Receiver&& receiver) const {
        return ex::connect(ex::just(), std::forward<Receiver>(receiver));
    }
};

template <typename T>
struct queue {
    multi_sender<ex::set_value_t(), ex::set_value_t(T, T)> async_pop() { return {}; }
};

int main() {
    queue<double> q;
    ex::sync_wait([](auto& q)->demo::lazy<> {
        //auto x = co_await ex::just(true) | into_optional;
        //auto x = co_await (ex::just(true) | into_optional);
        auto x = co_await demo::into_optional(ex::just(true));
        auto y = co_await demo::into_optional(q.async_pop());
    }(q));
}
