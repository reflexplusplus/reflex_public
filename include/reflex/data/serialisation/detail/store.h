#pragma once

#include "traits.h"
#include "../../functions/string.h"




//
//declarations

REFLEX_NS(Reflex::Data::Detail)

template <class TYPE> void StoreImpl(Archive & stream, const TYPE & value);

REFLEX_END





//
//impl

REFLEX_STATIC_ASSERT(sizeof(char16_t) == 2);

REFLEX_NS(Reflex::Data::Detail)

inline void StoreImpl(Archive & stream, const NullType &) {}
void StoreImpl(Archive & stream, bool value);
void StoreImpl(Archive & stream, WChar value);
template <class TYPE> void StoreImpl(Archive & stream, TRef <TYPE> ref);
template <class TYPE> void StoreImpl(Archive & stream, const ObjectOf <TYPE> & object);
template <class ... VARGS> void StoreImpl(Archive & stream, const Tuple <VARGS...> & value);
template <class TYPE, Reflex::UInt SIZE> void StoreImpl(Archive & stream, const TYPE(&value)[SIZE]);
template <class TYPE> void StoreImpl(Archive & stream, const ArrayView <TYPE> & value);
template <class TYPE> void StoreImpl(Archive & stream, const Array <TYPE> & value);
template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> void StoreImpl(Archive & stream, const Sequence<KEY, VALUE, COMPARE, CONTIGUOUS> & value);
template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> void StoreImpl(Archive & stream, const Map<KEY, VALUE, COMPARE, CONTIGUOUS> & value);

template <class TYPE> struct ArrayEncoder
{
	static void StoreImpl(Archive & stream, const TYPE * data, UInt size);

	static void StoreImpl(Archive & stream, const ArrayView <TYPE> & data);
};

template <> struct [[deprecated]] ArrayEncoder <WChar>
{
	static void StoreImpl(Archive & stream, const WChar * data, UInt size);

	static void StoreImpl(Archive & stream, const WString::View & data);
};

template <UInt IDX, class TUPLE> void SerializeTuple(Archive & stream, const TUPLE & tuple)
{
	if constexpr (IDX < kTupleSize<TUPLE>)
	{
		Detail::StoreImpl(stream, TupleElement<const TUPLE,IDX>::Get(tuple));

		SerializeTuple<IDX + 1>(stream, tuple);
	}
}

template <class TYPE> REFLEX_INLINE void WriteRaw(Archive & stream, const TYPE & value)
{
	REFLEX_STATIC_ASSERT(IsRawCopyable<TYPE>::value);
	REFLEX_STATIC_ASSERT(IsRawPackable<TYPE>::value);

	stream.Append({ Reinterpret<UInt8>(&value), sizeof(TYPE) });
}

template <class TYPE> REFLEX_INLINE void ArrayEncoder<TYPE>::StoreImpl(Archive & stream, const TYPE * data, UInt size)
{
	if constexpr (IsRawCopyable<TYPE>::value)
	{
		stream.Append(Archive::View(Reinterpret<UInt8>(data), size * sizeof(TYPE)));
	}
	else
	{
		REFLEX_LOOP_PTR(data, ptr, size) Detail::StoreImpl(stream, *ptr);
	}
}

template <class TYPE> REFLEX_INLINE void ArrayEncoder<TYPE>::StoreImpl(Archive & stream, const ArrayView <TYPE> & value)
{
	WriteRaw(stream, typename IndexType<Array<TYPE>>::Type(value.size));

	StoreImpl(stream, value.data, value.size);
}

REFLEX_INLINE void ArrayEncoder<WChar>::StoreImpl(Archive & stream, const WString::View & value)
{
	WriteRaw(stream, typename IndexType<Array<WChar>>::Type(value.size));

	EncodeUCS2(stream, value);
}

REFLEX_INLINE void StoreImpl(Archive & stream, bool value)
{
	stream.Push(UInt8(value));
}

REFLEX_INLINE void StoreImpl(Archive & stream, WChar value)
{
	WriteRaw(stream, UInt16(value));
}

template <class TYPE> REFLEX_INLINE void StoreImpl(Archive & stream, TRef <TYPE> ref)
{
	Detail::StoreImpl(stream, *ref);
}

template <class TYPE> REFLEX_INLINE void StoreImpl(Archive & stream, const ObjectOf <TYPE> & object)
{
	Detail::StoreImpl(stream, object.value);
}

template <class ... VARGS> REFLEX_INLINE void StoreImpl(Archive & stream, const Tuple <VARGS...> & value)
{
	using Type = Tuple <VARGS...>;

	if constexpr (IsRawCopyable<Type>::value)
	{
		stream.Append(Archive::View(Reinterpret<UInt8>(&value), sizeof(Type)));
	}
	else
	{
		SerializeTuple<0>(stream, value);
	}
}

template <class TYPE, Reflex::UInt SIZE> REFLEX_INLINE void StoreImpl(Archive & stream, const TYPE(&value)[SIZE])
{
	ArrayEncoder<TYPE>::StoreImpl(stream, value, SIZE);
}

template <class TYPE> inline void StoreImpl(Archive & stream, const ArrayView <TYPE> & value)
{
	ArrayEncoder<TYPE>::StoreImpl(stream, value);
}

template <class TYPE> REFLEX_INLINE void StoreImpl(Archive & stream, const Array <TYPE> & value)
{
	Detail::StoreImpl(stream, ToView(value));
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void StoreImpl(Archive & stream, const Sequence <KEY,VALUE,COMPARE,CONTIGUOUS> & value)
{
	WriteRaw(stream, typename IndexType<Sequence<KEY,VALUE,COMPARE,CONTIGUOUS>>::Type(value.GetSize()));

	for (auto & i : value)
	{
		Detail::StoreImpl(stream, i.key);
	
		Detail::StoreImpl(stream, i.value);
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void StoreImpl(Archive & stream, const Map <KEY, VALUE, COMPARE, CONTIGUOUS> & value)
{
	StoreImpl(stream, Reinterpret< Sequence <KEY, VALUE, COMPARE, CONTIGUOUS> >(value));
}

REFLEX_END

template <class TYPE> REFLEX_INLINE void Reflex::Data::Detail::StoreImpl(Archive & stream, const TYPE & data)
{
	if constexpr (kIsRawCopyable<TYPE>)
	{
		WriteRaw(stream, data);
	}
	else if constexpr (kIsStreamable<TYPE>)
	{
		data.Serialize(stream);
	}
	else
	{
		REFLEX_STATIC_ASSERT(!kSizeOf<TYPE>);	//conditional static warning
	}
}

