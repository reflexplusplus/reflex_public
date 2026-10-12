#pragma once

#include "detail/traits.h"
#include <functional>




//
//Primary API

namespace Reflex
{

	template <class SIG> class Function;	//callable function object, extends std::function with safe null semantics

}




//
//Function 

template <class RTN, class... ARGS>
class Reflex::Function<RTN(ARGS...)> : public std::function <RTN(ARGS...)>
{
public:

	using Base = std::function<RTN(ARGS...)>;

	using Signature = RTN(ARGS...);

	template <class CALLABLE> static constexpr bool kBindable = [] { if constexpr (kIsType<std::decay_t<CALLABLE>, Function>) return false; else return std::is_constructible_v<Base, CALLABLE>; }();



	//lifetime

	Function();

	template <class CALLABLE> Function(CALLABLE callable) requires (kBindable<CALLABLE>);

	Function(Function && fn);

	Function(const Function & fn);

	Function(const Base & fn);

	Function(Base && fn);

	Function(nullptr_t) = delete;



	//init

	void Clear();



	//invoke

	RTN operator()(ARGS... args) const;

	template <class ... VARGS> decltype(auto) Invoke(VARGS && ... args) const;



	//assign

	Function & operator=(Function && fn);

	Function & operator=(const Function & fn);

	Function & operator=(const Base & fn);

	Function & operator=(Base && fn);

	template <class CALLABLE> Function & operator=(CALLABLE b) requires (kBindable<CALLABLE>);

	Function & operator=(nullptr_t) = delete;



	//info

	explicit operator bool() const { return m_bound; }



private:

	bool m_bound;
};

REFLEX_SET_TRAIT_TEMPLATED(Function, IsBoolCastable);




//
//impl

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function()
{
	Clear();
}

template <class RTN, class... ARGS> template <class CALLABLE> inline Reflex::Function<RTN(ARGS...)>::Function(CALLABLE callable) requires (kBindable<CALLABLE>)
	: Base(std::move(callable))
	, m_bound(Base::operator bool())
{
	REFLEX_ASSERT(m_bound);
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function(Function && fn)
	: Base::function(std::move(static_cast<Base&&>(fn)))
	, m_bound(fn.m_bound)
{
	fn.Base::operator=(nullptr);
	fn.m_bound = false;
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function(const Function & fn)
	: Base::function(static_cast<const Base&>(fn))
	, m_bound(fn.m_bound)
{
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function(const Base & fn)
	: Base::function(fn)
	, m_bound(Base::operator bool())
{
	REFLEX_ASSERT(m_bound);
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)>::Function(Base && fn)
	: Base::function(std::move(fn))
	, m_bound(Base::operator bool())
{
	REFLEX_ASSERT(m_bound);
}

template <class RTN, class... ARGS> inline void Reflex::Function<RTN(ARGS...)>::Clear()
{
	Detail::FunctionTraits<RTN(ARGS...)>::Clear(*this);

	m_bound = false;
}

template <class RTN, class... ARGS> inline RTN Reflex::Function<RTN(ARGS...)>::operator()(ARGS... args) const
{
	REFLEX_ASSERT(Base::operator bool());

	return Base::operator()(std::forward<ARGS>(args)...);
}

template <class RTN, class... ARGS> template <class ... VARGS> inline decltype(auto) Reflex::Function<RTN(ARGS...)>::Invoke(VARGS && ... args) const
{
	return this->operator()(std::forward<VARGS>(args)...);
}

template <class RTN, class... ARGS> inline Reflex::Function <RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(Function && fn)
{
	if (this == &fn) return *this;

	Base::operator=(std::move(static_cast<Base&&>(fn)));

	m_bound = fn.m_bound;
	fn.Base::operator=(nullptr);
	fn.m_bound = false;

	return *this;
}

template <class RTN, class... ARGS> inline Reflex::Function <RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(const Function & fn)
{
	Base::operator=(static_cast<const Base&>(fn));

	m_bound = fn.m_bound;

	return *this;
}

template <class RTN, class... ARGS> inline Reflex::Function <RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(const Base & fn)
{
	REFLEX_ASSERT(fn);
	
	Base::operator=(fn);
	m_bound = Base::operator bool();

	return *this;
}

template <class RTN, class... ARGS> inline Reflex::Function<RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(Base && fn)
{
	REFLEX_ASSERT(fn);
	
	Base::operator=(std::move(fn));
	m_bound = Base::operator bool();

	return *this;
}

template <class RTN, class... ARGS> template <class CALLABLE> inline Reflex::Function<RTN(ARGS...)> & Reflex::Function<RTN(ARGS...)>::operator=(CALLABLE b) requires (kBindable<CALLABLE>)
{
	Base::operator=(std::move(b));

	m_bound = Base::operator bool();
	REFLEX_ASSERT(m_bound);

	return *this;
}
