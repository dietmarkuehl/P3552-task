#include <iostream>
#include <beman/execution26/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution26;

beman::task::task<void> fun(int i)
{
    switch (i) {
    default: break;
    case 0:
        try { co_await ex::just(0); } catch (...) { std::cout << "exception "; }
        break;
    case 1:
        // TODO try { co_await ex::just_error(0); } catch (...) { std::cout << "exception "; }
        break;
    case 2:
        // TODO try { co_await ex::just_stopped(); } catch (...) { std::cout << "exception "; }
        break;
    }
}

int main()
{
    for (int i{}; i != 3; ++i)
    {
        std::cout << "i=" << i << " ";
        ex::sync_wait(
            fun(i)
            | ex::then([]{ std::cout << "then\n"; })
            | ex::upon_error([](auto){ std::cout << "error\n"; })
            | ex::upon_stopped([]{ std::cout << "stopped\n"; })
        );
    }
}
