#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::File
{

	REFLEX_USE_ENUM(System, Path);

	constexpr WChar kPathDelimiter = System::kPathDelimiter;

	constexpr WChar kStroke = System::kPathDelimiter;

	constexpr WChar kDot = L'.';

	extern Output output;

}
