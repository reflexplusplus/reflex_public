#pragma once

#include "detail/layout_model.h"




//
//Primary API

namespace Reflex::GLX
{

	extern const Detail::LayoutModelCtr kStandardLayout;

	extern const Detail::LayoutModelCtr kStandardLayoutWrapped;

}




//
// Detail

REFLEX_NS(Reflex::GLX::Detail)

[[deprecated("use GLX::kStandardLayoutWrapped")]] inline const LayoutModelCtr & kWrapLayout = kStandardLayoutWrapped;

extern const LayoutModelCtr kStandardLayoutRealigningContent;	//realign is propogated downward as well as upward, sometimes needed for AbstractViewPort content

REFLEX_END
