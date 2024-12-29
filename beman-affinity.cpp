#include <beman/task/task.hpp>
#include <beman/execution26/execution.hpp>
#include <iostream>
#include "demo-thread_pool.hpp"

namespace ex = beman::execution26;

int main()
{
    std::cout << "main=" << std::this_thread::get_id() << "\n";
    //exec::async_scope scope;
    ex::sync_wait(ex::just() | ex::then([]{ std::cout << "then=" << std::this_thread::get_id() << "\n"; }));

    demo::thread_pool pool;

    ex::sync_wait(ex::schedule(pool.get_scheduler()) | ex::then([]{ std::cout << "then=" << std::this_thread::get_id() << "\n"; }));
    ex::sync_wait([](auto& pool)->beman::task::task<void>{
        co_await ex::schedule(pool.get_scheduler());
        std::cout << "coro=" << std::this_thread::get_id() << "\n";
    }(pool));

#if 0
    scope.spawn([](auto& pool)->beman::task::task<void>{
        co_await ex::schedule(pool.get_scheduler());
        std::cout << "scop=" << std::this_thread::get_id() << "\n";
    }(pool));

    ex::sync_wait(scope.on_empty());
#endif
}
