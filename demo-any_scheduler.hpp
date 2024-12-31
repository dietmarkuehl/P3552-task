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
            virtual void complete() = 0;
        };

        struct inner_state {
            struct receiver {
                using receiver_concept = ex::receiver_t;
                state_base* state;
                void set_value() && noexcept { this->state->complete(); }
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
            void complete() override { ex::set_value(std::move(this->receiver)); }
        };

        // sender implementation
        struct sender {
            struct base {
                virtual ~base() = default;
                virtual inner_state connect(state_base*) = 0;
            };
            template <ex::scheduler Scheduler>
            struct concrete
                : base {
                using sender_t = decltype(ex::schedule(std::declval<Scheduler>()));
                sender_t sender;

                template <ex::scheduler S>
                concrete(S&& s): sender(ex::schedule(std::forward<S>(s))) {}
                inner_state connect(state_base* b) override {
                    return inner_state(::std::move(sender), b);
                }
            };

            using sender_concept = ex::sender_t;
            using completion_signatures = ex::completion_signatures<ex::set_value_t()>;
            poly<base, 4 * sizeof(void*)> inner_sender;

            template <ex::scheduler S>
            sender(S&& s)
                : inner_sender(static_cast<concrete<S>*>(nullptr), std::forward<S>(s))
            {
            }

            template <ex::receiver R>
            state<R> connect(R&& r) { return state<R>(std::forward<R>(r), this->inner_sender); }
        };

        // scheduler implementation
        struct base {
            virtual ~base() = default;
            virtual sender schedule() = 0;
        };
        template <typename Scheduler>
        struct concrete
            : base {
            Scheduler scheduler;
            template <typename S>
            concrete(S&& s): scheduler(std::forward<S>(s)) {}
            sender schedule() { return sender(this->scheduler); }
        };

        poly<base, 4 * sizeof(void*)> scheduler;
        
        template <typename S>
        any_scheduler(S&& s): scheduler(static_cast<concrete<std::decay_t<S>>*>(nullptr), std::forward<S>(s)) {}
        sender schedule() { return this->scheduler->schedule(); }
    };
}

// ----------------------------------------------------------------------------

#endif
