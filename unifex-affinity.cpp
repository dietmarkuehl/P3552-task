#include <iostream>
#include <unifex/async_scope.hpp>
#include <unifex/just.hpp>
#include <unifex/static_thread_pool.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/task.hpp>
#include <unifex/then.hpp>

int main()
{
    std::cout << "main=" << std::this_thread::get_id() << "\n";
    unifex::async_scope scope;
    unifex::sync_wait(unifex::just() | unifex::then([]{ std::cout << "then=" << std::this_thread::get_id() << "\n"; }));

    unifex::static_thread_pool outer_pool(1u);
    unifex::static_thread_pool pool(1u);
    unifex::sync_wait(unifex::schedule(outer_pool.get_scheduler()) | unifex::then([]{ std::cout << "out =" << std::this_thread::get_id() << "\n"; }));
    unifex::sync_wait(unifex::schedule(pool.get_scheduler()) | unifex::then([]{ std::cout << "pool=" << std::this_thread::get_id() << "\n"; }));

    unifex::sync_wait([](auto& pool)->unifex::task<void>{
        co_await unifex::schedule(pool.get_scheduler());
        std::cout << "coro=" << std::this_thread::get_id() << "\n";
    }(pool));
    unifex::sync_wait([](auto& pool)->unifex::task<void>{
        co_await (unifex::schedule(pool.get_scheduler()) | unifex::then([]{}));
        std::cout << "then=" << std::this_thread::get_id() << "\n";
    }(pool));
    // The unifex::task can't be connected if there isn't a scheduler:
    // auto r = scope.spawn([](auto& pool)->unifex::task<void>{
    //     co_return;
    // }(pool));
    //auto r = scope.spawn([](auto& pool)->unifex::task<void>{
    //    co_await unifex::just(0);
    //    std::cout << "scop=" << std::this_thread::get_id() << "\n";
    //}(pool));
    auto r = scope.spawn(unifex::on(outer_pool.get_scheduler(), [](auto& pool)->unifex::task<void>{
        co_await unifex::just(0);
        std::cout << "sco1=" << std::this_thread::get_id() << "\n";
    }(pool)));
    auto s = scope.spawn(unifex::on(outer_pool.get_scheduler(), [](auto& pool)->unifex::task<void>{
        co_await unifex::schedule(pool.get_scheduler());
        std::cout << "sco2=" << std::this_thread::get_id() << "\n";
    }(pool)));
    auto t = scope.spawn(unifex::on(outer_pool.get_scheduler(), [](auto& pool)->unifex::task<void>{
        co_await (unifex::schedule(pool.get_scheduler()) | unifex::then([]{}));
        std::cout << "sco3=" << std::this_thread::get_id() << "\n";
    }(pool)));

    unifex::sync_wait(scope.complete());
    pool.request_stop();
    outer_pool.request_stop();
}
