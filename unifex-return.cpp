#include <concepts>
#include <iostream>
#include <tuple>
#include <unifex/sync_wait.hpp>
#include <unifex/just.hpp>
#include <unifex/then.hpp>
#include <unifex/task.hpp>

namespace {
    template <typename... T>
    struct multi_sender
    {
        template <
            template <typename...> class Variant,
            template <typename...> class Tuple
        >
        using value_types = Variant<Tuple<T>... >;
        template <template <typename...> class Variant>
        using error_types = Variant<std::exception_ptr>;
        static constexpr bool sends_done = false;

        template <typename Receiver>
        struct state
        {
            std::remove_cvref_t<Receiver> receiver;

            void start() & noexcept {
                unifex::set_value(std::move(this->receiver), 17);
            }
        };

        template <typename Receiver>
        friend state<Receiver> tag_invoke(unifex::tag_t<unifex::connect>, multi_sender const&, Receiver&& receiver)
        {
            return { std::forward<Receiver>(receiver) };
        }
    };
}

int main()
{
    unifex::sync_wait([]()->unifex::task<void>{
        auto value = co_await unifex::just(17);
        static_assert(std::same_as<int, decltype(value)>);
        auto tuple = co_await unifex::just(17, true);
        static_assert(std::same_as<std::tuple<int, bool>, decltype(tuple)>);

        // exactly one set_value is required: co_await multi_sender<>();
        co_await multi_sender<int>();
        // exactly one set_value is required: co_await multi_sender<int, bool>();
    }());
}
