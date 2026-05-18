#pragma once

#include "../meta/auxtypes.h"
#include "threadvalidator.h"




//
//declaration

REFLEX_NS(Reflex::Detail)

template <class TYPE, bool TLS, class VARIANT = void> class ScopeOf;	//TODO make it refer to an external variable, so can be used per-instance

REFLEX_END




//
//Scopeable

template <class TYPE, bool TLS, class VARIANT>
class Reflex::Detail::ScopeOf
{
public:

	//types

	using Type = TYPE;



	//access

	static const TYPE & GetCurrent();



	//lifetime

	ScopeOf(const TYPE & value);

	~ScopeOf();



private:

	struct Static { static inline TYPE g_current = {}; };

	struct ThreadLocalStatic { static inline thread_local TYPE g_current = {}; };

	using StaticType = ConditionalType <TLS,ThreadLocalStatic,Static>;


	TYPE m_previous;

};




//
//

#define REFLEX_DEFINE_SCOPEOF_MT(TYPE,VARIANT,INIT_VALUE) template <> thread_local TYPE Reflex::Detail::ScopeOf<TYPE,true,VARIANT>::ThreadLocalStatic::g_current = INIT_VALUE




//
//implementation

template <class TYPE, bool TLS, class VARIANT> REFLEX_INLINE const TYPE & Reflex::Detail::ScopeOf<TYPE,TLS,VARIANT>::GetCurrent()
{
	return StaticType::g_current;
}

template <class TYPE, bool TLS, class VARIANT> REFLEX_INLINE Reflex::Detail::ScopeOf<TYPE,TLS,VARIANT>::ScopeOf(const TYPE & value)
	: m_previous(StaticType::g_current)
{
	StaticType::g_current = value;
}

template <class TYPE, bool TLS, class VARIANT> REFLEX_INLINE Reflex::Detail::ScopeOf<TYPE,TLS,VARIANT>::~ScopeOf()
{
	StaticType::g_current = m_previous;
}
