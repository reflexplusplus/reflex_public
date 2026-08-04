#pragma once

#include "detail/traits.h"




//
//Primary API

namespace Reflex
{

	template <class SIG> class Function;

}






//
//Function 

template <class SIG>
class Reflex::Function : public std::function <SIG>
{
public:

	using Base = std::function<SIG>;

	using Signature = SIG;



	//lifetime

	Function();

	template < class BINDER, class = std::enable_if_t<std::is_constructible_v<Base, BINDER>> > Function(BINDER binder)
		: Base(binder)
		, m_bound(true)
	{
	}

	Function(Function && fn);

	Function(const Function & fn);

	Function(const Base & fn);

	Function(Base && fn);



	//init

	void Clear();



	//invoke

	template <class ... VARGS> decltype(auto) Invoke(VARGS && ... args) const;



	//assign

	Function & operator=(Function && fn);

	Function & operator=(const Function & fn);

	Function & operator=(const Base & fn);

	Function & operator=(Base && fn);

	template <class BINDER> Function & operator=(BINDER b);



	//info

	explicit operator bool() const { return m_bound; }



private:

	bool m_bound;
};

REFLEX_SET_TRAIT_TEMPLATED(Function, IsBoolCastable);




//
//impl

template <class SIG> inline Reflex::Function<SIG>::Function()
{
	Clear();
}

//template <class SIG> template <class BINDER> inline Reflex::Function<SIG>::Function(BINDER b)	//TEMPLATE && will catch everything, and wrongly move this is a known limitation of move ctr
//	: Base::function(std::move(b))
//	, m_bound(true)
//{
//}

template <class SIG> inline Reflex::Function<SIG>::Function(Function && fn)
	: Base::function(std::move(static_cast<Base&&>(fn)))
	, m_bound(fn.m_bound)
{
}

template <class SIG> inline Reflex::Function<SIG>::Function(const Function & fn)
	: Base::function(static_cast<const Base&>(fn))
	, m_bound(fn.m_bound)
{
}

template <class SIG> inline Reflex::Function<SIG>::Function(const Base & fn)
	: Base::function(fn)
	, m_bound(true)
{
	if (!fn) Clear();
}

template <class SIG> inline Reflex::Function<SIG>::Function(Base && fn)
	: Base::function(std::move(fn))
	, m_bound(true)
{
	if (!Base::operator bool()) Clear();
}

template <class SIG> inline void Reflex::Function<SIG>::Clear()
{
	Detail::FunctionTraits<SIG>::Clear(*this);

	m_bound = false;
}

template <class SIG> template <class ... VARGS> inline decltype(auto) Reflex::Function<SIG>::Invoke(VARGS && ... args) const
{
	return this->operator()(std::forward<VARGS>(args)...);
}

template <class SIG> inline Reflex::Function <SIG> & Reflex::Function<SIG>::operator=(Function && fn)
{
	Base::operator=(std::move(static_cast<Base&&>(fn)));

	m_bound = fn.m_bound;

	return *this;
}

template <class SIG> inline Reflex::Function <SIG> & Reflex::Function<SIG>::operator=(const Function & fn)
{
	Base::operator=(static_cast<const Base&>(fn));

	m_bound = fn.m_bound;

	return *this;
}

template <class SIG> inline Reflex::Function <SIG> & Reflex::Function<SIG>::operator=(const Base & fn)
{
	if (fn)
	{
		Base::operator=(fn);

		m_bound = true;
	}
	else
	{
		Clear();
	}

	return *this;
}

template <class SIG> inline Reflex::Function<SIG> & Reflex::Function<SIG>::operator=(Base && fn)
{
	if (fn)
	{
		Base::operator=(std::move(fn));

		m_bound = true;
	}
	else
	{
		Clear();
	}

	return *this;
}

template <class SIG> template <class BINDER> inline Reflex::Function<SIG> & Reflex::Function<SIG>::operator=(BINDER b)
{
	m_bound = true;

	Base::operator=(std::move(b));

	return *this;
}
