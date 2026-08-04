#pragma once

#include "tref.h"
#include "object.h"
#include "../functions/cast.h"
#include "../functions/logic.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE, ReferenceSafeFlags SAFE> class Reference;

	template <class TYPE, ReferenceSafeFlags SAFE = kReferenceDefaultSafeFlags> using ConstReference = Reference <const TYPE,SAFE>;

}




//
//Reference

template <class TYPE, Reflex::ReferenceSafeFlags SAFE>
class Reflex::Reference : public CommonReference <TYPE>
{
public:

	//types

	using Type = TYPE;



	//lifetime

	Reference();

	Reference(NullObjectToken);		//helpful for default arg, e.g m_reference = kNullObject;

	Reference(NewObjectToken);		//helpful for default arg, e.g m_reference = kNewObject;

	Reference(TYPE * ptr);

	Reference(TYPE & object);

	Reference(const Reference & reference);

	template <class DERIVED> Reference(const Reference <DERIVED> & reference);

	Reference(TRef <TYPE> object);

	template <class DERIVED> Reference(TRef <DERIVED> reference);

	Reference(int) = delete;

	Reference(std::nullptr_t) = delete;

	~Reference();



	//assign

	void Clear();


	Reference & operator=(TYPE * object);

	Reference & operator=(TYPE & object);

	Reference & operator=(const Reference & reference);

	Reference & operator=(Reference && rhs);

	template <class DERIVED> Reference & operator=(const Reference <DERIVED> & reference);

	template <class DERIVED> Reference & operator=(Reference <DERIVED> && rhs);

	Reference & operator=(TRef <TYPE> tref);

	template <class DERIVED> Reference & operator=(TRef <DERIVED> tref);

	Reference & operator=(int) = delete;

	Reference & operator=(std::nullptr_t) = delete;



	//special

	TRef <NonConstT<TYPE>> RemoveConst() const { return Reflex::RemoveConst(Base::m_object); }



private:

	using Base = CommonReference <TYPE>;

	template <class TYPE_, ReferenceSafeFlags SAFE_> friend class Reference;

	static constexpr bool kNullable = bool(UInt8(SAFE) & UInt8(kReferenceNullable));

	void Validate(TYPE & type);

};

REFLEX_SET_TRAIT_TEMPLATED(Reference, IsBoolCastable);

template <class T, Reflex::ReferenceSafeFlags SAFE> struct Reflex::IsReflexReference < Reflex::Reference <T, SAFE> > { static constexpr bool value = true; static constexpr bool retaining = true; };




//
//impl

REFLEX_STATIC_ASSERT(sizeof(Reflex::AtomicUInt32) == sizeof(Reflex::UInt32));

REFLEX_STATIC_ASSERT(sizeof(Reflex::Reference<Reflex::Object>) == sizeof(void*));

REFLEX_NS(Reflex)

template <class TYPE> inline void ByRef(Reference <TYPE> value);	//intentionally not implemented

template <class auto_t> REFLEX_INLINE void Retain(auto_t && objectref)
{
	auto & object = Deref(objectref);

	using ObjectType = NonConstT<NonRefT<decltype(object)>>;

	if constexpr (kIsSingleThreadExclusive<ObjectType>)
	{
		object.RetainSt();
	}
	else
	{
		object.RetainMt();
	}
}

template <class auto_t> REFLEX_INLINE void Release(auto_t && objectref)
{
	auto & object = Deref(objectref);

	using ObjectType = NonConstT<NonRefT<decltype(object)>>;

	if constexpr (kIsSingleThreadExclusive<ObjectType>)
	{
		object.ReleaseSt();
	}
	else
	{
		object.ReleaseMt();
	}
}

REFLEX_END

REFLEX_NS(Reflex::Detail)

template <class TYPE, bool NULLABLE> struct NullAccess
{
	REFLEX_INLINE static TYPE * Get() { return &Detail::GetNullInstance<NonConstT<TYPE>>(); }
};

template <class TYPE> struct NullAccess <TYPE,false>
{
	REFLEX_INLINE static TYPE * Get() { return Cast<TYPE>(&Detail::GetNullInstance<NonConstT<TYPE>>()); }
};

template <class TYPE, class DERIVED> REFLEX_INLINE void SetReferenceCountedPointer(TYPE * & ptr, DERIVED * value)
{
	auto current = ptr;

	ptr = value;

	Retain(*value);

	Release(*current);
}

template <class TYPE, class auto_1> REFLEX_INLINE void SetReferenceCountedPointer(TRef <TYPE> & ptr, auto_1 && ref)
{
	auto current = ptr;

	ptr = Deref(ref);

	Retain(*ptr);

	Release(*current);
}

template <class auto_t> REFLEX_INLINE void ReleaseSilent(auto_t && objectref)
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

REFLEX_END

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE void Reflex::Reference<TYPE,SAFE>::Validate(TYPE & type)
{
	//this needs more work..  at least, in non strict mode, this should be logged to console

	if constexpr (SAFE & kReferenceOnHeap)
	{
		REFLEX_ASSERT(type.GetAllocator());
	}
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE, SAFE>::Reference(NullObjectToken)
	: Reference(&Detail::GetNullInstance<NonConstT<TYPE>>())
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference(TYPE * object)
{
	Retain(*object);

	Base::m_object = object;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference()
	: Reference(Detail::NullAccess<TYPE,kNullable>::Get())
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference(TYPE & object)
	: Reference(&object)
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference(const Reference & value)
	: Reference(value.m_object)
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> template <class DERIVED> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference(const Reference <DERIVED> & value)
	: Reference(value.m_object)
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference(TRef <TYPE> tref)
	: Reference(*tref)
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> template <class DERIVED> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::Reference(TRef <DERIVED> tref)
	: Reference(*Cast<TYPE>(tref.Adr()))
{
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE>::~Reference()
{
	Release(*Base::m_object);
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE void Reflex::Reference<TYPE,SAFE>::Clear()
{
	auto null = Detail::NullAccess<TYPE,kNullable>::Get();

	Retain(*null);

	Detail::SetReferenceCountedPointer(Base::m_object, null);
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference <TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(TYPE * object)
{
	REFLEX_ASSERT(object);

	Validate(*object);

	Detail::SetReferenceCountedPointer(Base::m_object, object);

	return *this;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference <TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(TYPE & object)
{
	Validate(object);

	Detail::SetReferenceCountedPointer(Base::m_object, &object);

	return *this;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference <TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(const Reference & reference)
{
	Validate(reference);

	Detail::SetReferenceCountedPointer(Base::m_object, reference.m_object);

	return *this;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> template <class DERIVED> REFLEX_INLINE Reflex::Reference <TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(const Reference <DERIVED> & reference)
{
	Validate(reference);

	Detail::SetReferenceCountedPointer(Base::m_object, reference.m_object);

	return *this;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference<TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(Reference && rhs)
{
	Reflex::Swap(Base::m_object, rhs.m_object);

	return *this;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> template <class DERIVED> REFLEX_INLINE Reflex::Reference<TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(Reference <DERIVED> && rhs)
{
	Cast<TYPE>(rhs.m_object);

	auto & casted = Reinterpret<Reference>(rhs);

	Reflex::Swap(Base::m_object, casted.m_object);

	return *this;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> REFLEX_INLINE Reflex::Reference <TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(TRef <TYPE> tref)
{
	Validate(tref);

	Detail::SetReferenceCountedPointer(Base::m_object, tref.Adr());

	return *this;
}

template <class TYPE, Reflex::ReferenceSafeFlags SAFE> template <class DERIVED> REFLEX_INLINE Reflex::Reference <TYPE,SAFE> & Reflex::Reference<TYPE,SAFE>::operator=(TRef <DERIVED> tref)
{
	Validate(tref);

	Detail::SetReferenceCountedPointer(Base::m_object, tref.Adr());

	return *this;
}
