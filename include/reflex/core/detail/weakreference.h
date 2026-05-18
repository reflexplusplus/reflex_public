#pragma once

#include "../object/common_reference.h"
#include "../list.h"




//
//Deprecated API

REFLEX_NS(Reflex::Detail)

template <class TYPE> class LegacyWeakReference;

template <class TYPE> class LegacyWeakReferenceTarget;

REFLEX_END

REFLEX_NS(Reflex)

template <class TYPE> TYPE & Deref(Detail::LegacyWeakReference <TYPE> & weakreference) { return weakreference; }

template <class TYPE> TYPE & Deref(const Detail::LegacyWeakReference <TYPE> & weakreference) { return weakreference; }

REFLEX_END




//
//Detail::LegacyWeakReference

template <class TYPE>
class Reflex::Detail::LegacyWeakReference :
	public Item <LegacyWeakReference<TYPE>,false,NullType>,
	public CommonReference <TYPE>
{
public:

	//lifetime

	LegacyWeakReference();

	LegacyWeakReference(TYPE & object);

	LegacyWeakReference(const LegacyWeakReference & weakref);



	//access

	operator TRef <TYPE>() const { return LegacyWeakReference::Adr(); }


	void Clear();

	LegacyWeakReference & operator=(TYPE & object);

	LegacyWeakReference & operator=(const LegacyWeakReference & ref);


	using CommonReference<TYPE>::operator==;



private:

	friend class LegacyWeakReferenceTarget <TYPE>;

	using Target = LegacyWeakReferenceTarget<TYPE>;

};

template <class TYPE>
class Reflex::Detail::LegacyWeakReferenceTarget
{
public:

	//types

	using LegacyWeakReference = Reflex::Detail::LegacyWeakReference<TYPE>;



protected:

	//lifetime

	LegacyWeakReferenceTarget();

	LegacyWeakReferenceTarget(const LegacyWeakReferenceTarget & object);

	~LegacyWeakReferenceTarget();



private:

	friend Reflex::Detail::LegacyWeakReference <TYPE>;

	typename LegacyWeakReference::List m_list;

};




//
//impl

REFLEX_SET_TRAIT_TEMPLATED(Detail::LegacyWeakReference, IsBoolCastable);

template <class TYPE> inline Reflex::Detail::LegacyWeakReferenceTarget<TYPE>::LegacyWeakReferenceTarget()
{
}

template <class TYPE> inline Reflex::Detail::LegacyWeakReferenceTarget<TYPE>::LegacyWeakReferenceTarget(const LegacyWeakReferenceTarget & object)
{
}

template <class TYPE> inline Reflex::Detail::LegacyWeakReferenceTarget<TYPE>::~LegacyWeakReferenceTarget()
{
	for (auto itr = m_list.GetLast(); itr;)
	{
		itr->m_object = &GetNullInstance<TYPE>();

		itr = itr->GetPrev();
	}
}

template <class TYPE> inline Reflex::Detail::LegacyWeakReference<TYPE>::LegacyWeakReference()
	: CommonReference<TYPE>(&GetNullInstance<TYPE>())
{
}

template <class TYPE> inline Reflex::Detail::LegacyWeakReference<TYPE>::LegacyWeakReference(TYPE & object)
	: CommonReference<TYPE>(&object)
{
	this->Attach(Cast<Target>(this->m_object)->m_list);
}

template <class TYPE> inline Reflex::Detail::LegacyWeakReference<TYPE>::LegacyWeakReference(const LegacyWeakReference & weakref)
	: CommonReference<TYPE>(weakref.m_object)
{
	this->Attach(Cast<Target>(this->m_object)->m_list);
}

template <class TYPE> inline void Reflex::Detail::LegacyWeakReference<TYPE>::Clear()
{
	this->m_object = &GetNullInstance<TYPE>();

	this->Detach();
}

template <class TYPE> inline Reflex::Detail::LegacyWeakReference <TYPE> & Reflex::Detail::LegacyWeakReference<TYPE>::operator=(const LegacyWeakReference & weakref)
{
	this->m_object = RemoveConst(weakref.m_object);

	this->Attach(Cast<Target>(this->m_object)->m_list);

	return *this;
}

template <class TYPE> inline Reflex::Detail::LegacyWeakReference <TYPE> & Reflex::Detail::LegacyWeakReference<TYPE>::operator=(TYPE & object)
{
	this->m_object = &object;

	this->Attach(Cast<Target>(object).m_list);

	return *this;
}
