---
title: Add a Coroutine Lazy Type
document: D3552R0
date: 2024-12-29
audience:
    - Library Evolution Working Group (LEWG)
    - Library Working Group (LWG)
author:
    - name: Dietmar Kühl (Bloomberg)
      email: <dkuhl@bloomberg.net>
    - name: Maikel Nadolski
      email: <maikel.nadolski@gmail.com>
toc: false
---

C++20 added support for coroutines which can improve the experience
writing asynchronous code. C++26 added the sender/receiver model
for a general interface to asynchronous operations. The expectation
is that users would use the framework using some coroutine type. To
support that a suitable class needs to be defined and this proposal
is providing such a definition.

# The Name

Just to get it out of the way: the class (template) used to implement
a coroutine task needs to have a name. In previous discussion [SG1
requested](https://wiki.edg.com/bin/view/Wg21rapperswil2018/P1056R0)
that the name `task` is retained and LEWG choose `lazy` as an
alternative.  It isn't clear whether the respective reasoning is
still relevant. To the authors the name matters much less than
various other details of the interface.  Thus, the text is written
in terms of `lazy`. The name is easily changed (prior to standarisation)
if that is desired.

# Prior Work

This proposal isn't the first to propose a coroutine type. Prior proposals
didn't see any recent (post introduction of sender/receiver) update although
corresponding proposals were discussed informally in multiple occassions.
There are also implementations of coroutine types based on a sender/receiver
model in active use. This section provides an overview of this prior work
and where relevant of corresponding discussions. This section is primarily
for motivating requirements and describing some points in the design space.

## [P1056](https://wg21.link/P1056): Add lazy coroutine (coroutine task) type

The paper describes a `task`/`lazy` type (in
[P1056r0](https://wg21.link/P1056r0) the name was `task`; the primary
change for [P1056r1](https://wg21.link/P1056r1) is changing the
name to `lazy`). The fundamental idea is to have a coroutine type
which can be `co_await`ed: the interface of `lazy` consists of move
constructor, deliberately no move assignment, a destructor, and
`operator co_await()`. The proposals doesn't go into much detail
on how to eventually use a coroutine but mentions that there could
be functions like `sync_await(task<To>)` to await completion of a
task (similar to
[`execution::sync_wait(sender)`](https://eel.is/c++draft/exec.sync.wait))
or a few variations of that.

A fair part of the paper argues why `future.then()` is _not_ a good
approach to model coroutines and their results. Using `future`
requires allocation, synchronisation, reference counting, and
scheduling which can all be avoided when using coroutines in a
structured way.

The paper also mentions support for [symmetric
transfer](https://wg21.link/P0913) and allocator support. Both of these
are details on how the coroutine is implemented.

[Discussion for P1056r0 in SG1](https://wiki.edg.com/bin/view/Wg21rapperswil2018/P1056R0)

- The task doesn't really have to anything with concurrency.
- Decomposing a task cheaply is fundamental. The [HALO
  Optimisations](https://wg21.link/P0981R0) help.
- The `task` isn't move assignable because there are better approaches
  than using containers to hold them. It is move constructible as there
  are no issues with overwriting a potentially live task.
- Resuming where things complete is unsafe but the task didn't want to
  impose any overhead on everybody.
- There can be more than one `task` type for different needs.
- Holding a mutex lock while `co_await`ing which may resume on a different
  thread is hazardous. Static analysers should be able to detect these cases.
- Votes confirmed the no move assignment and forwarding to LEWG assuming
  the name is not `task`.
- Votes against to deal with associated executors and a request to have
  strong language about transfer between threads.

## [P2506](https://wg21.link/P2506): std::lazy: a coroutine for deferred execution

This paper is effectively restating what [P1056](https://wg21.link/P1056)
said with the primary change being more complete proposed wording.
Although sender/receiver were discussed when the paper was written
but [`std::execution`](https://wg21.link/P2300) hadn't made it into
the working paper, the proposal did _not_ take a sender/receiver
interface into account.

Although there were mails seemingly scheduling a discussion in LEWG
I didn't manage to actually locate any.

## [cppcoro](https://github.com/lewissbaker/cppcoro)

This library contains multiple coroutine types, algorithms, and
some facilities for asynchronous work. For the purpose of this
discussion only the task types are of interest. There are two task
types
[`cppcoro::task`](https://github.com/lewissbaker/cppcoro?tab=readme-ov-file#taskt)
and
[`cppcoro::shared_task`](https://github.com/lewissbaker/cppcoro?tab=readme-ov-file#shared_taskt).
The key difference between `task` and `shared_task` is that the
latter can be copied and awaited by multiple other coroutines. As
a result `shared_task` always produces an lvalue and may have
slightly higher costs due to the need to maintain a reference count.

The types and algorithms are pre-sender/receiver and operate entirely
in terms for awaiters/awaitables. The interface of both task types
is a bit richer than that from
[P1056](https://wg21.link/P1056)/[P2506](https://wg21.link/P2506). Below
`t` is either a `cppcoro::task<T>` or a `cppcoro::shared_task<T>`:

- The task objects can be move constructed and move assigned; `shared_task<T>`
  object can also be copy constructed and copy assigned.
- Using `t.is_ready()` it can be queried if `t` has completed.
- Using `co_await t` awaits completion of `t`, yielding the result. The result
  may be throwing an exception if the coroutine completed by throwing.
- Using `co_await t.when_ready()` allows synchronizing with the completion of
  `t` without actually getting the result. This form of synchronization won't
  throw any excpetion.
- `cpproro::shared_task<T>` also supports equality comparisons.

In both cases the task starts suspended and is resumed when it is
`co_await`ed.  This way a continuation is known when the task is
resumed which is similar to `start(op)`ing an operation state `op`.
The coroutine body needs to use `co_await` or `co_return`. `co_await`
expects an awaitable or an awaiter as argument. Using `co_yield`
is not supported. The implementations support symmetic transfer but
doesn't mention allocators.

The `shared_task<T>` is similar to a `split(sender)`: in both cases
the same result is produced for multiple consumers. Correspondingly,
there isn't a need to support a separate `shared_task<T>` in a
sender/receiver world. Likewise, throwing of results can be avoid
by suitably rewriting the result of the `set_error` channel avoiding
the need for an operation akin to `when_ready()`.

## [libunifex](https://github.com/facebookexperimental/libunifex)

`libunifex` is an earlier implementation of the sender/receiver
ideas. Compared to `std::execution` it is lacking some of the
flexibilities. For example, it doesn't have a concept of environments
or domains. However, the fundamental idea of three completion
channels for success, failure, and cancellation and the general
shape of how these are used is present (even using the same names
for `set_value` and `set_error`; the equivalent of `set_stopped`
is called `set_done`). `libunifex` is in production use in multiple
places. The implementation includes a
[`unifex::task<T>`](https://github.com/facebookexperimental/libunifex/blob/main/include/unifex/task.hpp).

As `libunifex` is sender/receiver-based, its `unifex::task<T>` is
implemented such that `co_await` can deal with senders in addition
to awaitables or awaiters. Also, `unifex::task<T>` is _scheduler
affine_: the coroutine code resume on the same scheduler even if a
sender completed on a different scheduler. The corresponding scheduler
is taken from the receiver.  The exception for this rule is when
explicitly awaiting the result of `scheduler(scheduler)`.  The
relevant treatment is in the promise type's `await_transform()`:

- If a sender `s` which is result of `schedule(scheduler)` is
  `co_await`ed, the corresponding `scheduler` is installed as the
  task's scheduler and the task resume on the result of completing
  `s`.
- For both senders and awaiters being awaited, the coroutine
  will be resumed on the task's current scheduler when the task is
  scheduler affine unless the sender algorithm is tagged to inhibit
  transfer to the current scheduler (using a `static constexpr`
  member named `blocking` which is initialised to
  `blocking_kind::always_inline`).
- If a sender `s` is `co_await`ed it gets `connect`ed to a receiver
  provided by the task to form an awaiter holding the operation
  state which gets `start`ed by the awaiter's `await_suspend`. The
  receiver arranges for the `set_value` completion to become a
  result return from `await_resume`, a `set_error` completon to
  become an exception, and a `set_done` completion to not resume
  the task.

When `co_await`ing a sender `s` there can be at most one `set_value`
completion: if there is more than one `set_value` completion the
`await_transform` will do nothing and the result sending cannot be
`co_await`ed (unless it is also given an awaitable interface). The
result type of `co_await s` depends on the number of arguments to
`set_value`:

- If there is no for `set_value`  then the type of `co_await s`
    will be `void`.
- If there is exactly one argument of type `T` for `set_value`
    then the type of `co_await s` will be `T`.
- If there are more than one arguments for `set_value` then
    the type of `co_await s` will be `std::tuple<T1, T2, ...>`
    with the corresponding argument types.

If a receiver doesn't have a scheduler, it can't be `connect()`ed
to a `unifex::task<T>`. In particular, when using a
`unifex::async_scope s` it isn't possible to directly call `s.spawn(t)`
with a `unifex::task<T> t` as the `async_scope` doesn't provide a
scheduler.

`libunifex` provides some sender algorithms to transform the sender
result into something which may be more suitable to be `co_await`ed.
For example, `unifex::done_as_optional(sender)` turns a successful
completion for a type `T` into an `std::optional<T>` and the
cancellation completion `set_done` into a `set_value` completion
with a disengaged `std::optional<T>`.

The `unifex::task<T>` is itself a sender and can be used correspondingly.
To deal with scheduler affinity a type erase scheduler
`unifex::any_scheduler` is used.

The `unifex::task<T>` doesn't have allocator support. When creating
a task multiple objects are allocated on the heap: it seems there
is a total of 6 allocations for each `unifex::task<T>` being created.
After that, it seem the different `co_await`s don't use a separate
allocation.

The `unifex::task<T>` doesn't guard against stack overflow directly
guard against stack overflow. Due to rescheduling continuations on
a scheduler when the completion isn't always inlne, the issue only
arises when `co_await`ing many senders with `blocking_kind::always_inline`.

## [stdexec](https://github.com/NVIDIA/stdexec)

The
[`exec::task`](https://github.com/NVIDIA/stdexec/blob/main/include/exec/task.hpp)
in stdexec is somewhat similar to the unifex task with some choices
being different, though:

- The `exec::task<T, C>` is also scheduler affine. The chosen scheduler
    is unconditionally used for every `co_await`, i.e., there is no
    attempt made to avoid scheduling, e.g., when the `co_await`ed sender
    completes inline.
- Unlike unifex, it is OK if the receiver's environment doesn't provide
    a scheduler. In that case an inline scheduler is used. If an inline
    scheduler is used there is the possibility of stack overflow.
- It is possible to `co_await just_error(e)` and `co_await just_stopped()`,
    i.e., the sender isn't required to have a `set_value_t` completion.

The `exec::task<T, C>` also provides a _context_ `C`. An object of
this type becomes the environment for receivers `connect()`ed to
`co_await`ed senders. The default context provides access to the
task's scheduler. In addition an `in_place_stop_token` is provides
which forwards the stop requests from the environment of the receiver
which is connected to the task.

Like the unifex task `exec::task<T, C>` doesn't provide any allocator
support. When creating a task there are two allocations and an
additional allocation for each `co_await`.

# Objectives

Also see [sender/receiver issue 241](https://github.com/cplusplus/sender-receiver/issues/241).

Based on the prior work and discussions around corresponding coroutine
support there a number of required or desired features (listed in
no particular order):

1. A coroutine task needs to be awaiter/awaitable friendly. While
    that seems obvious, it is, e.g., possible to create an
    `await_transform` which is deleted for awaiters.
2. For sender algorithms it is common to adapt the results using
    suitable algorithms. When awaiting sender in a coroutine it may
    be annoying having to transform the result into a shape which
    is friendly to a coroutine use. Thus, it may be reasonable to
    support rewriting certain shapes of completion signatures into
    something different to make the use of senders easier in a
    coroutine task.
3. A coroutine task needs to be sender friendly: it is expected that
    asynchronous code is often written using coroutines awaiting
    senders. However, it isn't as obvious as not all senders will
    necessarily be awaitable. For example neither unifex nor stdexec
    support senders with more than one `set_value` completion.
4. It is possibly confusing and problematic if coroutines resume on
    a different execution context than the one they were suspended
    on.  Senders could, however, complete on an entirely different
    scheduler than where they started. As a result, a coroutine
    task should be scheduler affine by default, i.e., it should
    always resume on the same scheduler. There should probably be
    a way to opt out of scheduler affinity when the implications are
    well understood.
5. When using coroutines there will probably be an allocation at
    least for the coroutine frame. To support the use in environments
    where memory allocations using `new`/`delete` aren't supported
    the coroutine task should support allocations using allocators.
6. Receivers have associated environments which can support an open
    set of queries. Normally, queries on an environment can be
    forwarded to the environment of a `connect()`ed receiver.
    Since the coroutine types are determined before the coroutines
    receiver is known and the queries themselves don't specify a
    result type that isn't possible when a coroutine provides a
    receiver to a sender in a `co_await` expression. It should still
    be possible to provide a user-customisable environment from the
    receiver used by `co_await` expressions. One aspect of this
    environment is to forward stop requests to `co_await`ed child
    operations.
7. The coroutine should be able to indicate that it was cancelled,
    i.e., to get `set_stopped()` called on the task's receiver.
    `std::execution::with_awaitable_senders` already provided this
    ability senders being `co_await`ed but that doesn't necessarily
    extend to the coroutine implementation.
8. Similar to indicating that a task got cancelled it would be good
    if a task could indicate that an error occurred without throwing
    an exception which escapes from the coroutine.
9. In general a task has to assume that an exception escapes a the
    coroutine implementation. As a result, the task's completion
    signatures need to include `set_error_t(std::exception_ptr)`.
    If it can be indicated to the tas that no exception will escape
    the coroutine, this completion signature can be avoided.
10. When many `co_await`ed operations complete synchronously, there
    is a chance for stack overflow. It may be reasonable to have
    the implementation prevent that by using a suitable scheduler
    sometimes.
11. In some situations it can be useful to somehow schedule an
    asychronous clean-up operation which is triggered upon
    coroutine exit.

The algorithm `std::execution::as_awaitable` does turn a sender
into an awaitable and is expected to be used by custom written
coroutines. Likewise, it is intended that custom coroutines use
the CRTP class template `std::execution::with_awaiable_senders`.
It may be reasonable to adjust the functionality of these components
instead of definiting the functionality specific to a `lazy<...>`
coroutine task.

It is important to note that different coroutine task implementations
can live side by side: not all functionality has to be implemented
by the same coroutine task. The objective for this proposal is to
select a set of features which is provides a coroutine task suitable
for most uses. It may also be reasonable to provide some variations
as different names. A future revision of the standard or third party
libraries can also provide additional variations.

# Design

This section discusses various design options for achieving the
listed objectives. Most of the designs are independent of each other
and can be left out if the consensus is that it shouldn't be used
for whatever reason.

## Template Declaration for `lazy`

**TODO** turn into text

- Coroutines can `co_return` a value; if no argument is given or
    no `co_return` is used, the argument type is void
- The return type `T` can become a value `set_value_t(T)` or
    `set_value_t()` completion
- The return type cannot be deduced => it needs to be explicit on
    `lazy<T>`
- For various features it is beneficial to have an additional,
    defaulted template argument for a context (see below for details):
    - provide an initial environment for child senders
    - define the default scheduler affinity
    - define whether coroutine is `noexcept`
    - define allocator awareness
    - etc.
- The default context should be explicity named and provide "safe"
    defaults
    - inheriting from the default context can be used to override
        defaults
- Having different types for `lazy<T, C>` isn't a problem because
    these entities aren't stored in containers: instead they are
    maintained via something like `counting_scope` which starts the
    work and cleans up upon completion

## `lazy` constructors and assignments

**TODO**

- use guidance from [P1056](https://wg21.link/p1056) and [P2506](https://wg21.link/p2506)

## Result Type For `co_await`

**TODO** turn into text

- status quo based on `execution::as_awaitable(sender)`:
    - Exactly one `set_value_t(T...)` completion
    - `set_value_t()` => `void`
    - `set_value_t(T)` => `decay_t<T>`
    - `set_value_t(T0, T1, ...)` => `tuple<decay_t<T>...>`
- `co_await just_stopped()` could indicate cancellation; supported
   by stdexec.
- `co_await just_error(error)` would be reasonable for completions
    always indicating failure; supported by stdexec.
- the result type is already inconsistent instead of always
    `tuple<decay_t<T>...>` => support multiple `set_value_t`
    completions and produce a `variant<tuple<T...>...>`
- the last option is different from `co_await into_variant<sender>`
    as it treats `set_stopped_t()` and `set_error_t(error)` differently
- it may be reasonable to detect specific shapes of completion
    signatures and turn them into something more coroutine friendly like
    an `optional<T>` or an `expected<T, E>`; alternatively, support
    corresponding algorithms like `into_optional`, `into_expected`

Changes to the status quo would be applied to `as_awaitable(sender)`
to get consistent behavior for user-define coroutine tasks using
`as_awaitable` or `with_awaitable_sender`.

To make the proposed `co_await` result type more concrete, here is an
example using a custom sender type `custom` for which the completion signatures
can be explicitly specified:

    namespace ex = std::execution;

    template <typename... Completions>
    struct custom {
        using sender_concept = ex::sender_t;
        using completion_signatures = ex::completion_signatures<
            ex::set_stopped_t(), Completions...
        >;

        template <ex::receiver R>
        struct state {
            using operation_state_concept = ex::operation_state_t;
            std::decay_t<R> r;
            void start() & noexcept { ex::set_stopped(std::move(r)); }
        };

        template <ex::receiver R>
        state<R> connect(R&& r) { return { std::forward<R>(r) }; }
    };

    ex::lazy<void> fun() {
                                                          // co_await result type
        co_await ex::just();                              // void
        int i = co_await ex::just(0);                     // int
        auto[i, b, c] = co_await ex::just(0, true, 'c');  // tuple<int, bool, char>
        auto v = co_await custom<ex::set_value_t(), ex::set_value_t(int)>;
                                          // variant<tuple<>, tuple<int>>
    }

## Scheduler Affinity

**TODO** turn into text

**TODO** confirm whether it is possible have an inline scheduler which can
    immediately start the operation (or if corresponding detection can go
    into `continues_on`)

- Resume the coroutine on the same scheduler even if the sender completes
    on a different scheduler: `await_transform(sender)` returns
    `continues_on(as_awaitable(sender), scheduler)`.
- The used `scheduler` is determined using `get_scheduler(env)` where
    `env` is the environment of the task's receiver => the `scheduler`
    needs to be type-erased in some form
- What should happen if there is no `get_scheduler(env)`? Require a
    scheduler (unifex), use an inline scheduler (stdexec), or something
    else?
- Provide the definition of an inline scheduler to inhibit scheduler
    affinity, i.e., avoiding the cost of scheduling.
- Allow avoiding scheduling, probably using a suitable tag for the
    sender for cases where the sender is known not be scheduled on a
    different scheduler, e.g., `just(args...)` or `then(...)`, or
    when explicitly scheduling on a different scheduler
    `co_await schedule(s)` as is supported by unifex.
- When avoiding scheduling, probably the result of `as_awaitable(sender)`
    should be returned directly instead of going through `continues_on`
    with an inline scheduler.
- Scheduler affinity may potentially be controlled via the context with
    the default being scheduler affine.
- Note: scheduling on a non-inline scheduler helps with stack overflow.

## Allocator Support

**TODO** turn into text

- At least the coroutine frame is likely allocated on the heap
- Coroutines support passing the coroutine arguments to a
    `promise_type::operator new` and the `promise_type` constructor:
    using `allocator_arg_t` the leading arguments can be used to pass
    an allocator: `[](allocator_arg, alloc, args...) -> lazy<T>`
- The `promise_type::operator delete` only gets the pointer and
    optionally the size => if allocators are supported it always
    necessary to somehow store an allocator
- It may be reasonable to indicate allocator awareness via the context
    with the default being allocator aware
- The allocator, when provided, should be used for any allocation used
    by the coroutine implementation (if any)
- The allocator it should be used the `get_allocator(env)` for the
    coroutine and child receiver's environment. As the coroutine
    parameters do not affect the coroutine type, the allocator would
    be a `std::pmr::polymorphic_allocator<>`, if necessary with a
    memory resource forwarding allocation requests to the passed
    allocator.

## Environment Support

**TODO** turn into text

**TODO** look at forwarding specific queries based on traits and/or
    functionality specified for a given query

- At least, some environment members should be forwarded:
    - a stop token based on the receiver's `get_stop_token(get_env(r))`
        which can be done linking up an `inplace_stop_source`
    - a scheduler based on `get_scheduler(get_env(r))` requiring a
        type erased scheduler
    - if the coroutine task is allocator aware
        `get_allocator(get_env(child_receiver))` should provide
        corresponding `pmr::polymorphic_allocator<>`
- As the coroutine type is type erased, directly forwarding an
    environmnent isn't an option: the environment queries only provide
    a name but no concrete result type
- The context can function as an environment together with the forwarded
    items mentioned above

## Support For Requesting Cancellation/Stopped

**TODO** turn into text

- If `co_await just_stopped()` works, i.e., it is possible to have no
    `set_value_t(T...)` completion that would be a viable approach to
    indicate cancellation from within the coroutine
- Otherwise or in addition the technique for reporting errors from
    within the coroutine can be used ([see below](#support-for-reporting-an-error-without-exception))

## Support For Reporting An Error Without Exception

**TODO** turn into text

- The coroutine body can operate on the coroutine using `co_await`,
    `co_return`, or `co_yield`
- Using something like `co_return with_error(error)` could be used to
    complete with `set_error_t(error)` instead of using `set_value_t`
    - doing so would just require to detect a suitably named type like
        `with_error<E>`
    - sadly, a promise type can't have both `return_value(V)` and
        `return_void()`, i.e., errors could only be reported if the
        coroutine returns non-`void`
    - that's not the preferred approach
- A technique similar to `co_await sender` resulting in `set_stopped_t()`
    could be used for errors, e.g., when completing with
    `set_value_t(with_error<E>)` the `co_await` could result in the
    coroutine to never get resumed and the `lazy` sender would
    complete with `set_error_t(E)`
    - The logic is rather hidden and possibly confusing.
    - `co_await just_error(error)` throws an exception from the coroutine
    - `co_await just(with_error(error))` completes with `set_error_t(error)`
    - alternatively `co_await just_error(with_error(error))` does so
    - that's not the preferred approach
- `co_yield expression` isn't used for anything
    - `co_yield with_error(error)` can complete with `set_error_t(error)`
    - the completion is triggered from the awaiter's `await_suspend` such
        that the coroutine is suspended when completion is triggered
    - `co_yield just_error(error)` (or possibly even some general sender)
        does not work: `co_yield` doesn't await its argument
    - `co_yield with_stopped()` could be used for cancellation
    - note: the coroutine will not be resumed after a `co_yield`: instead
        the senders completes
    - that's the preferred approach
- To support error completions the completion signatures of `lazy<T, C>`
    need to declare the relevant completion options. This declaration can
    be made via the context `C`.

## Avoiding `set_error_t(exception_ptr)`

**TODO** turn into text

- It may be desirable to avoid the `set_error_t(exception_ptr)` completion
    especially in environments not supporting exceptions
- For the `lazy<T, C>` type it cannot be detected if the coroutine is
    declared as `noexcept`
- unifex supports a variation of `unifex::task<T>` which doesn't complete
    with `exception_ptr`
- It would be possible to detect based on the context `C` what the
    completion signatures for `lazy<T, C>` are.

## Avoiding Stack Overflow

**TODO** turn into text

- When always using scheduler affinity with a scheduler which doesn't
    immediately return the potential for stack overflows is avoided
- Using an inline scheduler or avoiding the scheduler can expose
    coroutines to stack overflows
- It should be possible to count the synchronous recursion depth
    and inject a scheduler which doesn't never immediately continues
- This does introduce a small overhead even in cases where there is
    no danger of stack overflows

## Asynchronous Clean-Up

**TODO** document an existing approach or come up with an approach

# Caveats

The use of coroutines introduces some issues which are entirely
independent of how coroutines are defined. Some these were brought
up on prior discussions but they aren't anything which can be
solved as part of any particular coroutine implementation. In
particular:

1. As `co_await`ing the result of an operation (or `co_yield`ing a
    a value) may suspend a coroutine, there is a potential to
    introduce problems when resource which meant to be held temporarily
    are held when suspending. For example, holding a lock to a mutex
    while suspending a coroutine can result in a different thread
    trying to release the lock when the coroutine is resumed on a
    differen thread.
2. Destroying a coroutine is only safe when it is suspended. For
    task implementation that means that it shall only call a
    completion handler once the coroutine is suspended. That part
    is under the control of the coroutine implementation. However,
    there is no way to guard against users explicitly destroying a
    coroutine from within its implementation or from another thread:
    that's akin to destroying an object while it being used.

Discussion of these issues should be delegated to suitable proposals
wanting to improve this situation in some form.

# Open Questions

This section lists open questsions based on the design discussion
above.

- Result type: expand `as_awaitable(sender)` to not require exactly
    on `set_value_t(T...)` completion?
- Scheduler affinity: should `lazy` support scheduler affinity?
- Scheduler affinity: require a `get_scheduler()` query on the
  receiver's environments?
- Scheduler affinity: add a definition for `inline_scheduler`
  (using whatever name) to support disabling scheduler affinity?
- **TODO** at the various other outstanding questions

# Proposed Wording

**TODO**: the intent is to have all relevant wording in place by the
time the paper needs to be submitted on 2025-01-13 and remove/augment
it according to the outcome of discussions in SG1 and/or LEWG.
