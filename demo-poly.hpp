// demo-poly.hpp                                                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_POLY
#define INCLUDED_DEMO_POLY

#include <concepts>
#include <new>
#include <cstddef>

// ----------------------------------------------------------------------------

namespace demo {
    template <typename Base, std::size_t Size>
    class alignas(sizeof(double)) poly
    {
    private:
        std::array<std::byte, Size>  buf{};
        Base*                        ptr{};

    public:
        template <typename T, typename... Args>
        poly(T*, Args&&... args)
            : ptr(new(this->buf.data()) T(::std::forward<Args>(args)...))
        {
            static_assert(sizeof(T) <= Size);
        }
        poly(poly&& other): ptr(other.ptr->move(this->buf.data())){}
        poly(poly const& other): ptr(other.ptr->clone(this->buf.data())){}
        ~poly() { this->ptr->~Base(); }
        bool operator== (poly const& other) const { return other.ptr->equals(this); }
        Base* operator->() { return this->ptr; }
    };
}

// ----------------------------------------------------------------------------

#endif
