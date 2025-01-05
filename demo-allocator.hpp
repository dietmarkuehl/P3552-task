// demo-allocator.hpp                                                 -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_ALLOCATOR
#define INCLUDED_DEMO_ALLOCATOR

#include <memory>
#include <memory_resource>
#include <new>
#include <cstddef>

// ----------------------------------------------------------------------------

namespace demo
{
    template <typename>
    struct allocator_of {
        using type = std::allocator<std::byte>;
    };
    template <typename Context>
        requires requires{ typename Context::allocator_type; }
    struct allocator_of<Context> {
        using type = typename Context::allocator_type;
    };
    template <typename Context>
    using allocator_of_t = typename allocator_of<Context>::type;

    template <typename Allocator>
    Allocator find_allocator() {
        return Allocator();
    }
    template <typename Allocator, typename Alloc, typename... A>
        requires requires(Alloc const& alloc) { Allocator(alloc); }
    Allocator find_allocator(std::allocator_arg_t const&, Alloc const& alloc, A const&...) {
        return Allocator(alloc);
    }
    template <typename Allocator, typename A0, typename... A>
    Allocator find_allocator(A0 const&, A const&...a) {
        return demo::find_allocator<Allocator>(a...);
    }

    template <typename C, typename... A>
    void* coroutine_allocate(std::size_t size, A const&...a) {
        using allocator_type = demo::allocator_of_t<C>;
        if constexpr (std::same_as<void, allocator_type>) {
            return ::operator new(size);
        }
        else {
            using traits = std::allocator_traits<allocator_type>;
            allocator_type alloc{demo::find_allocator<allocator_type>(a...)};
            std::byte* ptr{traits::allocate(alloc, size + sizeof(allocator_type))};
            new(ptr + size) allocator_type(alloc);
            return ptr;
        }
    }
    template <typename C>
    void coroutine_deallocate(void* ptr, std::size_t size) {
        using allocator_type = demo::allocator_of_t<C>;
        if constexpr (std::same_as<void, allocator_type>) {
            return ::operator delete(ptr, size);
        }
        else {
            using traits = std::allocator_traits<allocator_type>;
            void* vptr{static_cast<std::byte*>(ptr) + size};
            auto* aptr{static_cast<allocator_type*>(vptr)};
            allocator_type alloc(*aptr);
            aptr->~allocator_type();
            traits::deallocate(alloc, static_cast<std::byte*>(ptr), size);
        }
    }
}

// ----------------------------------------------------------------------------

#endif
