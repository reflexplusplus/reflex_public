#pragma once

#include "../object/functions/functions.h"




//
//Detail

namespace Reflex::Detail
{

	template <class TYPE> struct SilentReference;

	template <class auto_t> void SilentRelease(auto_t && objectref);

}




//
//SilentReference

template <class TYPE> 
struct Reflex::Detail::SilentReference : public CommonReference <TYPE>
{
public:

	REFLEX_NONCOPYABLE(SilentReference);

	REFLEX_INLINE SilentReference(TYPE * ptr)
		: CommonReference<TYPE>(ptr)
	{
		Retain(CommonReference<TYPE>::m_object);
	}

	REFLEX_INLINE SilentReference(TYPE & ref)
		: CommonReference<TYPE>(&ref)
	{
		Retain(CommonReference<TYPE>::m_object);
	}

	REFLEX_INLINE SilentReference(const CommonReference <TYPE> & ref)
		: CommonReference<TYPE>(ref.Adr())
	{
		Retain(CommonReference<TYPE>::m_object);
	}

	REFLEX_INLINE ~SilentReference()
	{
		SilentRelease(CommonReference<TYPE>::m_object);
	}
};




//
//impl

template <class auto_t> REFLEX_INLINE void Reflex::Detail::SilentRelease(auto_t && objectref)
{
	auto & object = Deref(objectref);

	using ObjectType = NonConstT<NonRefT<decltype(object)>>;

	if constexpr (kIsSingleThreadExclusive<ObjectType>)
	{
		--RemoveConst(object.GetActualRetainCount());
	}
	else
	{
		REFLEX_ATOMIC_DEC(RemoveConst(object.GetActualRetainCount()), 1);
	}
}
