#pragma once

#include "defines.h"
#include "traits.h"
#include "../meta/auxtypes.h"
#include "../string/detail/compare.h"
#include "../string/raw.h"
#include "../detail/iterate.h"
#include "../functions/memory.h"
#include "../functions/cast.h"
#include "../functions/logic.h"




//
//Primary API

namespace Reflex
{

	template <class TYPE> class ArrayRegion;

	template <class TYPE> using ArrayView = ArrayRegion <const TYPE>;

}




//
//ArrayRegion

template <class TYPE>
class Reflex::ArrayRegion
{
public:

	using Type = TYPE;

	using Itr = TYPE *;
	using ReverseItr = TYPE *;
	using ConstItr = TYPE *;
	using ConstReverseItr = TYPE *;

	using InverseConstType = ConditionalType < kIsConst<TYPE>, NonConstT<TYPE>, const TYPE >;

	static constexpr bool kIsNullTerminated = Reflex::IsNullTerminated<NonConstT<TYPE>>::value;



	//lifetime

	constexpr ArrayRegion();

	ArrayRegion(std::initializer_list < NonConstT<TYPE> > && params);

	constexpr ArrayRegion(TYPE * data, UInt size);

	template <UInt SIZE> constexpr ArrayRegion(TYPE(&data)[SIZE]);

	template <UInt SIZE> constexpr ArrayRegion(InverseConstType(&data)[SIZE]);

	constexpr ArrayRegion(const TYPE * nullterminated);		//for string

	ArrayRegion(ConditionalType < kIsConst<TYPE>, const Array <NonConstT<TYPE>>, Array <TYPE> > & array);

	constexpr ArrayRegion(const ArrayRegion & ref) = default;

	consteval ArrayRegion(TYPE * data, UInt size, bool vc_workaround) : data(data), size(size) {}

	ArrayRegion(nullptr_t) = delete;



	//access

	bool Empty() const;


	TYPE & GetFirst() const;

	TYPE & GetLast() const;


	TYPE & operator[](UInt idx) const;



	//operators

	ArrayRegion & operator=(const ArrayRegion & value) = default;


	explicit operator bool() const { return True(this->size); }

	bool operator==(const ArrayRegion & value) const;

	bool operator!=(const ArrayRegion & value) const;

	bool operator<(const ArrayRegion & value) const;


	ArrayRegion <TYPE> * operator&() = delete;		//usage is typically a mistake, use GetAdr if actually needed



	//iterate

	auto begin() const { return this->data; }

	auto end() const { return this->data + this->size; }


	auto rbegin() const { return Detail::ReverseItr(end()); }

	auto rend() const { return Detail::ReverseItr(begin()); }


	
	TYPE * data;
	
	UInt size;

};

REFLEX_SET_TRAIT_TEMPLATED(ArrayRegion, IsBoolCastable);




//
//impl

REFLEX_NS(Reflex::Detail)

template <class ARRAY>
struct ArrayItemTypeImpl
{
};

template <class TYPE>
struct ArrayItemTypeImpl < ArrayRegion <TYPE> >
{
	using Type = TYPE;
};

template <class TYPE>
struct ArrayItemTypeImpl < const ArrayRegion <const TYPE> >
{
	using Type = const TYPE;
};

template <class TYPE, UInt SIZE>
struct ArrayItemTypeImpl < TYPE(&)[SIZE] >
{
	using Type = TYPE;
};

template <class TYPE, UInt SIZE>
struct ArrayItemTypeImpl < const TYPE(&)[SIZE] >
{
	using Type = const TYPE;
};

template <class ARRAY> using ArrayItemType = typename ArrayItemTypeImpl<ARRAY>::Type;

template <class ARRAY> using ArrayViewType = ArrayView < NonConstT < ArrayItemType <ARRAY> > >;

REFLEX_END

REFLEX_NS(Reflex)

template <class TYPE> ArrayRegion <TYPE> * GetAdr(ArrayRegion <TYPE> & region) { return reinterpret_cast<ArrayRegion<TYPE>*>(&region.data); }

template <class TYPE> const ArrayRegion <TYPE> * GetAdr(const ArrayRegion <TYPE> & region) { return reinterpret_cast<const ArrayRegion<TYPE>*>(&region.data); }

REFLEX_END

template <class TYPE> inline constexpr Reflex::ArrayRegion<TYPE>::ArrayRegion()
	: data(nullptr)
	, size(0)
{
}

template <class TYPE> inline constexpr Reflex::ArrayRegion<TYPE>::ArrayRegion(TYPE * data, UInt size)
	: data(data)
	, size(size)
{
}

template <class TYPE> template <Reflex::UInt SIZE> inline constexpr Reflex::ArrayRegion<TYPE>::ArrayRegion(TYPE(&data)[SIZE])
	: data(data)
	, size(SIZE - kIsNullTerminated)
{
	if constexpr (kIsConst<TYPE> && kIsNullTerminated)
	{
		REFLEX_ASSERT(RawStringLength(data) == (SIZE - 1));
	}
}

template <class TYPE> template <Reflex::UInt SIZE> inline constexpr Reflex::ArrayRegion<TYPE>::ArrayRegion(InverseConstType(&data)[SIZE])
	: data(data)
	, size(SIZE - kIsNullTerminated)
{
	static_assert(!(kIsConst<TYPE> && kIsNullTerminated), "passing non-const character buffer to String::View is ambiguous");
}

template <class TYPE> inline constexpr Reflex::ArrayRegion<TYPE>::ArrayRegion(const TYPE * nullterminated)
	: data(nullterminated)
	, size(RawStringLength(nullterminated))
{
	REFLEX_STATIC_ASSERT(kIsNullTerminated);
}

template <class TYPE> REFLEX_INLINE Reflex::ArrayRegion<TYPE>::ArrayRegion(std::initializer_list < NonConstT<TYPE> > && value)
	: data(RemoveConst(value.begin()))
	, size(UInt(value.size()))
{
}

template <class TYPE> REFLEX_INLINE bool Reflex::ArrayRegion<TYPE>::Empty() const
{
	return size == 0;
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::ArrayRegion<TYPE>::GetFirst() const
{
	REFLEX_ASSERT(this->size);

	return *this->data;
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::ArrayRegion<TYPE>::GetLast() const
{
	REFLEX_ASSERT(this->size);

	return this->data[this->size - 1];
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::ArrayRegion<TYPE>::operator[](UInt idx) const
{
	REFLEX_ASSERT(idx < this->size);

	return this->data[idx];
}

template <class TYPE> REFLEX_INLINE bool Reflex::ArrayRegion<TYPE>::operator==(const ArrayRegion & value) const
{
	if (ArrayRegion::size != value.size) return false;

	if constexpr (kIsRawComparable<TYPE>)
	{
		return MemCompare(ArrayRegion::data, value.data, sizeof(TYPE) * ArrayRegion::size);
	}
	else
	{
		UInt idx = ArrayRegion::size;

		while (idx--)
		{
			if (!(ArrayRegion::data[idx] == value.data[idx])) return false;
		}

		return true;
	}
}

template <class TYPE> REFLEX_INLINE bool Reflex::ArrayRegion<TYPE>::operator!=(const ArrayRegion & value) const
{
	return !operator==(value);
}

template <class TYPE> REFLEX_INLINE bool Reflex::ArrayRegion<TYPE>::operator<(const ArrayRegion & value) const
{
	if constexpr (kIsNullTerminated)
	{
		return Detail::StringLessThan<false>(ArrayRegion::data, ArrayRegion::size, value.data, value.size);
	}
	else
	{
		if (ArrayRegion::size == value.size)
		{
			const TYPE * pb = value.data;

			REFLEX_LOOP_PTR(ArrayRegion::data, pa, ArrayRegion::size)
			{
				auto & a = *pa;

				auto & b = *pb++;

				if (a < b) return true;

				if (a > b) return false;
			}

			return false;
		}
		else
		{
			return ArrayRegion::size < value.size;
		}
	}
}
