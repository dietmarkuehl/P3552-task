// demo-into_optional.hpp                                             -*-C++-*-
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef INCLUDED_DEMO_INTO_OPTIONAL
#define INCLUDED_DEMO_INTO_OPTIONAL

#include <optional>
#include <tuple>
#include <beman/execution26/execution.hpp>

// ----------------------------------------------------------------------------

namespace demo {
    namespace ex = beman::execution26;
inline constexpr struct into_optional_t
    : beman::execution26::sender_adaptor_closure<into_optional_t>
{
    template <ex::sender Upstream>
    struct sender
    {
        using upstream_t = std::remove_cvref_t<Upstream>;
        using sender_concept = ex::sender_t;
        upstream_t upstream;

        template <typename...> struct type_list {};

        template <typename T>
        static auto find_type(type_list<type_list<T>>) { return std::optional<T>{}; }
        template <typename T>
        static auto find_type(type_list<type_list<T>, type_list<>>) { return std::optional<T>{}; }
        template <typename T>
        static auto find_type(type_list<type_list<>, type_list<T>>) { return std::optional<T>{}; }
        template <typename T0, typename T1, typename... T>
        static auto find_type(type_list<type_list<T0, T1, T...>>) { return std::optional<std::tuple<T0, T1, T...>>{}; }
        template <typename T0, typename T1, typename... T>
        static auto find_type(type_list<type_list<T0, T1, T...>, type_list<>>) { return std::optional<std::tuple<T0, T1, T...>>{}; }
        template <typename T0, typename T1, typename... T>
        static auto find_type(type_list<type_list<>, type_list<T0, T1, T...>>) { return std::optional<std::tuple<T0, T1, T...>>{}; }

        template <typename Env>
        auto get_type(Env&&) const {
            return find_type(ex::value_types_of_t<Upstream, std::remove_cvref_t<Env>, type_list, type_list>());
        }

        template <typename... E, typename... S>
        constexpr auto make_signatures(auto&& env, type_list<E...>, type_list<S...>) const {
            return ex::completion_signatures<
                ex::set_value_t(decltype(this->get_type(env))),
                ex::set_error_t(E)...,
                S...
                >();
        }
        template<typename Env>
        auto get_completion_signatures(Env&& env) const {
            return make_signatures(env,
                                   ex::error_types_of_t<Upstream, std::remove_cvref_t<Env>, type_list>{},
                                   std::conditional_t<
                                        ex::sends_stopped<Upstream, std::remove_cvref_t<Env>>,
                                        type_list<ex::set_stopped_t()>,
                                        type_list<>>{}
                                   );
        }

        template <typename Receiver>
        auto connect(Receiver&& receiver) && {
            return ex::connect(
                ex::then(std::move(this->upstream),
                    []<typename...A>(A&&... a)->decltype(get_type(ex::get_env(receiver))) {
                        if constexpr (sizeof...(A) == 0u)
                            return {};
                        else
                            return {std::forward<A>(a)...};
                }),
                std::forward<Receiver>(receiver)
            );
        }
    };

    template <typename Upstream>
    sender<Upstream> operator()(Upstream&& upstream) const { return {std::forward<Upstream>(upstream)}; }
} into_optional{};
}

// ----------------------------------------------------------------------------

#endif
