#include <iostream>
#include <beman/execution26/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution26;

void* operator new(std::size_t size) {
    void* pointer(std::malloc(size));
    std::cout << "global new(" << size << ")->" << pointer << "\n";
    return pointer;
}
void  operator delete(void* pointer) noexcept {
    std::cout << "global delete(" << pointer << ")\n";
    return std::free(pointer);
}

int main()
{
    std::cout << "running just\n";
    ex::sync_wait(ex::just(0));
    std::cout << "just done\n";

    std::cout << "running exec::task\n";
    ex::sync_wait([]()->beman::task::task<void>{
        using on_exit = std::unique_ptr<char const, decltype([](auto msg){ std::cout << msg; })>;
        on_exit exit("task released\n");
        std::cout << "task started\n";
        co_await ex::just(0);
        co_await ex::just(0);
    }());
    std::cout << "beman::task::task done\n";
}
