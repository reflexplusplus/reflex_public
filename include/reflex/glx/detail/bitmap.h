#pragma once

#include "defines.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

System::RawBitmap DecodeBitmap(Data::Archive::View data);	//supports PNG, JPG and BMP


System::RawBitmap DecodePNG(Data::Archive::View data);

Data::Archive EncodePNG(const System::BitmapInfo & info, Data::Archive::View data, UInt8 compress = true);


System::RawBitmap DecodeBMP(Data::Archive::View data);

Data::Archive EncodeBMP(const System::BitmapInfo & info, Data::Archive::View data, UInt8 flags = 0);


System::RawBitmap DecodeJPG(Data::Archive::View data);

Data::Archive EncodeJPG(const System::BitmapInfo & info, Data::Archive::View data, UInt8 quality = 90);


System::RawBitmap DecodeGLX(Data::Archive::View data);

Data::Archive EncodeGLX(const System::BitmapInfo & info, Data::Archive::View data, UInt8 flags = 0);


void AllocateBitmap(const System::BitmapInfo & info, Data::Archive & archive);


bool ConvertPixelDensity(System::BitmapInfo & info, UInt pixel_density);

void PreMultAlpha(const System::BitmapInfo & info, Data::Archive & data);

bool ConvertToSupportedFormat(System::ImageFormat & format, Data::Archive & pixels, const bool(&supported)[System::kNumImageFormat]);	//returns true if input format was already supported, or was converted to supported


extern bool (&VerifyBitmap)(const System::BitmapInfo & info, const Data::Archive::View & data);


extern bool g_supported_image_formats[System::kNumImageFormat];		// Initialized at GLX startup; do not modify afterward.


constexpr UInt32 kGLX = 193456912ul;

constexpr UInt32 kPNG = UInt32(1196314761ul);

constexpr UInt16 kJPG = UInt16(55551);

REFLEX_END
