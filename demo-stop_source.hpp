// demo-stop_source.hpp                                               -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_STOP_SOURCE
#define INCLUDED_DEMO_STOP_SOURCE

#include <beman/execution26/stop_token.hpp>

// ----------------------------------------------------------------------------

namespace demo {
    template <typename>
    struct stop_source_of {
        using type = beman::execution26::inplace_stop_source;
    };
    template <typename Context>
        requires requires{ typename Context::stop_source_type; }
    struct stop_source_of<Context> {
        using type = typename Context::stop_source_type;
    };
    template <typename Context>
    using stop_source_of_t = typename stop_source_of<Context>::type;
}

// ----------------------------------------------------------------------------

#endif
