#include <iostream>
#include <stdexec/execution.hpp>
#include <exec/task.hpp>

namespace {
    template <typename... T>
    struct multi_sender {
        using sender_concept = stdexec::sender_t;
        using completion_signatures = stdexec::completion_signatures<stdexec::set_stopped_t(), stdexec::set_value_t(T)...>;

        template <typename Receiver>
        struct state {
            std::remove_cvref_t<Receiver> receiver;
            void start() & noexcept {
                stdexec::set_stopped(std::move(receiver));
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
    stdexec::sync_wait([]()->exec::task<void>{
        int value = co_await stdexec::just(0);
        std::tuple<int, bool> tuple = co_await stdexec::just(0, true);
        co_await stdexec::just_error(0);
        co_await stdexec::just_stopped();
        co_await multi_sender<>{};
        int mv = co_await multi_sender<int>{};
        // at most one set_value completion: co_await multi_sender<int, bool>{};
    }());
    std::cout << "\n";
}
