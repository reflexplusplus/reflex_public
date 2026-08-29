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

	using ItemType = NonConstT <TYPE>;

	static constexpr bool kIsNullTerminated = Reflex::IsNullTerminated<ItemType>::value;



	//lifetime

	constexpr ArrayRegion(const ArrayRegion & ref) = default;

	constexpr ArrayRegion();

	ArrayRegion(std::initializer_list <ItemType> && params);

	constexpr ArrayRegion(TYPE * data, UInt size);

	template <UInt SIZE> constexpr ArrayRegion(TYPE(&data)[SIZE]);

	template <UInt SIZE> constexpr ArrayRegion(ItemType(&data)[SIZE]) requires(kIsConst<TYPE>);

	constexpr ArrayRegion(const ItemType * nullterminated) requires(kIsConst<TYPE> && kIsNullTerminated);		//for string

	ArrayRegion(ConditionalType < kIsConst<TYPE>, const Array <ItemType>, Array <TYPE> > & array);

	consteval ArrayRegion(TYPE * data, UInt size, bool vc_workaround) : data(data), size(size) {}


	ArrayRegion(ItemType * nullterminated) requires(kIsConst<TYPE> && kIsNullTerminated) = delete;

	ArrayRegion(nullptr_t) = delete;



	//access

	bool Empty() const;


	TYPE & GetFirst() const;

	TYPE & GetLast() const;


	TYPE & operator[](UInt idx) const;



	//operators

	ArrayRegion & operator=(const ArrayRegion & value) = default;

	ArrayRegion & operator=(std::initializer_list <ItemType> && params);

	template <UInt SIZE> ArrayRegion & operator=(TYPE(&data)[SIZE]);

	template <UInt SIZE> ArrayRegion & operator=(ItemType(&data)[SIZE]) requires(kIsConst<TYPE>);

	ArrayRegion & operator=(const ItemType * nullterminated) requires(kIsConst<TYPE> && kIsNullTerminated);


	ArrayRegion & operator=(Array <ItemType> && value) = delete;		//typically unsafe

	ArrayRegion & operator=(ItemType * nullterminated) requires(kIsConst<TYPE> && kIsNullTerminated) = delete;

	ArrayRegion & operator=(nullptr_t) = delete;


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

template <class TYPE> template <Reflex::UInt SIZE> inline constexpr Reflex::ArrayRegion<TYPE>::ArrayRegion(ItemType(&data)[SIZE]) requires(kIsConst<TYPE>)
	: data(data)
	, size(SIZE - kIsNullTerminated)
{
	static_assert(!(kIsConst<TYPE> && kIsNullTerminated), "passing non-const character buffer to String::View is ambiguous");
}

template <class TYPE> inline constexpr Reflex::ArrayRegion<TYPE>::ArrayRegion(const ItemType * nullterminated) requires(kIsConst<TYPE> && kIsNullTerminated)
	: data(nullterminated)
	, size(RawStringLength(nullterminated))
{
}

template <class TYPE> REFLEX_INLINE Reflex::ArrayRegion<TYPE>::ArrayRegion(std::initializer_list <ItemType> && value)
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

template <class TYPE> inline Reflex::ArrayRegion <TYPE> & Reflex::ArrayRegion<TYPE>::operator=(std::initializer_list <ItemType> && params)
{
	data = RemoveConst(params.begin());
	size = UInt(params.size());

	return *this;
}

template <class TYPE> template <Reflex::UInt SIZE> inline Reflex::ArrayRegion <TYPE> & Reflex::ArrayRegion<TYPE>::operator=(TYPE(&data)[SIZE])
{
	ArrayRegion::data = data;
	ArrayRegion::size = SIZE - kIsNullTerminated;

	if constexpr (kIsConst<TYPE> && kIsNullTerminated)
	{
		REFLEX_ASSERT(RawStringLength(data) == (SIZE - 1));
	}

	return *this;
}

template <class TYPE> template <Reflex::UInt SIZE> inline Reflex::ArrayRegion <TYPE> & Reflex::ArrayRegion<TYPE>::operator=(ItemType(&data)[SIZE]) requires(kIsConst<TYPE>)
{
	static_assert(!kIsNullTerminated, "assigning non-const character buffer to String::View is ambiguous");

	ArrayRegion::data = data;
	ArrayRegion::size = SIZE - kIsNullTerminated;

	return *this;
}

template <class TYPE> inline Reflex::ArrayRegion <TYPE> & Reflex::ArrayRegion<TYPE>::operator=(const ItemType * nullterminated) requires(kIsConst<TYPE> && kIsNullTerminated)
{
	data = nullterminated;
	size = RawStringLength(nullterminated);

	return *this;
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
