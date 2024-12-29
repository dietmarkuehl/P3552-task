#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <exec/task.hpp>
#include <stdexec/execution.hpp>
#include <iostream>

int main()
{
    std::cout << "main=" << std::this_thread::get_id() << "\n";
    exec::async_scope scope;
    stdexec::sync_wait(stdexec::just() | stdexec::then([]{ std::cout << "then=" << std::this_thread::get_id() << "\n"; }));

    exec::static_thread_pool pool(1u);

    stdexec::sync_wait(stdexec::schedule(pool.get_scheduler()) | stdexec::then([]{ std::cout << "then=" << std::this_thread::get_id() << "\n"; }));
    stdexec::sync_wait([](auto& pool)->exec::task<void>{
        co_await stdexec::schedule(pool.get_scheduler());
        std::cout << "coro=" << std::this_thread::get_id() << "\n";
    }(pool));
    scope.spawn([](auto& pool)->exec::task<void>{
        co_await stdexec::schedule(pool.get_scheduler());
        std::cout << "scop=" << std::this_thread::get_id() << "\n";
    }(pool));

    stdexec::sync_wait(scope.on_empty());
}
