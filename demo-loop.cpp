#include <iostream>
#include <coroutine>
#include <exception>
#include <beman/execution26/execution.hpp>
#include "demo-lazy.hpp"
#include "demo-inline_scheduler.hpp"

namespace ex = beman::execution26;

demo::lazy<void> loop()
{
    for (int i{}; i < 1000000; ++i)
        co_await ex::just(i);
}

int main()
{
    ex::sync_wait(ex::detail::write_env(
        [] -> demo::lazy<void> {
            for (int i{}; i < 1000000; ++i)
                co_await ex::just(i);
        }(),
        ex::detail::make_env(ex::get_scheduler, demo::inline_scheduler{})
    ));
}
