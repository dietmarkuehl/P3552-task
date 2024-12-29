#include <iostream>
#include <stdexec/execution.hpp>
#include <exec/task.hpp>

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
    stdexec::sync_wait(stdexec::just(0));
    std::cout << "just done\n";

    std::cout << "running exec::task\n";
    stdexec::sync_wait([]()->exec::task<void>{
        using on_exit = std::unique_ptr<char const, decltype([](auto msg){ std::cout << msg; })>;
        on_exit exit("task released\n");
        std::cout << "task started\n";
        co_await stdexec::just(0);
        co_await stdexec::just(0);
    }());
    std::cout << "exec::task done\n";
}
