// demo-inline_scheduler.hpp                                          -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_INLINE_SCHEDULER
#define INCLUDED_DEMO_INLINE_SCHEDULER

#include <beman/execution26/execution.hpp>
#include <utility>
#include <type_traits>

// ----------------------------------------------------------------------------

namespace demo {
    namespace ex = beman::execution26;

    struct inline_scheduler {
        struct env {
            inline_scheduler query(ex::get_completion_scheduler_t<ex::set_value_t> const&) const noexcept {
                return {};
            }
        };
        template <ex::receiver Receiver>
        struct state {
            using operation_state_concept = ex::operation_state_t;
            std::remove_cvref_t<Receiver> receiver;
            void start() & noexcept {
                ex::set_value(std::move(receiver));
            }
        };
        struct sender {
            using sender_concept = ex::sender_t;
            using completion_signatures = ex::completion_signatures<ex::set_value_t()>;

            env get_env() const noexcept { return {}; }
            template <ex::receiver Receiver>
            state<Receiver> connect(Receiver&& receiver) { return { std::forward<Receiver>(receiver)}; }
        };
        static_assert(ex::sender<sender>);

        using scheduler_concept = ex::scheduler_t;
        inline_scheduler() = default;
        template <typename Scheduler>
        explicit inline_scheduler(Scheduler&&) { static_assert(ex::scheduler<Scheduler>); }
        sender schedule() noexcept { return {}; }
        bool operator== (inline_scheduler const&) const = default;
    };
    static_assert(ex::scheduler<inline_scheduler>);
}

// ----------------------------------------------------------------------------

#endif
