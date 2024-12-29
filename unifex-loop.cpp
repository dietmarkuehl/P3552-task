#include <iostream>
#include <unifex/sync_wait.hpp>
#include <unifex/just.hpp>
#include <unifex/task.hpp>

int main()
{
    unifex::sync_wait([]()->unifex::task<void>{
        for (int i{}; i < 1000000; ++i) {
            co_await unifex::just(i);
            if (i % 10000 == 0) {
                std::cout << i << "\n";
            }
        }
    }());
}
