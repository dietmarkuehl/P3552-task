#include <iostream>
#include <stdexec/execution.hpp>
#include <exec/task.hpp>

exec::task<void> fun(int i)
{
    switch (i) {
    default: break;
    case 0:
        try { co_await stdexec::just(0); } catch (...) { std::cout << "exception "; }
        break;
    case 1:
        try { co_await stdexec::just_error(0); } catch (...) { std::cout << "exception "; }
        break;
    case 2:
        try { co_await stdexec::just_stopped(); } catch (...) { std::cout << "exception "; }
        break;
    }
}

int main()
{
    for (int i{}; i != 3; ++i)
    {
        std::cout << "i=" << i << " ";
        stdexec::sync_wait(
            fun(i)
            | stdexec::then([]{ std::cout << "then\n"; })
            | stdexec::upon_error([](auto){ std::cout << "error\n"; })
            | stdexec::upon_stopped([]{ std::cout << "stopped\n"; })
        );
    }
}
