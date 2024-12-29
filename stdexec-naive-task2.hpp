#include <coroutine>
#include <stdexec/execution.hpp>

namespace naive {
    template <typename T>
    struct task {
        template <typename R>
        struct completion { using type = stdexec::set_value_t(R); };
        template <>
        struct completion<void> { using type = stdexec::set_value_t(); };

        using sender_concept = stdexec::sender_t;
        using completion_signatures = stdexec::completion_signatures<
            typename completion<T>::type,
            stdexec::set_error_t(std::exception_ptr)
        >;

        struct void_t {};
        template <typename R>
        struct promise_base {
            using result_t = std::variant<std::monostate, T, std::exception_ptr>;
            result_t result;
            void return_value(T&& value) { this->result.template emplace<T>(std::forward<T>(value)); }
        };
        template <>
        struct promise_base<void> {
            using result_t = std::variant<std::monostate, void_t, std::exception_ptr>;
            result_t result;
            void return_void() { this->result.template emplace<void_t>(void_t{}); }
        };

        struct state_base {
            virtual void complete(promise_base<std::remove_cvref_t<T>>::result_t&) = 0;
        protected:
            ~state_base() = default;
        };

        struct promise_type
            : promise_base<std::remove_cvref_t<T>>
#ifdef USE_WITH_AWAITABLE_SENDERS
            , stdexec::with_awaitable_senders<promise_type>
#endif
        {
            struct final_awaiter {
                promise_type* promise;
                static constexpr bool await_ready() noexcept { return false; }
                void await_suspend(std::coroutine_handle<>) noexcept {
                    promise->state->complete(promise->result);
                }
                static void await_resume() noexcept {}
            };
            std::suspend_always initial_suspend() noexcept { return {}; }
            final_awaiter final_suspend() noexcept { return {this}; }
            void unhandled_exception() {
                this->result.template emplace<std::exception_ptr>(std::current_exception());
            }
            task get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this)}; }
#ifndef USE_WITH_AWAITABLE_SENDERS
            template <stdexec::sender Sender>
            auto await_transform(Sender&& sender) noexcept {
                return stdexec::as_awaitable(sender, *this);
            }
#endif
            state_base* state{};
            
            std::coroutine_handle<> unhandled_stopped() { return {}; }
        };

        template <typename Receiver>
        struct state
            : state_base
        {
            template <typename R, typename H>
            state(R&& r, H h)
                : receiver(std::forward<R>(r))
                , handle(std::move(h))
            {
            }
            std::remove_cvref_t<Receiver>       receiver;
            std::coroutine_handle<promise_type> handle;
            void start() & noexcept {
                handle.promise().state = this;
                handle.resume();
            }
            void complete(promise_base<std::remove_cvref_t<T>>::result_t& result) override {
                switch (result.index()) {
                case 1:
                    if constexpr (std::same_as<void, T>) {
                        stdexec::set_value(std::move(this->receiver));
                    }
                    else {
                        stdexec::set_value(std::move(this->receiver), std::move(std::get<1>(result)));
                    }
                    break;
                case 2:
                    stdexec::set_error(std::move(this->receiver), std::move(std::get<2>(result)));
                    break;
                }
            }
        };

        std::coroutine_handle<promise_type> handle;
        template <typename Receiver>
        state<Receiver> connect(Receiver receiver)
        {
            return state<Receiver>(std::forward<Receiver>(receiver), std::move(this->handle));
        }
    };
}
