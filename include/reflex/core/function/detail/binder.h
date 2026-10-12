#pragma once

#include "invoke.h"
#include <tuple>

namespace Reflex::Detail
{
    template <UInt INDEX> struct FunctionPlaceholder {};

    template <class TYPE> struct FunctionReference {
        TYPE * pointer;
        TYPE & Get() const { return *pointer; }
    };
    template <class FN, class... BOUND> class FunctionBinder;

    template <class TYPE, class TUPLE> TYPE & ResolveBound(TYPE & value, TUPLE &) { return value; }
    template <class TYPE, class TUPLE> const TYPE & ResolveBound(const TYPE & value, TUPLE &) { return value; }
    template <unsigned INDEX, class TUPLE>
    decltype(auto) ResolveBound(FunctionPlaceholder<INDEX>, TUPLE & args) {
        return std::get<INDEX>(std::move(args));
    }
    template <class TYPE, class TUPLE>
    TYPE & ResolveBound(FunctionReference<TYPE> value, TUPLE &) { return value.Get(); }
    template <class FN, class... BOUND, class TUPLE>
    decltype(auto) ResolveBound(FunctionBinder<FN, BOUND...> & value, TUPLE & args) {
        return std::apply(value, std::move(args));
    }
    template <class FN, class... BOUND, class TUPLE>
    decltype(auto) ResolveBound(const FunctionBinder<FN, BOUND...> & value, TUPLE & args) {
        return std::apply(value, std::move(args));
    }

    template <class FN, class... BOUND> class FunctionBinder
    {
        FN m_fn;
        std::tuple<BOUND...> m_bound;
        template <class SELF, class TUPLE, size_t... INDEX>
        static decltype(auto) Apply(SELF & self, TUPLE & args, std::index_sequence<INDEX...>) {
            return InvokeCallable(self.m_fn, ResolveBound(std::get<INDEX>(self.m_bound), args)...);
        }
    public:
        template <class F, class... VALUES>
        requires (!std::is_same_v<std::remove_cvref_t<F>, FunctionBinder>)
        FunctionBinder(F && fn, VALUES && ... values)
            : m_fn(std::forward<F>(fn)), m_bound(std::forward<VALUES>(values)...) {}
        template <class... ARGS> decltype(auto) operator()(ARGS && ... args) {
            auto tuple = std::forward_as_tuple(std::forward<ARGS>(args)...);
            return Apply(*this, tuple, std::index_sequence_for<BOUND...>{});
        }
        template <class... ARGS> decltype(auto) operator()(ARGS && ... args) const {
            auto tuple = std::forward_as_tuple(std::forward<ARGS>(args)...);
            return Apply(*this, tuple, std::index_sequence_for<BOUND...>{});
        }
    };
}
