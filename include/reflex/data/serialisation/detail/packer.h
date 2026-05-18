#pragma once

#include "traits.h"




//
//impl

REFLEX_NS(Reflex::Data::Detail)

template <class TYPE, bool RAW_PACKABLE = kIsRawPackable<TYPE>>
struct Packer
{
};

template <class TYPE>
struct Packer <TYPE,false>
{
	static_assert(sizeof(TYPE) == 0, "Non raw-packable type");
};

template <>
struct Packer <NullType,true>
{
	static inline Archive::View Pack(const NullType &) { return {}; }

	static inline void Unpack(const Archive::View & archive, NullType & type) {}
};

template <class TYPE>
struct Packer <TYPE,true>
{
	using OutputType = Archive::View;

	static inline Archive::View Pack(const TYPE & type)
	{
		return { Reinterpret<UInt8>(&type), sizeof(TYPE) };
	}

	static inline void Unpack(const Archive::View & archive, TYPE & value)
	{
		REFLEX_ASSERT(archive.size >= sizeof(TYPE));

		value = *Reinterpret<TYPE>(archive.data);
	}
};

template <class TYPE>
struct Packer <ArrayRegion<TYPE>,true>
{
	using OutputType = Archive::View;

	static inline Archive::View Pack(const ArrayRegion <TYPE> & values)
	{
		return { Reinterpret<UInt8>(values.data), UInt(values.size * sizeof(TYPE)) };
	}

	static inline void Unpack(const Archive::View & archive, ArrayView <TYPE> & values)
	{
		REFLEX_ASSERT((archive.size % sizeof(TYPE)) == 0);

		values = { Reinterpret<TYPE>(archive.data), UInt(archive.size / sizeof(TYPE)) };
	}
};

template <class TYPE>
struct Packer <Array<TYPE>, true>
{
	using OutputType = Archive::View;

	static inline Archive::View Pack(const Array <TYPE> & values)
	{
		return { Reinterpret<UInt8>(values.GetData()), UInt(values.GetSize() * sizeof(TYPE)) };
	}

	static inline void Unpack(const Archive::View & archive, Array <TYPE> & values)
	{
		REFLEX_ASSERT((archive.size % sizeof(TYPE)) == 0);

		values = { Reinterpret<TYPE>(archive.data), UInt(archive.size / sizeof(TYPE)) };
	}
};

template <>
struct Packer <ArrayView<UInt8>,true>
{
};

template <>
struct Packer <Array<UInt8>,true> 
{
}; 

template <class TYPE, UInt N>
struct Packer <TYPE[N],true> //: public Packer <ArrayView<TYPE>,true>
{
};

REFLEX_END
