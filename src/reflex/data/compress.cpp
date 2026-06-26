#include "../../../include/reflex/data/functions/compression.h"

#if NULL != 0
REFLEX_STATIC_ASSERT(false);
#endif

#ifndef REFLEX_MINIMAL
#include "lz4.cpp"
#endif




//
//impl

Reflex::Data::Archive Reflex::Data::Compress(const CompressionAlgorithm & algorithm, const Archive::View & data)
{
	Archive out;

	out.SetSize(algorithm.GetMaxCompressedSize(data.size) + 4);

	UInt compressed_size = algorithm.Compress(data, out.GetData() + 4);

	out.SetSize(compressed_size + 4);

	*Reinterpret<UInt>(out.GetData()) = data.size;

	return out;
}

Reflex::Data::Archive Reflex::Data::Decompress(const DecompressionAlgorithm & algorithm, const Archive::View & data)
{
	auto pair = Splice(data, 4);

	Archive out;

	out.SetSize(*Reinterpret<UInt>(pair.a.data));

	algorithm.Decompress(pair.b, out);

	return out;
}
