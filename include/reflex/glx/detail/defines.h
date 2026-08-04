#pragma once

#include "[require].h"
#include "../layout/standard.h"




//
//Detail

namespace Reflex::GLX::Detail
{

	using Graphic = System::Renderer::Graphic;

	enum FitMode : UInt8
	{
		kFitModeNone,
		kFitModeStretch,
		kFitModeContain,
		kFitModeCover,
	};


	extern const Key32 kOrientationKeys[kNumOrientation];

	constexpr System::fRect kNormalRect = { { 0.0f, 0.0f }, { 1.0f, 1.0f } };


	extern const bool & kCanRenderToTexture;

	extern const Float32 & kPixelSize;


	extern Data::Detail::PropertySheetInterface & iresource;

	extern Data::Detail::PropertySheetInterface & istylesheet;

}