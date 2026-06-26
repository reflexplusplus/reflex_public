#pragma once

#include "reflex/glx/detail/bitmap.h"




//
//declarations

REFLEX_NS(Reflex::GLX::Detail)

void RemapToSupportedFormat(System::ImageFormat & format, Data::Archive & pixels);

extern const FunctionPointer <void(System::ImageFormat&,Data::Archive&)> kRemapBitmapFns[System::kNumImageFormat];

extern const bool * const kSupportsImageFormat;

REFLEX_END




//
//imp

REFLEX_INLINE void Reflex::GLX::Detail::RemapToSupportedFormat(System::ImageFormat & format, Data::Archive & pixels)
{
	REFLEX_LOOP(idx, 2)
	{
		if (kSupportsImageFormat[format]) return;

		kRemapBitmapFns[format](format, pixels);
	}
}
