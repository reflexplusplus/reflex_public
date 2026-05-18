#pragma once

#include "types.h"




//
//Primary API

REFLEX_NS(Reflex::SIMD)

template <class TYPE> class TypeV4;

REFLEX_END




//
//TypeV4

template <class TYPE>
class Reflex::SIMD::TypeV4
{
public:

	//type

	using SingleType = TYPE;

	using DataType = typename PlatformVectorType<TYPE,4>::Type;



	//lifetime

	TypeV4();

	TypeV4(TYPE value);

	TypeV4(TYPE a, TYPE b, TYPE c, TYPE d);

	TypeV4(const DataType & data);

	TypeV4(const TypeV4 <TYPE> & data);



	//cast

	operator DataType&();

	operator const DataType&() const;



	//set

	void Set(TYPE value);

	void Set(TYPE a, TYPE b, TYPE c, TYPE d);



	//assign

	TypeV4 & operator=(const TypeV4 & value);

	TypeV4 & operator=(const DataType & data);

	TypeV4 & operator=(TYPE value);



	//math

	TypeV4 & operator+=(const TypeV4 & value);

	TypeV4 & operator-=(const TypeV4 & value);

	TypeV4 & operator*=(const TypeV4 & value);

	TypeV4 & operator/=(const TypeV4 & value);



	//access

	TYPE & operator[](UInt idx);

	const TYPE & operator[](UInt idx) const;

	TYPE ReadFirst() const;

	Quad <TYPE> Read() const;

	void Read(TYPE * unaligned) const;


	TYPE * GetData();

	const TYPE * GetData() const;



	DataType data;

};




//
//impl

REFLEX_NS(Reflex::SIMD)

REFLEX_INLINE void AssertAlignment(const void * ptr)
{
	REFLEX_ASSERT(!(ToUIntNative(ptr) & 15));
}

REFLEX_END

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4()
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(TYPE value)
	: data(PlatformVectorType<TYPE,4>::Set1(value))
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(TYPE a, TYPE b, TYPE c, TYPE d)
	: data(PlatformVectorType<TYPE,4>::Set(d, c, b, a))
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(const DataType & data)
	: data(data)
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(const TypeV4 <TYPE> & data)
	: data(data.data)
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::operator typename Reflex::SIMD::TypeV4<TYPE>::DataType&()
{
	return data;
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::operator const typename Reflex::SIMD::TypeV4<TYPE>::DataType&() const
{
	return data;
}

template <class TYPE> REFLEX_INLINE void Reflex::SIMD::TypeV4<TYPE>::Set(TYPE value)
{
	data = PlatformVectorType<TYPE,4>::Set1(value);
}

template <class TYPE> REFLEX_INLINE void Reflex::SIMD::TypeV4<TYPE>::Set(TYPE a, TYPE b, TYPE c, TYPE d)
{
	data = PlatformVectorType<TYPE,4>::Set(d, c, b, a);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator=(const TypeV4 & value)
{
	data = value.data;

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator=(const DataType & value)
{
	data = value;

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator=(TYPE value)
{
	Set(value);

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator+=(const TypeV4 & value)
{
	*this = (*this) + value;

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator-=(const TypeV4 & value)
{
	*this = (*this) - value;

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator*=(const TypeV4 & value)
{
	*this = (*this) * value;

	return *this;
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator/=(const TypeV4 & value)
{
	*this = (*this) / value;

	return *this;
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::SIMD::TypeV4<TYPE>::operator[](UInt idx)
{
	REFLEX_ASSERT(idx < 4);

	return Reinterpret<TYPE>(&data)[idx];
}

template <class TYPE> REFLEX_INLINE const TYPE & Reflex::SIMD::TypeV4<TYPE>::operator[](UInt idx) const
{
	REFLEX_ASSERT(idx < 4);

	return Reinterpret<TYPE>(&data)[idx];
}

template <class TYPE> REFLEX_INLINE void Reflex::SIMD::TypeV4<TYPE>::Read(TYPE * unaligned) const
{
	PlatformVectorType<TYPE,4>::StoreUnaligned(unaligned, *this);
}

template <class TYPE> REFLEX_INLINE TYPE Reflex::SIMD::TypeV4<TYPE>::ReadFirst() const
{
	return REFLEX_GETFIRST_F32_X4(data);
}

template <class TYPE> REFLEX_INLINE Reflex::Quad <TYPE> Reflex::SIMD::TypeV4<TYPE>::Read() const
{
	Quad <TYPE> rtn;

	PlatformVectorType<TYPE,4>::StoreUnaligned(&rtn.a, *this);

	return rtn;
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::SIMD::TypeV4<TYPE>::GetData()
{
	return Reinterpret<TYPE>(&data);
}

template <class TYPE> REFLEX_INLINE const TYPE * Reflex::SIMD::TypeV4<TYPE>::GetData() const
{
	return Reinterpret<TYPE>(&data);
}
