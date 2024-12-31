// demo-poly.hpp                                                      -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_POLY
#define INCLUDED_DEMO_POLY

#include <new>
#include <cstddef>

// ----------------------------------------------------------------------------

namespace demo {
    template <typename Base, std::size_t Size>
    class alignas(sizeof(double)) poly_base {
        std::array<std::byte, Size>  buffer{};
        void* data() { return buffer.data(); }
    
    protected:
        Base* ptr()  { return static_cast<Base*>(this->data()); }

    public:
        template <typename T, typename... Args>
        poly_base(T*, Args&&... args) {
            static_assert(sizeof(T) <= Size);
            new(this->data()) T(::std::forward<Args>(args)...);
        }
        ~poly_base() { this->ptr()->~Base(); }
        Base* operator->() { return this->ptr(); }
    };
    template <typename Base, std::size_t Size, bool = requires(Base* b){ b->move(nullptr); }>
    class poly
        : public poly_base<Base, Size> {
    public:
        using poly_base<Base, Size>::poly_base;
        poly(poly&&) { this->ptr()->move(this->data); }
    };
    template <typename Base, std::size_t Size>
    class poly<Base, Size, false>
        : public poly_base<Base, Size> {
    public:
        using poly_base<Base, Size>::poly_base;
    };
}

// ----------------------------------------------------------------------------

#endif
