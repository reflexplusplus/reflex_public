#pragma once

#include "detail/nullable.h"
#include "traits.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> class CommonReference;

}




//
//CommonReference

template <class TYPE>
class Reflex::CommonReference
{
public:

	//types

	using Type = TYPE;

	using IndexType = typename SubIndexType<NonConstT<TYPE>>::Type;



	//access

	explicit operator bool() const;

	operator TYPE&() const;

	TYPE * operator->() const;

	TYPE & operator*() const;

	TYPE * Adr() const { return m_object; }



	//'forwarders'

	auto begin() const { return m_object->begin(); }

	auto end() const { return m_object->end(); }

	auto rbegin() const { return m_object->rbegin(); }

	auto rend() const { return m_object->rend(); }

	decltype(auto) operator[](IndexType idx) const;

	decltype(auto) operator[](IndexType idx);



	//compare

	bool operator==(const CommonReference & ref) const;

	bool operator!=(const CommonReference & ref) const;

	bool operator<(const CommonReference & ref) const;


	bool operator<(const TYPE & ptr) const;



	//legacy

	bool operator<(const TYPE * ptr) const;



protected:

	template <class TYPE_> friend class CommonReference;



	//allow inheritance

	CommonReference() {}

	CommonReference(TYPE * ptr) : m_object(ptr) { REFLEX_ASSERT(ptr); }

	CommonReference(NoValue special_zero_init) : m_object(0) {}



	//assign

	CommonReference & operator=(const CommonReference & value) = default;

	CommonReference & operator=(TYPE & object);

	CommonReference & operator=(TYPE * object);


	TYPE * m_object;

};




//
//impl

template <class TYPE> decltype(auto) Reflex::CommonReference<TYPE>::operator[](IndexType idx) const
{
	return (*m_object)[idx];
}

template <class TYPE> decltype(auto) Reflex::CommonReference<TYPE>::operator[](IndexType idx)
{
	return (*m_object)[idx];
}

template <class TYPE> REFLEX_INLINE Reflex::CommonReference <TYPE> & Reflex::CommonReference<TYPE>::operator=(TYPE * ref)
{
	m_object = RemoveConst(ref);

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::CommonReference <TYPE> & Reflex::CommonReference<TYPE>::operator=(TYPE & ref)
{
	m_object = RemoveConst(&ref);

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::CommonReference<TYPE>::operator bool() const
{
	return m_object != &Detail::GetNullInstance<NonConstT<TYPE>>();
}

template <class TYPE> REFLEX_INLINE Reflex::CommonReference<TYPE>::operator TYPE&() const
{
	return *m_object;
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::CommonReference<TYPE>::operator->() const
{
	return m_object;
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::CommonReference<TYPE>::operator*() const
{
	return *m_object;
}

template <class TYPE> REFLEX_INLINE bool Reflex::CommonReference<TYPE>::operator==(const CommonReference & ref) const
{
	return m_object == ref.m_object;
}

template <class TYPE> REFLEX_INLINE bool Reflex::CommonReference<TYPE>::operator!=(const CommonReference & ref) const
{
	return m_object != ref.m_object;
}

template <class TYPE> REFLEX_INLINE bool Reflex::CommonReference<TYPE>::operator<(const CommonReference & ref) const
{
	return m_object < ref.m_object;
}

template <class TYPE> REFLEX_INLINE bool Reflex::CommonReference<TYPE>::operator<(const TYPE * ptr) const
{
	return m_object < ptr;
}
