#include <iostream>
#include <unifex/async_scope.hpp>
#include <unifex/sync_wait.hpp>
#include <unifex/just.hpp>
#include <unifex/then.hpp>
#include <unifex/task.hpp>

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
    unifex::sync_wait(unifex::just());
    std::cout << "just done\n";

    std::cout << "running unifex::task\n";
    unifex::sync_wait([]()->unifex::task<void> {
        std::cout << "first co_await\n";
        co_await unifex::just();
        std::cout << "second co_await\n";
        co_await unifex::just();
        std::cout << "returning from task\n";
    }());
    std::cout << "unifex::task done\n";
}
