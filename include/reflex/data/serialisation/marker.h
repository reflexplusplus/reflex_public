#pragma once

#include "store.h"




namespace Reflex::Data
{

	template <class TYPE> class Marker;


	Archive::View ReadChunk(Archive::View & stream);

}




//
//Data::Marker

template <class TYPE>
class Reflex::Data::Marker
{
public:

	//lifetime

	Marker(Archive & archive);



	//access

	void Set(const TYPE & value);



	//info

	UInt GetPosition() const { return m_position; }

	UInt GetDelta() const { return archive.GetSize() - (m_position + sizeof(TYPE)); }



private:

	Archive & archive;

	UInt m_position;

};




//
//impl

REFLEX_NS(Reflex::Data)

template <class HEADER = NullType> class LegacyChunkWriter; //LEGACY

template <class TYPE> Pair <TYPE, Archive::View> inline ReadLegacyChunk(Archive::View & stream);	//LEGACY

REFLEX_END

template <class HEADER>
class Reflex::Data::LegacyChunkWriter
{
public:

	//lifetime

	LegacyChunkWriter(Archive & stream);

	~LegacyChunkWriter();



	//header

	void SetHeader(const HEADER & header);



private:

	Archive & stream;

	UInt m_sizepos;

};

template <class TYPE> REFLEX_INLINE Reflex::Data::Marker<TYPE>::Marker(Archive & archive)
	: archive(archive),
	m_position(archive.GetSize())
{
	REFLEX_STATIC_ASSERT(IsRawCopyable<TYPE>::value);

	UInt8 bytes[sizeof(TYPE)] = { 0 };

	archive.Append({ bytes, sizeof(TYPE) });
}

template <class TYPE> REFLEX_INLINE void Reflex::Data::Marker<TYPE>::Set(const TYPE & value)
{
	*Reinterpret<TYPE>(archive.GetData() + m_position) = value;
}

template <class HEADER> REFLEX_INLINE Reflex::Data::LegacyChunkWriter<HEADER>::LegacyChunkWriter(Archive & stream)
	: stream(stream),
	m_sizepos(stream.GetSize())
{
	REFLEX_STATIC_ASSERT(kSizeOf<HEADER> == 0 || IsRawCopyable<HEADER>::value);

	UInt32 size = 0;	//reserve size

	stream.Append({ Reinterpret<UInt8>(&size), sizeof(UInt32) });

	if constexpr (kSizeOf<HEADER>)
	{
		HEADER header = {};

		stream.Append({ Reinterpret<UInt8>(&header), kSizeOf<HEADER>});
	}
}

template <class HEADER> REFLEX_INLINE Reflex::Data::LegacyChunkWriter<HEADER>::~LegacyChunkWriter()
{
	UInt32 * psize = Reinterpret<UInt32>(stream.GetData() + m_sizepos);

	*psize = (stream.GetSize() - m_sizepos) - (sizeof(UInt32) + kSizeOf<HEADER>);
}

template <class HEADER> REFLEX_INLINE void Reflex::Data::LegacyChunkWriter<HEADER>::SetHeader(const HEADER & header)
{
	auto * pheader = Reinterpret<HEADER>(stream.GetData() + m_sizepos + sizeof(UInt32));

	*pheader = header;
}

template <class TYPE> Reflex::Pair <TYPE,Reflex::Data::Archive::View> inline Reflex::Data::ReadLegacyChunk(Data::Archive::View & stream)
{
	REFLEX_STATIC_ASSERT(IsRawCopyable<TYPE>::value);

	auto header = Deserialize<Tuple<UInt32, TYPE>>(stream);

	return { header.b, Inc(stream, header.a) };
}

inline Reflex::Data::Archive::View Reflex::Data::ReadChunk(Data::Archive::View & stream)
{
	return Inc(stream, Deserialize<UInt32>(stream));
}
