#include <memory>
#include <iostream>
#include <coroutine>
#include <exception>
#include <system_error>
#include <beman/execution26/execution.hpp>
#include "demo-task.hpp"

namespace ex = beman::execution26;

int main()
{
    ex::sync_wait([]()->demo::task<void>{ co_await ex::just(); }());
    ex::sync_wait([]()->demo::task<void>{ co_await std::suspend_never(); }());
}
