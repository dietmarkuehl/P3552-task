#include <memory>
#include <iostream>
#include <coroutine>
#include <exception>
#include <system_error>
#include <beman/execution26/execution.hpp>
#include "demo-lazy.hpp"

namespace ex = beman::execution26;

int main()
{
    ex::sync_wait([]()->demo::lazy<void>{ co_await ex::just(); }());
    ex::sync_wait([]()->demo::lazy<void>{ co_await std::suspend_never(); }());
}
