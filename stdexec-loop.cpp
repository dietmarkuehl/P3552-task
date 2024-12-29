#include <iostream>
#include <stdexec/execution.hpp>
#include <exec/task.hpp>
#include <exec/async_scope.hpp>

exec::task<void> loop()
{
    for (int i{}; i < 1000000; ++i) {
        co_await stdexec::just(i);
        if (i % 10000 == 0) {
            std::cout << i << "\n";
        }
    }
}

int main()
{
    stdexec::sync_wait(loop());
    std::cout << "sync_wait(loop()) done\n" << std::flush;

    {
       exec::async_scope scope;
       scope.spawn(loop());
       stdexec::sync_wait(scope.on_empty());
       std::cout << "scope.spawn(loop()) done\n" << std::flush;
    }
}
