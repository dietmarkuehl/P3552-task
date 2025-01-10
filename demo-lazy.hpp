// demo-lazy.hpp                                                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_LAZY
#define INCLUDED_DEMO_LAZY

#include <beman/execution26/execution.hpp>
#include "demo-allocator.hpp"
#include "demo-any_scheduler.hpp"
#include "demo-stop_source.hpp"
#include "demo-inline_scheduler.hpp"
#include <concepts>
#include <coroutine>
#include <optional>
#include <type_traits>
#include <iostream> //-dk:TODO remove

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

    template <typename E>
    struct with_error {
        E error;

        // the members below are only need for co_await with_error{...}
        static constexpr bool await_ready() noexcept { return false; }
        template <typename Promise>
            requires requires(Promise p, E e){
                p.result.template emplace<E>(std::move(e));
                p.state->complete(p.result);
            }
        void await_suspend(std::coroutine_handle<Promise> handle)
            noexcept(noexcept(handle.promise().result.template emplace<E>(std::move(this->error))))
        {
            handle.promise().result.template emplace<E>(std::move(this->error));
            handle.promise().state->complete(handle.promise().result);
        }
        static constexpr void await_resume() noexcept {}
    };

    struct default_context
    {
    };

    template <typename T = void, typename C = default_context>
    struct lazy {
        using allocator_type   = demo::allocator_of_t<C>;
        using scheduler_type   = demo::scheduler_of_t<C>;
        using stop_source_type = demo::stop_source_of_t<C>;
        using stop_token_type  = decltype(std::declval<stop_source_type>().get_token());

        template <typename R>
        struct completion { using type = ex::set_value_t(R); };
        template <>
        struct completion<void> { using type = ex::set_value_t(); };

        using sender_concept = ex::sender_t;
        using completion_signatures = ex::completion_signatures<
            typename completion<T>::type,
            ex::set_error_t(std::exception_ptr),
            ex::set_error_t(std::error_code),
            ex::set_stopped_t()
        >;

        struct void_t {};
        template <typename R>
        struct promise_base {
            using result_t = std::variant<std::monostate, T, std::exception_ptr, std::error_code>;
            result_t result;
            void return_value(T&& value) { this->result.template emplace<T>(std::forward<T>(value)); }
            template <typename E>
            void return_value(demo::with_error<E> with) { this->result.template emplace<E>(with.error); }
        };
        template <>
        struct promise_base<void> {
            using result_t = std::variant<std::monostate, void_t, std::exception_ptr, std::error_code>;
            result_t result;
            void return_void() { this->result.template emplace<void_t>(void_t{}); }
        };

        struct state_base {
            virtual void complete(promise_base<std::remove_cvref_t<T>>::result_t&) = 0;
            virtual stop_token_type get_stop_token() = 0;
            virtual C& get_context() = 0;
        protected:
            virtual ~state_base() = default;
        };

        struct promise_type
            : promise_base<std::remove_cvref_t<T>>
        {
            template <typename... A>
            void* operator new(std::size_t size, A const&... a) {
                return demo::coroutine_allocate<C>(size, a...);
            }
            void operator delete(void* ptr, std::size_t size) {
                return demo::coroutine_deallocate<C>(ptr, size);
            }

            template <typename... A>
            promise_type(A const&... a)
                : allocator(demo::find_allocator<allocator_type>(a...))
            {
            }

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

            template <typename E>
            auto await_transform(with_error<E> with) noexcept { return std::move(with); }
            template <ex::sender Sender>
            auto await_transform(Sender&& sender) noexcept {
                if constexpr (std::same_as<demo::inline_scheduler, scheduler_type>)
                    return ex::as_awaitable(std::forward<Sender>(sender), *this);
                else
                    return ex::as_awaitable(ex::continues_on(std::forward<Sender>(sender), *(this->scheduler)), *this);
            }
            template <demo::awaiter Awaiter>
            auto await_transform(Awaiter&&) noexcept  = delete;

            template <typename E>
            final_awaiter yield_value(with_error<E> with) noexcept {
                this->result.template emplace<E>(with.error);
                return {this};
            }

            [[no_unique_address]] allocator_type allocator;
            std::optional<scheduler_type>        scheduler{};
            state_base*                          state{};
            
            std::coroutine_handle<> unhandled_stopped() {
                this->state->complete(this->result);
                return std::noop_coroutine();
            }

            struct env {
                promise_type const* promise;

                scheduler_type  query(ex::get_scheduler_t) const noexcept { return *promise->scheduler; }
                allocator_type  query(ex::get_allocator_t) const noexcept { return promise->allocator; }
                stop_token_type query(ex::get_stop_token_t) const noexcept { return promise->state->get_stop_token(); }
                template <typename Q, typename... A>
                    requires requires(C const& c, Q q, A&&...a){ q(c, std::forward<A>(a)...); }
                auto query(Q q, A&&... a) const noexcept { return q(promise->state->get_context(), std::forward<A>(a)...); }
            };

            env get_env() const noexcept { return {this}; }
        };

        template <typename Receiver>
        struct state_rep {
            std::remove_cvref_t<Receiver> receiver;
            C                             context;
            template <typename R>
            state_rep(R&& r): receiver(std::forward<R>(r)), context() {}
        };
        template <typename Receiver>
            requires requires{ C(ex::get_env(std::declval<std::remove_cvref_t<Receiver>&>())); }
                && (not requires(Receiver const& receiver) {
                    typename C::template env_type<decltype(ex::get_env(receiver))>;
                })
        struct state_rep<Receiver> {
            std::remove_cvref_t<Receiver> receiver;
            C                             context;
            template <typename R>
            state_rep(R&& r): receiver(std::forward<R>(r)), context(ex::get_env(this->receiver)) {}
        };
        template <typename Receiver>
            requires requires(Receiver const& receiver) {
                typename C::template env_type<decltype(ex::get_env(receiver))>;
            }
        struct state_rep<Receiver> {
            using upstream_env = decltype(ex::get_env(std::declval<std::remove_cvref_t<Receiver>&>()));
            std::remove_cvref_t<Receiver>               receiver;
            typename C::template env_type<upstream_env> own_env;
            C                                           context;
            template <typename R>
            state_rep(R&& r)
                : receiver(std::forward<R>(r))
                , own_env(ex::get_env(this->receiver))
                , context(this->own_env)
            {
            }
        };

        template <typename Receiver>
        struct state
            : state_base
            , state_rep<Receiver>
        {
            using operation_state_concept = ex::operation_state_t;
            using stop_token_t = decltype(ex::get_stop_token(ex::get_env(std::declval<Receiver>())));
            struct stop_link {
                stop_source_type& source;
                void operator()() const noexcept { source.request_stop(); }
            };
            using stop_callback_t = ex::stop_callback_for_t<stop_token_t, stop_link>;
            template <typename R, typename H>
            state(R&& r, H h)
                : state_rep<Receiver>(std::forward<R>(r))
                , handle(std::move(h))
            {
            }
            ~state() {
                if (this->handle) {
                    this->handle.destroy();
                }
            }
            std::coroutine_handle<promise_type> handle;
            stop_source_type                    source;
            std::optional<stop_callback_t>      stop_callback;

            void start() & noexcept {
                handle.promise().scheduler.emplace(ex::get_scheduler(ex::get_env(this->receiver)));
                handle.promise().state = this;
                handle.resume();
            }
            void complete(promise_base<std::remove_cvref_t<T>>::result_t& result) override {
                switch (result.index()) {
                case 0: // set_stopped
                    this->reset_handle();
                    ex::set_stopped(std::move(this->receiver));
                    break;
                case 1: // set_value
                    if constexpr (std::same_as<void, T>) {
                        reset_handle();
                        ex::set_value(std::move(this->receiver));
                    }
                    else {
                        auto r(std::move(std::get<1>(result)));
                        this->reset_handle();
                        ex::set_value(std::move(this->receiver), std::move(r));
                    }
                    break;
                case 2: // set_error
                    {
                        auto r(std::move(std::get<2>(result)));
                        this->reset_handle();
                        ex::set_error(std::move(this->receiver), std::move(r));
                    }
                    break;
                case 3: {
                        auto r(std::move(std::get<3>(result)));
                        this->reset_handle();
                        ex::set_error(std::move(this->receiver), std::move(r));
                    }
                    break;
                }
            }
            stop_token_type get_stop_token() override {
                if (this->source.stop_possible() && not this->stop_callback) {
                    this->stop_callback.emplace(ex::get_stop_token(ex::get_env(this->receiver)), stop_link(this->source));
                }
                return this->source.get_token();
            }
            C& get_context() override { return this->context; }
            void reset_handle() {
                this->handle.destroy();
                this->handle = {};
            }
        };

        std::coroutine_handle<promise_type> handle;
        lazy(std::coroutine_handle<promise_type> h): handle(std::move(h)) {}
#if 0
        lazy(lazy const& other) = delete;
#else
        lazy(lazy const& other): handle(other.handle) {
            std::cout << "lazy::lazy(lazy const&)\n" << std::flush;
            assert(nullptr == "copy called");
        }
#endif
        lazy(lazy&& other): handle(std::exchange(other.handle, {})) {}
        ~lazy() {
            if (this->handle) {
                this->handle.destroy();
            }
        }
        template <typename Receiver>
        state<Receiver> connect(Receiver receiver)
        {
            return state<Receiver>(std::forward<Receiver>(receiver), std::exchange(this->handle, {}));
        }
    };

    struct error_awaiter {
    };
}

// ----------------------------------------------------------------------------

#endif
