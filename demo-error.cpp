#include <iostream>
#include <beman/execution26/execution.hpp>
#include "demo-task.hpp"

namespace ex = beman::execution26;

demo::task<int> fun(int i)
{
    using on_exit = std::unique_ptr<char const, decltype([](auto msg){ std::cout << msg << "\n"; })>;
    on_exit msg("task destroyed");
    std::cout << "task started\n";

    switch (i) {
    default: break;
    case 0:
        try { co_await ex::just(0); } catch (...) { std::cout << "exception "; }
        break;
    case 1:
        //try { co_await ex::just_error(0); } catch (...) { std::cout << "exception "; }
        break;
    case 2:
        //try { co_await ex::just_stopped(); } catch (...) { std::cout << "exception "; }
        break;
    case 3:
        try { co_await demo::error_awaiter{}; } catch (...) { std::cout << "exception "; }
        break;
    case 4:
        try { co_return 17; } catch (...) { std::cout << "exception "; }
        break;
    case 5:
        try { co_return demo::with_error(std::error_code{}); } catch (...) { std::cout << "exception "; }
        break;
    case 6:
        try { co_yield std::error_code{}; } catch (...) { std::cout << "exception "; }
        break;
    }

    std::cout << "co_return ";
    co_return 0;
}

int main()
{
    for (int i{}; i != 7; ++i)
    {
        std::cout << "i=" << i << " ";
        ex::sync_wait(
            fun(i)
            | ex::then([](auto&&...){ std::cout << "then\n"; })
            | ex::upon_error([](auto){ std::cout << "error\n"; })
            | ex::upon_stopped([]{ std::cout << "stopped\n"; })
        );
    }
}
