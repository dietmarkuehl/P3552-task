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
        Base*       pointer()       { return static_cast<Base*>(static_cast<void*>(buf.data())); }
        Base const* pointer() const { return static_cast<Base const*>(static_cast<void const*>(buf.data())); }

    public:
        template <typename T, typename... Args>
        poly(T*, Args&&... args) {
            new(this->buf.data()) T(::std::forward<Args>(args)...);
            static_assert(sizeof(T) <= Size);
        }
        poly(poly&& other) { other.pointer()->move(this->buf.data()); }
        poly(poly const& other) { other.pointer()->clone(this->buf.data()); }
        ~poly() { this->pointer()->~Base(); }
        bool operator== (poly const& other) const { return other.pointer()->equals(this); }
        Base* operator->() { return this->pointer(); }
    };
}

// ----------------------------------------------------------------------------

#endif
