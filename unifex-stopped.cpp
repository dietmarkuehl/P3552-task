#include <iostream>
#include <unifex/sync_wait.hpp>
#include <unifex/just.hpp>
#include <unifex/just_error.hpp>
#include <unifex/just_done.hpp>
#include <unifex/then.hpp>
#include <unifex/upon_error.hpp>
#include <unifex/upon_done.hpp>
#include <unifex/task.hpp>

unifex::task<void> fun(int i)
{
    switch (i) {
    default: break;
    case 0:
        try { co_await unifex::just(0); } catch (...) { std::cout << "exception "; }
        break;
    case 1:
        //try { co_await unifex::just_error(0); } catch (...) { std::cout << "exception "; }
        break;
    case 2:
        try { co_await unifex::just_done(); } catch (...) { std::cout << "exception "; }
        break;
    }
}

int main()
{
    for (int i{}; i != 3; ++i)
    {
        std::cout << "i=" << i << " ";
        unifex::sync_wait(
            fun(i)
            | unifex::then([]{ std::cout << "then\n"; })
            | unifex::upon_error([](auto){ std::cout << "error\n"; })
            | unifex::upon_done([]{ std::cout << "stopped\n"; })
        );
    }
}
