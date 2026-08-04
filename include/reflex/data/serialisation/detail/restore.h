#pragma once

#include "traits.h"
#include "../../functions/string.h"




//
//Detail

REFLEX_NS(Reflex::Data::Detail)

template <class TYPE> void RestoreImpl(Archive::View & stream, TYPE & data);

REFLEX_END




//
//impl

REFLEX_NS(Reflex::Data::Detail)

inline void RestoreImpl(Archive::View & stream, NullType &) {}
void RestoreImpl(Archive::View & stream, bool & value);
void RestoreImpl(Archive::View & stream, WChar & value);
template <class TYPE> void RestoreImpl(Archive::View & stream, TRef <TYPE> ref);
template <class TYPE> void RestoreImpl(Archive::View & stream, ObjectOf <TYPE> & object);
template <class ... VARGS> void RestoreImpl(Archive::View & stream, Tuple <VARGS...> & value);
template <class TYPE, UInt SIZE> void RestoreImpl(Archive::View & stream, TYPE(&data)[SIZE]);
template <class TYPE> void RestoreImpl(Archive::View & stream, Array <TYPE> & data);
template <class TYPE> void RestoreImpl(Archive::View & stream, ArrayView <TYPE> & data);
template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> void RestoreImpl(Archive::View & stream, Sequence<KEY, VALUE, COMPARE, CONTIGUOUS> & data);
template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> void RestoreImpl(Archive::View & stream, Map<KEY, VALUE, COMPARE, CONTIGUOUS> & value);

template <class TYPE> struct ArrayDecoder
{
	static void RestoreImpl(Archive::View & stream, TYPE * data, UInt size);

	static void RestoreImpl(Archive::View & stream, Array <TYPE> & data);

	static void RestoreImpl(Archive::View & stream, ArrayView <TYPE> & data);
};

template <> struct ArrayDecoder <WChar>
{
	static void RestoreImpl(Archive::View & stream, WChar * data, UInt size);

	static void RestoreImpl(Archive::View & stream, WString & data);

	static void RestoreImpl(Archive::View & stream, WString::View & data);		//NOT DEFINED, WIN/MAC INCOMPATIBLE
};

template <class TYPE> REFLEX_INLINE void ArrayDecoder<TYPE>::RestoreImpl(Archive::View & stream, TYPE * data, UInt size)
{
	if constexpr (IsRawCopyable<TYPE>::value)
	{
		UInt nbytes = size * sizeof(TYPE);

		MemCopy(stream.data, data, nbytes);

		stream = Nudge(stream, nbytes);
	}
	else
	{
		REFLEX_LOOP_PTR(data, ptr, size) Detail::RestoreImpl(stream, *ptr);
	}
}

template <class TYPE> REFLEX_INLINE void ArrayDecoder<TYPE>::RestoreImpl(Archive::View & stream, Array <TYPE> & data)
{
	typename IndexType<Array<TYPE>>::Type size;

	Detail::RestoreImpl(stream, size);

	data.SetSize(size);

	RestoreImpl(stream, data.GetData(), size);
}

template <class TYPE> REFLEX_INLINE void ArrayDecoder<TYPE>::RestoreImpl(Archive::View & stream, ArrayView <TYPE> & data)
{
	REFLEX_STATIC_ASSERT(IsRawCopyable<TYPE>::value);

	typename IndexType<Array<TYPE>>::Type size;

	Detail::RestoreImpl(stream, size);

	data = { Reinterpret<TYPE>(stream.data), size };

	stream = Nudge(stream, sizeof(TYPE) * size);
}

REFLEX_INLINE void ArrayDecoder<WChar>::RestoreImpl(Archive::View & stream, WString & data)
{
	typename IndexType<Array<WChar>>::Type size;

	Detail::RestoreImpl(stream, size);

	UInt nbytes = size * 2;

	data.Clear();

	DecodeUCS2(data, { stream.data, nbytes });

	stream = Nudge(stream, nbytes);
}

template <UInt IDX, class TUPLE> void RestoreTuple(Archive::View & stream, TUPLE & tuple)
{
	if constexpr (IDX < kTupleSize<TUPLE>)
	{
		Detail::RestoreImpl(stream, TupleElement<TUPLE,IDX>::Get(tuple));

		RestoreTuple<IDX + 1>(stream, tuple);
	}
}

template <class TYPE, UInt SIZE> REFLEX_INLINE void RawCopyChunked(const void * src, void * dst)
{
	auto pdst = Reinterpret<TYPE>(dst);

	REFLEX_LOOP_PTR(Reinterpret<TYPE>(src), psrc, (SIZE / sizeof(TYPE)))
	{
		*pdst++ = *psrc;
	}
}

template <UInt SIZE> REFLEX_INLINE void RawCopy(const void * src, void * dst)
{
	switch (SIZE)
	{
	case 1:
		*Reinterpret<UInt8>(dst) = *Reinterpret<UInt8>(src);
		break;

	case 2:
		*Reinterpret<UInt16>(dst) = *Reinterpret<UInt16>(src);
		break;

	case 4:
		*Reinterpret<UInt32>(dst) = *Reinterpret<UInt32>(src);
		break;

	case 8:
		*Reinterpret<UInt64>(dst) = *Reinterpret<UInt64>(src);
		break;

	case 16:
		*Reinterpret<UInt64>(dst) = *Reinterpret<UInt64>(src);
		Reinterpret<UInt64>(dst)[1] = Reinterpret<UInt64>(src)[1];
		break;

	default:
		if constexpr (((SIZE / 8) * 8) == SIZE)
		{
			RawCopyChunked<UInt64,SIZE>(src, dst);
		}
		else if constexpr (((SIZE / 4) * 4) == SIZE)
		{
			RawCopyChunked<UInt32,SIZE>(src, dst);
		}
		else
		{
			MemCopy(src, dst, SIZE);
		}
		break;
	}
}

template <class TYPE> REFLEX_INLINE void ReadRaw(Archive::View & stream, TYPE & value)
{
	REFLEX_STATIC_ASSERT(IsRawCopyable<TYPE>::value);

	RawCopy<sizeof(TYPE)>(stream.data, &value);

	Inc(stream, sizeof(TYPE));
}

REFLEX_INLINE void RestoreImpl(Archive::View & stream, bool & value)
{
	UInt8 temp;

	RestoreImpl(stream, temp);

	value = True(temp);
}

REFLEX_INLINE void RestoreImpl(Archive::View & stream, WChar & value)
{
	UInt16 temp;

	RestoreImpl(stream, temp);

	value = WChar(temp);
}

template <class TYPE> REFLEX_INLINE void RestoreImpl(Archive::View & stream, TRef <TYPE> ref)
{
	RestoreImpl(stream, *ref);
}

template <class TYPE> REFLEX_INLINE void RestoreImpl(Archive::View & stream, ObjectOf <TYPE> & object)
{
	RestoreImpl(stream, object.value);
}

template <class ... VARGS> REFLEX_INLINE void RestoreImpl(Archive::View & stream, Tuple <VARGS...> & value)
{
	using Type = Tuple <VARGS...>;

	if constexpr (IsRawCopyable<Type>::value)
	{
		RawCopy<sizeof(Type)>(stream.data, &value);

		stream = Nudge(stream, sizeof(Type));
	}
	else
	{
		RestoreTuple<0>(stream, value);
	}
}

template <class TYPE, UInt SIZE> REFLEX_INLINE void RestoreImpl(Archive::View & stream, TYPE(&data)[SIZE])
{
	ArrayDecoder<TYPE>::RestoreImpl(stream, data, SIZE);
}

template <class TYPE> inline void RestoreImpl(Archive::View & stream, Array <TYPE> & data)
{
	ArrayDecoder<TYPE>::RestoreImpl(stream, data);
}

template <class TYPE> inline void RestoreImpl(Archive::View & stream, ArrayView <TYPE> & data)
{
	ArrayDecoder<TYPE>::RestoreImpl(stream, data);
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void RestoreImpl(Archive::View & stream, Sequence <KEY, VALUE, COMPARE, CONTIGUOUS> & data)
{
	data.Clear();

	typename IndexType<Sequence<KEY, VALUE, COMPARE, CONTIGUOUS>>::Type size;

	Detail::RestoreImpl(stream, size);

	data.Allocate(size);

	KEY key;

	VALUE value;

	while (size--)
	{
		Detail::RestoreImpl(stream, key);

		Detail::RestoreImpl(stream, value);

		data.Insert(key, std::move(value));
	}
}

template <class KEY, class VALUE, class COMPARE, bool CONTIGUOUS> inline void RestoreImpl(Archive::View & stream, Map <KEY, VALUE, COMPARE, CONTIGUOUS> & value)
{
	RestoreImpl(stream, Reinterpret< Sequence <KEY, VALUE, COMPARE, CONTIGUOUS> >(value));
}

REFLEX_END

template <class TYPE> REFLEX_INLINE void Reflex::Data::Detail::RestoreImpl(Archive::View & stream, TYPE & data)
{
	if constexpr (kIsRawCopyable<TYPE>)
	{
		ReadRaw(stream, data);
	}
	else if constexpr (kIsStreamable<TYPE>)
	{
		data.Deserialize(stream);
	}
	else
	{
		REFLEX_STATIC_ASSERT(!kSizeOf<TYPE>);	//conditional static warning
	}
}

