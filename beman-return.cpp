#include <iostream>
#include <beman/execution26/execution.hpp>
#include <beman/task/task.hpp>

namespace ex = beman::execution26;

namespace {
    template <typename... T>
    struct multi_sender {
        using sender_concept = ex::sender_t;
        using completion_signatures = ex::completion_signatures<ex::set_stopped_t(), ex::set_value_t(T)...>;

        template <typename Receiver>
        struct state {
            using operation_state_concept = ex::operation_state_t;

            std::remove_cvref_t<Receiver> receiver;
            void start() & noexcept {
                ex::set_stopped(std::move(receiver));
            }
        };

        template <typename Receiver>
        state<Receiver> connect(Receiver&& receiver) {
            return { std::forward<Receiver>(receiver) };
        }
    };
}

int main()
{
    ex::sync_wait([]()->beman::task::task<void>{
        int value = co_await ex::just(0);
        std::tuple<int, bool> tuple = co_await ex::just(0, true);
        // co_await ex::just_error(0);
        // co_await ex::just_stopped();
        // task requires exactly one set_value completion: co_await multi_sender<>{};
        int mv = co_await multi_sender<int>{};
        // task requires exactly one set_value completion: co_await multi_sender<int, bool>{};
    }());
    std::cout << "\n";
}
