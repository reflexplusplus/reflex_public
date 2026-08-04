#include "../../../include/reflex/data/functions/compression.h"

#include "ext/lz4-dev/lib/lz4.h"
#include "ext/lz4-dev/lib/lz4.c"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct LZ4 : public CompressionAlgorithm
{
	UInt GetMaxCompressedSize(UInt size) const override
	{
		return LZ4_compressBound(size);
	}

	UInt Compress(const Archive::View & src, UInt8 * dest) const override
	{
		return LZ4_compress_default(Reinterpret<char>(src.data), Reinterpret<char>(dest), src.size, GetMaxCompressedSize(src.size));
	}

	bool Decompress(const Archive::View & src, Archive & dest) const override
	{
		return LZ4_decompress_safe(Reinterpret<char>(src.data), Reinterpret<char>(dest.GetData()), src.size, dest.GetSize());
	}
}

g_lz4;

REFLEX_END_INTERNAL

const Reflex::ConstTRef <Reflex::Data::CompressionAlgorithm> Reflex::Data::kLZ4 = Reflex::Data::g_lz4;
