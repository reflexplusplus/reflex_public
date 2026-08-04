#pragma once

#include "defines.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

System::RawBitmap DecodeBitmap(const Data::Archive::View & data, UInt pixel_density);

void PreMultAlpha(const System::BitmapInfo & info, Data::Archive & data);


System::RawBitmap DecodePNG(const Data::Archive::View & data, UInt pixel_density);

Data::Archive EncodePNG(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 compress = true);


System::RawBitmap DecodeBMP(const Data::Archive::View & data, UInt pixel_density);

Data::Archive EncodeBMP(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 flags = 0);


System::RawBitmap DecodeJPG(const Data::Archive::View & data, UInt pixel_density);

Data::Archive EncodeJPG(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 quality = 90);


System::RawBitmap DecodeGLX(const Data::Archive::View & data);

Data::Archive EncodeGLX(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 flags = 0);


void AllocateBitmap(const System::BitmapInfo & info, Data::Archive & archive);

void RemapToSupportedFormat(System::ImageFormat & format, Data::Archive & pixels);


extern bool (&VerifyBitmap)(const System::BitmapInfo & info, const Data::Archive::View & data);


extern const FunctionPointer <void(System::ImageFormat&, Data::Archive&)> kRemapBitmapFns[System::kNumImageFormat];

extern const bool * const kSupportsImageFormat;


constexpr UInt32 kGLX = 193456912ul;

constexpr UInt32 kPNG = UInt32(1196314761ul);

constexpr UInt16 kJPG = UInt16(55551);

REFLEX_END




//
//impl

REFLEX_INLINE void Reflex::GLX::Detail::RemapToSupportedFormat(System::ImageFormat & format, Data::Archive & pixels)
{
	REFLEX_LOOP(idx, 2)
	{
		if (kSupportsImageFormat[format]) return;

		kRemapBitmapFns[format](format, pixels);
	}
}
