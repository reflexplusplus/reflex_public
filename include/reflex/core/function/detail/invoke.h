#pragma once





namespace Reflex::Detail
{
    template <class FN, class OBJECT, class... ARGS> decltype(auto) inline InvokeMember(FN fn, OBJECT && object, ARGS && ... args)
    {
        if constexpr (std::is_member_function_pointer_v<FN>) 
        {
            if constexpr (requires { (std::forward<OBJECT>(object).*fn)(std::forward<ARGS>(args)...); })
                return (std::forward<OBJECT>(object).*fn)(std::forward<ARGS>(args)...);
            else return ((*std::forward<OBJECT>(object)).*fn)(std::forward<ARGS>(args)...);
        } 
        else 
        {
            static_assert(sizeof...(ARGS) == 0);
            if constexpr (requires { std::forward<OBJECT>(object).*fn; })
                return std::forward<OBJECT>(object).*fn;
            else return (*std::forward<OBJECT>(object)).*fn;
        }
    }

    template <class FN, class... ARGS> decltype(auto) inline InvokeCallable(FN && fn, ARGS && ... args)
    {
        if constexpr (std::is_member_pointer_v<std::decay_t<FN>>)
            return InvokeMember(fn, std::forward<ARGS>(args)...);
        else return std::forward<FN>(fn)(std::forward<ARGS>(args)...);
    }
}
