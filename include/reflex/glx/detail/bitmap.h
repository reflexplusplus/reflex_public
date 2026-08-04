#pragma once

#include "defines.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

ConstTRef <System::Renderer::Canvas> RetrieveBitmap(const WString::View & path, UInt pixeldensity, bool antialias);

ConstTRef <System::Renderer::Canvas> OpenBitmap(const System::BitmapInfo & info, const Data::Archive::View & data, bool antialias);

ConstTRef <System::Renderer::Canvas> OpenBitmap(const Data::Archive::View & data, UInt pixeldensity, bool antialias);


System::RawBitmap DecodeBitmap(const Data::Archive::View & data, UInt pixeldensity);

void PreMultAlpha(const System::BitmapInfo & info, Data::Archive & data);


System::RawBitmap DecodePNG(const Data::Archive::View & data, UInt pixeldensity);

Data::Archive EncodePNG(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 compress = true);


System::RawBitmap DecodeBMP(const Data::Archive::View & data, UInt pixeldensity);

Data::Archive EncodeBMP(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 flags = 0);


System::RawBitmap DecodeJPG(const Data::Archive::View & data, UInt pixeldensity);

Data::Archive EncodeJPG(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 quality = 90);


System::RawBitmap DecodeGLX(const Data::Archive::View & data);

Data::Archive EncodeGLX(const System::BitmapInfo & info, const Data::Archive::View & data, UInt8 flags = 0);


void AllocateBitmap(const System::BitmapInfo & info, Data::Archive & archive);


System::RawBitmap CropBitmap(const System::BitmapInfo & info, const Data::Archive::View & data, const System::iRect & rect);

System::RawBitmap HalveBitmap(const System::BitmapInfo & info, const Data::Archive::View & data);


extern const File::ResourcePool::Ctr kDecodeBitmap;

extern bool (&VerifyBitmap)(const System::BitmapInfo & info, const Data::Archive::View & data);


constexpr UInt32 kPNG = UInt32(1196314761ul);

constexpr UInt16 kJPG = UInt16(55551);

REFLEX_END




//
//impl

inline Reflex::ConstTRef <Reflex::System::Renderer::Canvas> Reflex::GLX::Detail::OpenBitmap(const Data::Archive::View & archive, UInt pixeldensity, bool antialias)
{
	auto bitmap = DecodeBitmap(archive, pixeldensity);

	PreMultAlpha(bitmap.a, bitmap.b);

	return OpenBitmap(bitmap.a, bitmap.b, antialias);
}

