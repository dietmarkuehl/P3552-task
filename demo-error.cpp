#include <iostream>
#include <system_error>
#include <beman/execution26/execution.hpp>
#include "demo-lazy.hpp"

namespace ex = beman::execution26;

demo::lazy<int> fun(int i)
{
    using on_exit = std::unique_ptr<char const, decltype([](auto msg){ std::cout << msg; })>;
    on_exit msg("> ");
    std::cout << "<";

    switch (i) {
    default: break;
    case 0:
        // successful execution: a later `co_return 0;` completes
        co_await ex::just(0);
        break;
    case 1:
        // the co_awaited work fails and an exception is thrown
        co_await ex::just_error(0);
        break;
    case 2:
        // the co_awaited work is stopped and the coroutine is stopped itself
        co_await ex::just_stopped();
        break;
    case 3:
        // successful return of a value
        co_return 17;
    case 4:
        // The coroutine completes with set_error(std::error_code{...})
        co_return demo::with_error(std::make_error_code(std::errc::io_error));

    case 5:
        co_await demo::with_error(std::make_error_code(std::errc::io_error));
        break;
    case 6:
        co_yield demo::with_error(std::make_error_code(std::errc::io_error));
        break;
    }

    co_return 0;
}

void print(char const* what, auto e)
{
    if constexpr (std::same_as<std::exception_ptr, decltype(e)>)
        std::cout << what << "(exception_ptr)\n";
    else
        std::cout << what << "(" << e << ")\n";
}

int main()
{
    for (int i{}; i != 7; ++i)
    {
        std::cout << "i=" << i << " ";
        ex::sync_wait(
            fun(i)
            | ex::then([](auto e){ print("then", e); })
            | ex::upon_error([](auto e){ print("upon_error", e); })
            | ex::upon_stopped([]{ std::cout << "stopped\n"; })
        );
    }
}
