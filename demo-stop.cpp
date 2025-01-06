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

    ex::sync_wait(
        ex::detail::write_env(
            []->demo::lazy<std::uint64_t> {
            ex::inplace_stop_token token(co_await ex::read_env(ex::get_stop_token));
            std::uint64_t count{};
            while (!token.stop_requested()) {
                    ++count;
            }
            std::cout << "count=" << count << "\n";
            co_return count;
        }(),
            ex::detail::make_env(ex::get_stop_token, source.get_token())
        )
    );
    
    t.join();
}