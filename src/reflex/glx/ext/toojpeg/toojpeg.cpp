#include "toojpeg.h"

namespace TooJPEG { namespace {

typedef signed short Int16;
typedef signed int Int32;

const UInt8 kHeaderJfif[2 + 2 + 16] =
{
	0xFF,0xD8,				 // SOI marker (start of image)
	0xFF,0xE0,				 // JFIF APP0 tag
	0,16,							// length: 16 bytes (14 bytes payload + 2 bytes for this length field)
	'J','F','I','F',0, // JFIF identifier, zero-terminated
	1,1,							 // JFIF version 1.1
	0,								 // no density units specified
	0,1,0,1,					 // density: 1 pixel "per pixel" horizontally and vertically
	0,0
};						 // no thumbnail (size 0 x 0)

const UInt8 kDefaultQuantLuminance[64] =
{
	16, 11, 10, 16, 24, 40, 51, 61, // there are a few experts proposing slightly more efficient values,
	12, 12, 14, 19, 26, 58, 60, 55, // e.g. https://www.imagemagick.org/discourse-server/viewtopic.php?t=20333
	14, 13, 16, 24, 40, 57, 69, 56, // btw: Google's Guetzli project optimizes the quantization tables per image
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68,109,103, 77,
	24, 35, 55, 64, 81,104,113, 92,
	49, 64, 78, 87,103,121,120,101,
	72, 92, 95, 98,112,100,103, 99
};

const UInt8 kDefaultQuantChrominance[64] =
{
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

const UInt8 kZigZagInv[64] =
{
	0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};

const UInt8 kDcLuminanceCodesPerBitsize[16] = { 0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };	 // sum = 12

const UInt8 kDcLuminanceValues[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };				 // => 12 codes

const UInt8 kAcLuminanceCodesPerBitsize[16] = { 0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,125 }; // sum = 162

const UInt8 kAcLuminanceValues[162] =
{
	0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08, // 16*10+2 symbols because
	0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28, // upper 4 bits can be 0..F
	0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59, // while lower 4 bits can be 1..A
	0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89, // plus two special codes 0x00 and 0xF0
	0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6, // order of these symbols was determined empirically by JPEG committee
	0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
	0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA
};

const UInt8 kDcChrominanceCodesPerBitsize[16] = { 0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };	 // sum = 12

const UInt8 kDcChrominanceValues[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 };				 // => 12 codes (identical to kDcLuminanceValues)

const UInt8 kAcChrominanceCodesPerBitsize[16] = { 0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,119 }; // sum = 162

const UInt8 kAcChrominanceValues[162] =																				// => 162 codes
{
	0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91, // same number of symbol, just different order
	0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26, // (which is more efficient for AC coding)
	0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,
	0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,
	0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,
	0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
	0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA
};

const Int16 kCodeWordLimit = 2048; // +/-2^11, maximum value after DCT

const UInt8 kSpectral[3] = { 0, 63, 0 }; // spectral selection: must be from 0 to 63; successive approximation must be 0

const float kAanScaleFactors[8] = { 1, 1.387039845f, 1.306562965f, 1.175875602f, 1, 0.785694958f, 0.541196100f, 0.275899379f };

struct BitCode
{
	BitCode() = default; // undefined state, must be initialized at a later time

	BitCode(UInt16 code_, UInt8 numBits_)
		: code(code_),
		numBits(numBits_)
	{
	}

	UInt16 code;		 // JPEG's Huffman codes are limited to 16 bits
	UInt8 numBits;		// number of valid bits
};

struct BitWriter
{
	BitWriter(void * client, TooJPEG::Output output)
		: m_client(client),
		m_output(output)
	{
	}

	void Output(const UInt8 * bytes, UInt32 n)
	{
		m_output(m_client, bytes, n);
	}

	struct BitBuffer
	{
		Int32 data = 0;	//only at most 24 bits are used
		UInt8 numBits = 0;	//number of valid bits (the right-most bits)
	} buffer;

	BitWriter & operator<<(const BitCode & data)
	{
		buffer.numBits += data.numBits;
		buffer.data	<<= data.numBits;
		buffer.data |= data.code;

		while (buffer.numBits >= 8)
		{
			buffer.numBits -= 8;

			UInt8 pair[2] = { UInt8(buffer.data >> buffer.numBits), 0 };

			Output(pair, pair[0] == 0xFF ? 2 : 1);

			// note: I don't clear those written bits, therefore buffer.bits may contain garbage in the high bits
			//			 if you really want to "clean up" (e.g. for debugging purposes) then uncomment the following line
			//buffer.bits &= (1 << buffer.numBits) - 1;
		}

		return *this;
	}

	void Flush()
	{
		*this << BitCode(0x7F, 7); // I should set buffer.numBits = 0 but since there are no single bits written after Flush() I can safely ignore it
	}

	BitWriter & operator<<(UInt8 oneByte)
	{
		Output(&oneByte, 1);

		return *this;
	}

	template <Int32 SIZE> BitWriter & operator<<(const UInt8(&bytes)[SIZE])
	{
		Output(bytes, SIZE);

		return *this;
	}

	void AddMarker(UInt8 id, UInt16 length)
	{
		UInt8 marker[4] = { 0xFF, id, UInt8(length >> 8), UInt8(length & 0xFF) };

		Output(marker, 4);
	}



private:

	void * m_client;

	TooJPEG::Output m_output;
};

template <typename Number> Number minimum(Number value, Number maximum)
{
	return value <= maximum ? value : maximum;
}

template <typename Number> Number clamp(Number value, Number minValue, Number maxValue)
{
	if (value <= minValue) return minValue; // never smaller than the minimum

	if (value >= maxValue) return maxValue; // never bigger	than the maximum

	return value;													 // value was inside interval, keep it
}

float rgb2y (float r, float g, float b) { return +0.299f * r +0.587f * g +0.114f * b; }
float rgb2cb(float r, float g, float b) { return -0.16874f * r -0.33126f * g +0.5f * b; }
float rgb2cr(float r, float g, float b) { return +0.5f * r -0.41869f * g -0.08131f * b; }

void DCT(float block[64], UInt8 stride) // stride must be 1 (=horizontal) or 8 (=vertical)
{
	const auto SqrtHalfSqrt = 1.306562965f; //		sqrt((2 + sqrt(2)) / 2) = cos(pi * 1 / 8) * sqrt(2)
	const auto InvSqrt = 0.707106781f; // 1 / sqrt(2)								= cos(pi * 2 / 8)
	const auto HalfSqrtSqrt = 0.382683432f; //		 sqrt(2 - sqrt(2)) / 2	= cos(pi * 3 / 8)
	const auto InvSqrtSqrt	= 0.541196100f; // 1 / sqrt(2 - sqrt(2))			= cos(pi * 3 / 8) * sqrt(2)

	// modify in-place
	auto & block0 = block[0];
	auto & block1 = block[1 * stride];
	auto & block2 = block[2 * stride];
	auto & block3 = block[3 * stride];
	auto & block4 = block[4 * stride];
	auto & block5 = block[5 * stride];
	auto & block6 = block[6 * stride];
	auto & block7 = block[7 * stride];

	// based on https://dev.w3.org/Amaya/libjpeg/jfdctflt.c , the original variable names can be found in my comments
	auto add07 = block0 + block7; auto sub07 = block0 - block7; // tmp0, tmp7
	auto add16 = block1 + block6; auto sub16 = block1 - block6; // tmp1, tmp6
	auto add25 = block2 + block5; auto sub25 = block2 - block5; // tmp2, tmp5
	auto add34 = block3 + block4; auto sub34 = block3 - block4; // tmp3, tmp4

	auto add0347 = add07 + add34; auto sub07_34 = add07 - add34; // tmp10, tmp13 ("even part" / "phase 2")
	auto add1256 = add16 + add25; auto sub16_25 = add16 - add25; // tmp11, tmp12

	block0 = add0347 + add1256; block4 = add0347 - add1256; // "phase 3"

	auto z1 = (sub16_25 + sub07_34) * InvSqrt; // all temporary z-variables kept their original names
	block2 = sub07_34 + z1; block6 = sub07_34 - z1; // "phase 5"

	auto sub23_45 = sub25 + sub34; // tmp10 ("odd part" / "phase 2")
	auto sub12_56 = sub16 + sub25; // tmp11
	auto sub01_67 = sub16 + sub07; // tmp12

	auto z5 = (sub23_45 - sub01_67) * HalfSqrtSqrt;
	auto z2 = sub23_45 * InvSqrtSqrt	+ z5;
	auto z3 = sub12_56 * InvSqrt;
	auto z4 = sub01_67 * SqrtHalfSqrt + z5;
	auto z6 = sub07 + z3; // z11 ("phase 5")
	auto z7 = sub07 - z3; // z13
	block1 = z6 + z4; block7 = z6 - z4; // "phase 6"
	block5 = z7 + z2; block3 = z7 - z2;
}

Int16 EncodeBlock(BitWriter& writer, float block[8][8], const float scaled[8 * 8], Int16 lastDC, const BitCode huffmanDC[256], const BitCode huffmanAC[256], const BitCode* codewords)
{
	auto block64 = (float*)block;

	for (auto offset = 0; offset < 8; offset++) DCT(block64 + offset * 8, 1);

	for (auto offset = 0; offset < 8; offset++) DCT(block64 + offset * 1, 8);

	for (auto i = 0; i < 8 * 8; i++) block64[i] *= scaled[i];

	auto dc = Int32(block64[0] + (block64[0] >= 0 ? +0.5f : -0.5f)); // C++11's nearbyint() achieves a similar effect

	auto posNonZero = 0; // find last coefficient which is not zero (because trailing zeros are encoded differently)

	Int16 quantized[8 * 8];

	for (auto i = 1; i < 8 * 8; i++) // start at 1 because block64[0]=DC was already processed
	{
		auto value = block64[kZigZagInv[i]];

		quantized[i] = Int16(value + (value >= 0 ? + 0.5f : -0.5f)); // C++11's nearbyint() achieves a similar effect

		if (quantized[i] != 0) posNonZero = i;
	}

	auto diff = dc - lastDC;

	if (diff == 0)
	{
		writer << huffmanDC[0x00];
	}
	else
	{
		auto bits = codewords[diff];

		writer << huffmanDC[bits.numBits] << bits;
	}


	auto offset = 0;

	for (auto i = 1; i <= posNonZero; i++)
	{
		while (quantized[i] == 0)
		{
			offset += 0x10;

			if (offset > 0xF0)
			{
				writer << huffmanAC[0xF0];

				offset = 0;
			}

			i++;
		}

		auto encoded = codewords[quantized[i]];

		writer << huffmanAC[offset + encoded.numBits] << encoded; // and the value itself

		offset = 0;
	}


	if (posNonZero < 64 - 1) writer << huffmanAC[0x00];

	return Int16(dc);
}

void GenerateHuffmanTable(const UInt8 numCodes[16], const UInt8* values, BitCode result[256])
{
	UInt16 huffmanCode = 0;	//was int

	for (UInt8 numBits = 1; numBits <= 16; numBits++)
	{
		for (auto i = 0; i < numCodes[numBits - 1]; i++)  result[*values++] = BitCode(huffmanCode++, numBits);

		huffmanCode <<= 1;
	}
}

} }

bool TooJPEG::EncodeBitmap(const UInt8 * pixels, UInt32 width, UInt32 height, UInt8 nchannel, Options options, void * client, Output output)
{
	bool isRGB = nchannel == 3;

	BitWriter bitWriter(client, output);

	bitWriter << kHeaderJfif;


	if (auto n = options.comment_length)
	{
		bitWriter.AddMarker(0xFE, UInt16(2 + n));

		while (n--) bitWriter << UInt8(*options.comment++);
	}


	// write quantization tables

	auto quality = clamp<UInt16>(options.quality, 1, 100);

	quality = quality < 50 ? 5000 / quality : 200 - quality * 2;

	UInt8 quantLuminance[64];
	UInt8 quantChrominance[64];

	for (auto i = 0; i < 64; i++)
	{
		Int32 luminance = (kDefaultQuantLuminance[kZigZagInv[i]] * quality + 50) / 100;

		Int32 chrominance = (kDefaultQuantChrominance[kZigZagInv[i]] * quality + 50) / 100;

		quantLuminance[i] = UInt8(clamp(luminance, 1, 255));

		quantChrominance[i] = UInt8(clamp(chrominance, 1, 255));
	}

	bitWriter.AddMarker(0xDB, 2 + (isRGB ? 2 : 1) * (1 + 64)); // length: 65 bytes per table + 2 bytes for this length field
																															// each table has 64 entries and is preceded by an ID byte
	bitWriter << 0x00 << quantLuminance;	 // first	quantization table

	if (isRGB) bitWriter << 0x01 << quantChrominance; // second quantization table, only relevant for color images

	bitWriter.AddMarker(0xC0, 2 + 6 + 3 * Int16(nchannel)); // length: 6 bytes general info + 3 per channel + 2 bytes for this length field

	bitWriter << 0x08 << UInt8(height >> 8) << (height & 0xFF) << UInt8(width >> 8) << (width & 0xFF);

	bitWriter << nchannel;			 // 1 component (grayscale, Y only) or 3 components (Y,Cb,Cr)

	for (UInt8 id = 1; id <= nchannel; id++) bitWriter << id << (id == 1 && false ? 0x22 : 0x11) << (id == 1 ? 0 : 1); // use quantization table 0 for Y, table 1 for Cb and Cr


	// Huffman tables

	bitWriter.AddMarker(0xC4, isRGB ? (2+208+208) : (2+208));

	bitWriter << 0x00 << kDcLuminanceCodesPerBitsize << kDcLuminanceValues;

	bitWriter << 0x10 << kAcLuminanceCodesPerBitsize << kAcLuminanceValues;

	BitCode huffmanLuminanceDC[256];

	BitCode huffmanLuminanceAC[256];

	GenerateHuffmanTable(kDcLuminanceCodesPerBitsize, kDcLuminanceValues, huffmanLuminanceDC);

	GenerateHuffmanTable(kAcLuminanceCodesPerBitsize, kAcLuminanceValues, huffmanLuminanceAC);


	// chrominance is only relevant for color images

	BitCode huffmanChrominanceDC[256];

	BitCode huffmanChrominanceAC[256];

	if (isRGB)
	{
		bitWriter << 0x01 << kDcChrominanceCodesPerBitsize << kDcChrominanceValues;

		bitWriter << 0x11 << kAcChrominanceCodesPerBitsize << kAcChrominanceValues;

		GenerateHuffmanTable(kDcChrominanceCodesPerBitsize, kDcChrominanceValues, huffmanChrominanceDC);

		GenerateHuffmanTable(kAcChrominanceCodesPerBitsize, kAcChrominanceValues, huffmanChrominanceAC);
	}

	bitWriter.AddMarker(0xDA, 2 + 1 + 2 * nchannel + 3); // 2 bytes for the length field, 1 byte for number of components,
																										// then 2 bytes for each component and 3 bytes for spectral selection

	bitWriter << nchannel;

	for (UInt8 id = 1; id <= nchannel; id++) bitWriter << id << (id == 1 ? 0x00 : 0x11); // Y: tables 0 for DC and AC; Cb + Cr: tables 1 for DC and AC


	bitWriter << kSpectral;


	float scaledLuminance[64];
	float scaledChrominance[64];

	for (auto i = 0; i < 64; i++)
	{
		auto row = kZigZagInv[i] / 8; // same as ZigZagInv[i] >> 3

		auto column = kZigZagInv[i] % 8; // same as ZigZagInv[i] &	7

		auto factor = 1 / (kAanScaleFactors[row] * kAanScaleFactors[column] * 8);

		scaledLuminance[kZigZagInv[i]] = factor / quantLuminance	[i];

		scaledChrominance[kZigZagInv[i]] = factor / quantChrominance[i];

		// if you really want JPEGs that are bitwise identical to Jon Olick's code then you need slightly different formulas (note: sqrt(8) = 2.828427125f)
		//static const float aasf[] = { 1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f, 1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f }; // line 240 of jo_jpeg.cpp
		//scaledLuminance	[ZigZagInv[i]] = 1 / (quantLuminance	[i] * aasf[row] * aasf[column]); // lines 266-267 of jo_jpeg.cpp
		//scaledChrominance[ZigZagInv[i]] = 1 / (quantChrominance[i] * aasf[row] * aasf[column]);
	}

	// ////////////////////////////////////////
	// precompute JPEG codewords for quantized DCT
	BitCode	codewordsArray[2 * kCodeWordLimit];					// note: quantized[i] is found at codewordsArray[quantized[i] + kCodeWordLimit ]
	BitCode* codewords = &codewordsArray[kCodeWordLimit]; // allow negative indices, so quantized[i] is at codewords[quantized[i]]
	UInt8 numBits = 1; // each codeword has at least one bit (value == 0 is undefined)
	Int32 mask = 1; // mask is always 2^numBits - 1, initial value 2^1-1 = 2-1 = 1

	for (Int16 value = 1; value < kCodeWordLimit; value++)
	{
		// numBits = position of highest set bit (ignoring the sign)
		// mask		= (2^numBits) - 1
		if (value > mask) // one more bit ?
		{
			numBits++;
			mask = (mask << 1) | 1; // append a set bit
		}
		codewords[-value] = BitCode(UInt16(mask - value), numBits); // note that I use a negative index => codewords[-value] = codewordsArray[kCodeWordLimit value]
		codewords[+value] = BitCode(UInt16(value), numBits);
	}

	const auto maxWidth	= width	- 1;	//last row
	const auto maxHeight = height - 1;	//bottom line

	Int16 lastYDC = 0, lastCbDC = 0, lastCrDC = 0;

	float Y[8][8], Cb[8][8], Cr[8][8];

	for (UInt32 mcuY = 0; mcuY < height; mcuY += 8)
	{
		for (UInt32 mcuX = 0; mcuX < width; mcuX += 8)
		{
			for (UInt32 blockY = 0; blockY < 8; blockY += 8)
			{
				for (UInt32 blockX = 0; blockX < 8; blockX += 8)
				{
					for (UInt32 deltaY = 0; deltaY < 8; deltaY++)
					{
						auto column = minimum(mcuX + blockX, maxWidth); // must not exceed image borders, replicate last row/column if needed

						auto row = minimum(mcuY + blockY + deltaY, maxHeight);

						for (UInt32 deltaX = 0; deltaX < 8; deltaX++)
						{
							auto pixelPos = row * width + column; // the cast ensures that we don't run into multiplication overflows

							if (column < maxWidth) column++;

							if (!isRGB)
							{
								Y[deltaY][deltaX] = pixels[pixelPos] - 128.f;

								continue;
							}

							auto rgb = pixels + (3 * pixelPos);

							auto r = *rgb++;
							auto g = *rgb++;
							auto b = *rgb;

							Y[deltaY][deltaX] = rgb2y(r, g, b) - 128; // again, the JPEG standard requires Y to be shifted by 128

							Cb[deltaY][deltaX] = rgb2cb(r, g, b); // standard RGB-to-YCbCr conversion

							Cr[deltaY][deltaX] = rgb2cr(r, g, b);
						}
					}

					lastYDC = EncodeBlock(bitWriter, Y, scaledLuminance, lastYDC, huffmanLuminanceDC, huffmanLuminanceAC, codewords);
				}
			}

			if (!isRGB) continue;

			lastCbDC = EncodeBlock(bitWriter, Cb, scaledChrominance, lastCbDC, huffmanChrominanceDC, huffmanChrominanceAC, codewords);
			lastCrDC = EncodeBlock(bitWriter, Cr, scaledChrominance, lastCrDC, huffmanChrominanceDC, huffmanChrominanceAC, codewords);
		}
	}

	bitWriter.Flush(); // now image is completely encoded, write any bits still left in the buffer

	bitWriter << 0xFF << 0xD9; // this marker has no length, therefore I can't use AddMarker()

	return true;
}
