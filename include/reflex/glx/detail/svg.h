#pragma once

#include "defines.h"
#include "functions/geometry.h"




//
//Detail

namespace Reflex::GLX::Detail
{

	struct SVG
	{
		Key32 id;
		Size size = kNormal;
		Pair <bool> size_normalized = { false, false };
		FitMode fit = kFitModeContain;
		Rect viewport = { {}, kNormal };
		Pair <Orientation> orientation = { kOrientationCenter, kOrientationCenter };
		ConstTRef <Data::PropertySet> node;	//only valid for lifetime of xml
	};

	Array <SVG> InspectSVG(const Data::PropertySet & xml);

	void DecodeSVG(ColourPoints & output, const SVG & svg, Size render_size, Float32 curve_step = kCornerStepRadians);


	extern const File::ResourcePool::Ctr kDecodeSVG;

}