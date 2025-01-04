#include <iostream>
#include <cstdlib>
#include <beman/execution26/execution.hpp>
#include "demo-lazy.hpp"

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

    std::cout << "running demo::lazy\n";
    ex::sync_wait([]()->demo::lazy<void>{
        using on_exit = std::unique_ptr<char const, decltype([](auto msg){ std::cout << msg; })>;
        on_exit exit("lazy released\n");
        std::cout << "lazy started\n";
        co_await ex::just(0);
        co_await ex::just(0);
    }());
    std::cout << "demo::lazy done\n";
}
