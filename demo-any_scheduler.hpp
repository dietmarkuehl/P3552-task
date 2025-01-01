// demo-any_scheduler.hpp                                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_ANY_SCHEDULER
#define INCLUDED_DEMO_ANY_SCHEDULER

#include <beman/execution26/execution.hpp>
#include "demo-poly.hpp"
#include <new>
#include <utility>

namespace ex = beman::execution26;

// ----------------------------------------------------------------------------

namespace demo {
    struct any_scheduler {
        struct state_base {
            virtual ~state_base() = default;
            virtual void complete_value() = 0;
            virtual void complete_error(std::exception_ptr) = 0;
            virtual void complete_stopped() = 0;
        };

        struct inner_state {
            struct receiver {
                using receiver_concept = ex::receiver_t;
                state_base* state;
                void set_value() && noexcept { this->state->complete_value(); }
                void set_error(std::exception_ptr ptr) && noexcept { this->state->complete_error(std::move(ptr)); }
                template <typename E>
                void set_error(E e) {
                    try { throw std::move(e); }
                    catch (...) { this->state->complete_error(std::current_exception()); }
                }
                void set_stopped() && noexcept { this->state->complete_stopped(); }
            };
            static_assert(ex::receiver<receiver>);

            struct base {
                virtual ~base() = default;
                virtual void start() = 0;
            };
            template <ex::sender Sender>
            struct concrete: base {
                using state_t = decltype(ex::connect(std::declval<Sender>(), std::declval<receiver>()));
                state_t state;
                template <ex::sender S>
                concrete(S&& s, state_base* b)
                    : state(ex::connect(std::forward<S>(s), receiver{b}))
                {
                }
                void start() override { ex::start(state); }
            };
            demo::poly<base, 8u * sizeof(void*)> state;
            template <ex::sender S>
            inner_state(S&& s, state_base* b)
                : state(static_cast<concrete<S>*>(nullptr), std::forward<S>(s), b) {
            }
            void start() { this->state->start(); }
        };

        template <ex::receiver Receiver>
        struct state
            : state_base {
            using operation_state_concept = ex::operation_state_t;

            std::remove_cvref_t<Receiver> receiver;
            inner_state                   s;
            template <ex::receiver R, typename PS>
            state(R&& r, PS& ps)
                : receiver(std::forward<R>(r))
                , s(ps->connect(this))
            {
            }
            void start() & noexcept {
                this->s.start();
            }
            void complete_value() override { ex::set_value(std::move(this->receiver)); }
            void complete_error(std::exception_ptr ptr) override { ex::set_error(std::move(receiver), std::move(ptr)); }
            void complete_stopped() override { ex::set_stopped(std::move(this->receiver)); }
        };

        struct env {
            any_scheduler query(ex::get_completion_scheduler_t<ex::set_value_t> const&) const noexcept;
        };

        // sender implementation
        struct sender {
            struct base {
                virtual ~base() = default;
                virtual base* move(void*) = 0;
                virtual base* clone(void*) = 0;
                virtual inner_state connect(state_base*) = 0;
            };
            template <ex::scheduler Scheduler>
            struct concrete
                : base {
                using sender_t = decltype(ex::schedule(std::declval<Scheduler>()));
                sender_t sender;

                template <ex::scheduler S>
                concrete(S&& s): sender(ex::schedule(std::forward<S>(s))) {}
                base* move(void* buffer) override { return new(buffer) concrete(std::move(*this)); }
                base* clone(void*buffer) override { return new(buffer) concrete(*this); }
                inner_state connect(state_base* b) override {
                    return inner_state(::std::move(sender), b);
                }
            };

            using sender_concept = ex::sender_t;
            using completion_signatures = ex::completion_signatures<ex::set_value_t()>;
            poly<base, 4 * sizeof(void*)> inner_sender;

            template <ex::scheduler S>
            explicit sender(S&& s)
                : inner_sender(static_cast<concrete<S>*>(nullptr), std::forward<S>(s))
            {
            }
            sender(sender&&) = default;
            sender(sender const&) = default;

            template <ex::receiver R>
            state<R> connect(R&& r) { return state<R>(std::forward<R>(r), this->inner_sender); }

            env get_env() const noexcept { return {}; }
        };

        // scheduler implementation
        struct base {
            virtual ~base() = default;
            virtual sender schedule() = 0;
            virtual base* move(void* buffer) = 0;
            virtual base* clone(void*) = 0;
            virtual bool equals(base const*) const = 0;
        };
        template <ex::scheduler Scheduler>
        struct concrete
            : base {
            Scheduler scheduler;
            template <ex::scheduler S>
            explicit concrete(S&& s): scheduler(std::forward<S>(s)) {}
            sender schedule() override { return sender(this->scheduler); }
            base* move(void* buffer) override { return new(buffer) concrete(std::move(*this)); }
            base* clone(void*buffer) override { return new(buffer) concrete(*this); }
            bool equals(base const* o) const override {
                auto other{dynamic_cast<concrete const*>(o)};
                return other? this->scheduler == other->scheduler: false;
            }
        };

        poly<base, 4 * sizeof(void*)> scheduler;
        
        using scheduler_concept = ex::scheduler_t;

        template <typename S>
            requires (not std::same_as<any_scheduler, std::remove_cvref_t<S>>)
        explicit any_scheduler(S&& s): scheduler(static_cast<concrete<std::decay_t<S>>*>(nullptr), std::forward<S>(s)) {}
        any_scheduler(any_scheduler&& other) = default;
        any_scheduler(any_scheduler const& other) = default;
        sender schedule() { return this->scheduler->schedule(); }

        bool operator== (any_scheduler const&) const = default;
    };
    static_assert(ex::scheduler<any_scheduler>);
}

// ----------------------------------------------------------------------------

#endif
