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

	using DataType = typename PlatformVectorType<TYPE,4>::Type;

	using SingleType = typename PlatformVectorType<TYPE,4>::SingleType;



	//lifetime

	TypeV4();

	TypeV4(SingleType value);

	TypeV4(SingleType a, SingleType b, SingleType c, SingleType d);

	TypeV4(const DataType & data);

	TypeV4(const TypeV4 & data);



	//cast

	operator DataType&();

	operator const DataType&() const;



	//set

	void Set(SingleType value);

	void Set(SingleType a, SingleType b, SingleType c, SingleType d);



	//assign

	TypeV4 & operator=(const TypeV4 & value);

	TypeV4 & operator=(const DataType & data);

	TypeV4 & operator=(SingleType value);



	//math

	TypeV4 & operator+=(const TypeV4 & value);

	TypeV4 & operator-=(const TypeV4 & value);

	TypeV4 & operator*=(const TypeV4 & value);

	TypeV4 & operator/=(const TypeV4 & value);



	//access

	SingleType & operator[](UInt idx);

	const SingleType & operator[](UInt idx) const;

	SingleType ReadFirst() const;

	Quad <SingleType> Read() const;

	void Read(SingleType * unaligned) const;


	SingleType * GetData();

	const SingleType * GetData() const;



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

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(SingleType value)
	: data(PlatformVectorType<TYPE,4>::Set1(value))
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(SingleType a, SingleType b, SingleType c, SingleType d)
	: data(PlatformVectorType<TYPE,4>::Set(d, c, b, a))
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(const DataType & data)
	: data(data)
{
	AssertAlignment(this);
}

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4<TYPE>::TypeV4(const TypeV4 & data)
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

template <class TYPE> REFLEX_INLINE void Reflex::SIMD::TypeV4<TYPE>::Set(SingleType value)
{
	data = PlatformVectorType<TYPE,4>::Set1(value);
}

template <class TYPE> REFLEX_INLINE void Reflex::SIMD::TypeV4<TYPE>::Set(SingleType a, SingleType b, SingleType c, SingleType d)
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

template <class TYPE> REFLEX_INLINE Reflex::SIMD::TypeV4 <TYPE> & Reflex::SIMD::TypeV4<TYPE>::operator=(SingleType value)
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

template <class TYPE> REFLEX_INLINE typename Reflex::SIMD::TypeV4<TYPE>::SingleType & Reflex::SIMD::TypeV4<TYPE>::operator[](UInt idx)
{
	REFLEX_ASSERT(idx < 4);

	return Reinterpret<SingleType>(&data)[idx];
}

template <class TYPE> REFLEX_INLINE const typename Reflex::SIMD::TypeV4<TYPE>::SingleType & Reflex::SIMD::TypeV4<TYPE>::operator[](UInt idx) const
{
	REFLEX_ASSERT(idx < 4);

	return Reinterpret<SingleType>(&data)[idx];
}

template <class TYPE> REFLEX_INLINE void Reflex::SIMD::TypeV4<TYPE>::Read(SingleType * unaligned) const
{
	PlatformVectorType<TYPE,4>::StoreUnaligned(unaligned, *this);
}

template <class TYPE> REFLEX_INLINE typename Reflex::SIMD::TypeV4<TYPE>::SingleType Reflex::SIMD::TypeV4<TYPE>::ReadFirst() const
{
	return REFLEX_GETFIRST_F32_X4(data);
}

template <class TYPE> REFLEX_INLINE Reflex::Quad <typename Reflex::SIMD::TypeV4<TYPE>::SingleType> Reflex::SIMD::TypeV4<TYPE>::Read() const
{
	Quad <SingleType> rtn;

	PlatformVectorType<TYPE,4>::StoreUnaligned(&rtn.a, *this);

	return rtn;
}

template <class TYPE> REFLEX_INLINE typename Reflex::SIMD::TypeV4<TYPE>::SingleType * Reflex::SIMD::TypeV4<TYPE>::GetData()
{
	return Reinterpret<SingleType>(&data);
}

template <class TYPE> REFLEX_INLINE const typename Reflex::SIMD::TypeV4<TYPE>::SingleType * Reflex::SIMD::TypeV4<TYPE>::GetData() const
{
	return Reinterpret<SingleType>(&data);
}
