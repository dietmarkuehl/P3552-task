// demo-thread_pool.hpp                                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_THREAD_POOL
#define INCLUDED_DEMO_THREAD_POOL

// ----------------------------------------------------------------------------

#include <beman/execution26/execution.hpp>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>

namespace demo
{
    namespace ex = beman::execution26;
    namespace ex = beman::execution26;
    struct thread_pool
    {

        struct node {
            node* next;
            virtual void run() = 0;
        protected:
            ~node() = default;
        };

        std::mutex              mutex;
        std::condition_variable condition;
        node*                   stack{};
        bool                    stopped{false};
        std::thread             driver{[this]{
            while (std::optional<node*> n = [this]{
                std::unique_lock cerberus(mutex);
                condition.wait(cerberus, [this]{ return stopped || stack; });
                return this->stack
                    ? std::optional<node*>(std::exchange(this->stack, this->stack->next))
                    : std::optional<node*>()
                    ;
            }()) {
                (*n)->run();
            }
        }};

        thread_pool() = default;
        ~thread_pool() {
            this->stop();
            this->driver.join();
        }
        void stop() {
            {
                std::lock_guard cerberus(this->mutex);
                stopped = true;
            }
            this->condition.notify_one();
        }

        struct scheduler {
            template <typename Receiver>
            struct state final
                : thread_pool::node {
                using operation_state_concept = ex::operation_state_t;
                std::remove_cvref_t<Receiver> receiver;
                thread_pool*           pool;

                template <typename R>
                state(R&& r, thread_pool* p)
                    : node{}
                    , receiver(std::forward<R>(r))
                    , pool(p)
                {
                }
                void start() & noexcept {
                    {
                        std::lock_guard cerberus(this->pool->mutex);
                        this->next = std::exchange(this->pool->stack, this);
                    }
                    this->pool->condition.notify_one();
                }
                void run() override { ex::set_value(std::move(this->receiver)); }
            };
            struct sender {
                using sender_concept = ex::sender_t;
                using completion_signatures = ex::completion_signatures<ex::set_value_t()>;
                thread_pool* pool;
                template <typename Receiver>
                state<Receiver> connect(Receiver&& receiver) {
                    return state<Receiver>(std::forward<Receiver>(receiver), pool);
                }
            };
            thread_pool* pool;
            sender schedule() {
                return {this->pool};
            }
            bool operator== (scheduler const&) const = default;
        };
        scheduler get_scheduler() { return { this }; }
    };

} // namespace demo

// ----------------------------------------------------------------------------

#endif
