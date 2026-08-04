#pragma once

#include "../detail/bitmap.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

ConstTRef <System::Renderer::Canvas> RetrieveBitmap(const WString::View & path, UInt pixel_density, bool antialias);

ConstTRef <System::Renderer::Canvas> OpenBitmap(const System::BitmapInfo & info, const Data::Archive::View & data, bool antialias);

ConstTRef <System::Renderer::Canvas> OpenBitmap(const Data::Archive::View & data, UInt pixel_density, bool antialias);


extern const File::ResourcePool::Ctr kDecodeBitmap;

REFLEX_END




//
//impl

inline Reflex::ConstTRef <Reflex::System::Renderer::Canvas> Reflex::GLX::Detail::OpenBitmap(const Data::Archive::View & archive, UInt pixel_density, bool antialias)
{
	auto [info,bytes] = DecodeBitmap(archive, pixel_density);

	RemapToSupportedFormat(info.format, bytes);

	PreMultAlpha(info, bytes);

	return OpenBitmap(info, bytes, antialias);
}
