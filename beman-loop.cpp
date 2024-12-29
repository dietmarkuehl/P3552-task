#include <iostream>
#include <fstream>
#include <coroutine>
#include <exception>
#include <beman/execution26/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution26;

beman::task::task<void> loop()
{
    for (int i{}; i < 1000000; ++i) {
        co_await ex::just(i);
        if (i % 10000 == 0) {
            std::cout << i << "\n";
        }
    }
}

int main()
{
    ex::sync_wait(loop());
    std::cout << "\n";
}
