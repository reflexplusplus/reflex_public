#pragma once

#include "../detail/bitmap.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

ConstAlreadyRetained <System::Renderer::Canvas> RetrieveBitmap(const WString::View & path, UInt pixel_density, bool antialias);

Unretained <System::Renderer::Canvas> CreateBitmap(const System::BitmapInfo & info, Data::Archive::View data, bool antialias);

Unretained <System::Renderer::Canvas> CreateBitmap(Data::Archive::View data, UInt pixel_density, bool antialias);


extern const File::ResourcePool::Ctr kDecodeBitmap;

REFLEX_END




//
//impl

inline Reflex::Unretained <Reflex::System::Renderer::Canvas> Reflex::GLX::Detail::CreateBitmap(Data::Archive::View archive, UInt pixel_density, bool antialias)
{
	auto [info,bytes] = DecodeBitmap(archive);

	if (ConvertToSupportedFormat(info.format, bytes, g_supported_image_formats))
	{
		if (ConvertPixelDensity(info, pixel_density))
		{
			PreMultAlpha(info, bytes);

			return CreateBitmap(info, bytes, antialias);
		}
	}

	return System::Renderer::Canvas::null;
}
