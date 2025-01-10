// demo-stop.cpp                                                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <beman/execution26/execution.hpp>
#include <beman/execution26/stop_token.hpp>
#include "demo-lazy.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <cinttypes>

namespace ex = beman::execution26;

int main() {
    ex::stop_source source;
    std::thread t([&source]{
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
        source.request_stop();
    });

    struct context {
        struct xstop_source_type { // remove the x to disable the stop token
            static constexpr bool stop_possible() { return false; }
            static constexpr void request_stop() {}
            static constexpr beman::execution26::never_stop_token get_token() { return {}; }
        };
    };

    auto[result] = ex::sync_wait(
        ex::detail::write_env(
            []->demo::lazy<std::uint64_t, context> {
            auto token(co_await ex::read_env(ex::get_stop_token));
            std::uint64_t count{};
            while (!token.stop_requested() && count != 200'000'000u) {
                    ++count;
            }
            co_return count;
        }(),
        ex::detail::make_env(ex::get_stop_token, source.get_token())
        )
    ).value_or(std::uint64_t());
    std::cout << "result=" << result << "\n";
    
    t.join();
}