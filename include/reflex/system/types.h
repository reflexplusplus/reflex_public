#pragma once

#include "defines.h"




//
//Secondary API

namespace Reflex::System
{

	using fPoint = Point <Float32>;

	using iPoint = Point <Int32>;


	using fSize = Size <Float32>;

	using iSize = Size <Int32>;


	using fRect = Rect <Float32>;

	using iRect = Rect <Int32>;


	struct Colour;

	using ColourPoint = Pair <fPoint,Colour>;


	struct BitmapInfo;

	using RawBitmap = Pair < BitmapInfo, Array <UInt8> >;

}




//
//Colour

struct Reflex::System::Colour
{
	Float32 r = 0.0f;
	Float32 g = 0.0f;
	Float32 b = 0.0f;
	Float32 a = 0.0f;
};




//
//BitmapInfo

struct Reflex::System::BitmapInfo
{
	iSize size;
	Int32 pixdensity;
	ImageFormat format;
};




//
//impl

REFLEX_ASSERT_RAW(Reflex::System::Colour)
REFLEX_ASSERT_RAW(Reflex::System::fPoint)
REFLEX_ASSERT_RAW(Reflex::System::fSize)
REFLEX_ASSERT_RAW(Reflex::System::fRect)


