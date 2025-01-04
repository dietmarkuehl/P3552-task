// demo-lazy.hpp                                                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_LAZY
#define INCLUDED_DEMO_LAZY

#include <concepts>
#include <coroutine>
#include <optional>
#include <iostream>
#include <beman/execution26/execution.hpp>
#include "demo-any_scheduler.hpp"

// ----------------------------------------------------------------------------

namespace demo
{
    namespace ex = beman::execution26;

    template <typename Awaiter>
    concept awaiter = ex::sender<Awaiter>
        && requires(Awaiter&& awaiter) {
            { awaiter.await_ready() } -> std::same_as<bool>;
            awaiter.disabled(); // remove this to get an awaiter unfriendly coroutine
        };

    struct with_error {
        std::error_code error;
    };

    struct default_context
    {
        static bool constexpr scheduler_affine{true};
    };

    template <typename T = void, typename C = default_context>
    struct lazy {
        template <typename R>
        struct completion { using type = ex::set_value_t(R); };
        template <>
        struct completion<void> { using type = ex::set_value_t(); };

        using sender_concept = ex::sender_t;
        using completion_signatures = ex::completion_signatures<
            typename completion<T>::type,
            ex::set_error_t(std::exception_ptr),
            ex::set_error_t(std::error_code)
        >;

        struct void_t {};
        template <typename R>
        struct promise_base {
            using result_t = std::variant<std::monostate, T, std::exception_ptr, std::error_code>;
            result_t result;
            void return_value(T&& value) { this->result.template emplace<T>(std::forward<T>(value)); }
            void return_value(demo::with_error error) { this->result.template emplace<std::error_code>(error.error); }
        };
        template <>
        struct promise_base<void> {
            using result_t = std::variant<std::monostate, void_t, std::exception_ptr, std::error_code>;
            result_t result;
            void return_void() { this->result.template emplace<void_t>(void_t{}); }
        };

        struct state_base {
            virtual void complete(promise_base<std::remove_cvref_t<T>>::result_t&) = 0;
        protected:
            virtual ~state_base() = default;
        };

        struct promise_type
            : promise_base<std::remove_cvref_t<T>>
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
            lazy get_return_object() { return { std::coroutine_handle<promise_type>::from_promise(*this)}; }
            template <ex::sender Sender>
            auto await_transform(Sender&& sender) noexcept {
                if constexpr (C::scheduler_affine)
                    return ex::as_awaitable(ex::continues_on(sender, *(this->scheduler)), *this);
                else
                    return ex::as_awaitable(sender, *this);
            }
            template <demo::awaiter Awaiter>
            auto await_transform(Awaiter&&) noexcept  = delete;

            final_awaiter yield_value(std::error_code error) {
                this->result.template emplace<std::error_code>(error);
                return {this};
            }

            std::optional<demo::any_scheduler> scheduler{};
            state_base* state{};
            
            std::coroutine_handle<> unhandled_stopped() { return {}; }

            struct env {
                promise_type const* promise;
                demo::any_scheduler query(ex::get_scheduler_t) const noexcept { return *promise->scheduler; }
            };

            env get_env() const noexcept { return {this}; }
        };

        template <typename Receiver>
        struct state
            : state_base
        {
            using operation_state_concept = ex::operation_state_t;
            template <typename R, typename H>
            state(R&& r, H h)
                : receiver(std::forward<R>(r))
                , handle(std::move(h))
            {
            }
            std::remove_cvref_t<Receiver>       receiver;
            std::coroutine_handle<promise_type> handle;
            void start() & noexcept {
                handle.promise().scheduler.emplace(ex::get_scheduler(ex::get_env(this->receiver)));
                handle.promise().state = this;
                handle.resume();
            }
            void complete(promise_base<std::remove_cvref_t<T>>::result_t& result) override {
                switch (result.index()) {
                case 1:
                    if constexpr (std::same_as<void, T>) {
                        ex::set_value(std::move(this->receiver));
                    }
                    else {
                        ex::set_value(std::move(this->receiver), std::move(std::get<1>(result)));
                    }
                    break;
                case 2:
                    ex::set_error(std::move(this->receiver), std::move(std::get<2>(result)));
                    break;
                case 3:
                    std::error_code rc{std::move(std::get<3>(result))};
                    if (this->handle) this->handle.destroy();
                    ex::set_error(std::move(this->receiver), std::move(rc));
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

    struct error_awaiter {
        static constexpr bool await_ready() noexcept { return false; }
        template <typename Promise>
        void await_suspend(std::coroutine_handle<Promise> handle) noexcept {
            handle.promise().result.template emplace<std::error_code>(std::error_code{});
            handle.promise().state->complete(handle.promise().result);
        }
        static constexpr void await_resume() noexcept {}
    };
}

// ----------------------------------------------------------------------------

#endif
