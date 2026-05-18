#pragma once

#include "../types.h"




//
//Primary API

namespace Reflex::Data
{

	struct DecompressionAlgorithm : public Object
	{
		virtual bool Decompress(const Archive::View & src, Archive & dest) const = 0;
	};

	struct CompressionAlgorithm : public DecompressionAlgorithm
	{
		virtual UInt GetMaxCompressedSize(UInt size) const = 0;

		virtual UInt Compress(const Archive::View & src, UInt8 * dest) const = 0;
	};


	Archive Compress(const CompressionAlgorithm & algorithm, const Archive::View & data);

	Archive Decompress(const DecompressionAlgorithm & algorithm, const Archive::View & data);


	extern const CompressionAlgorithm & kLZ4;

}

