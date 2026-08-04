#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	System::RawBitmap CropBitmap(const System::BitmapInfo & info, Data::Archive::View data, UInt x, UInt y, UInt w, UInt h);

	System::RawBitmap HalveBitmap(const System::BitmapInfo & info, Data::Archive::View data);

	Data::Archive BilinearResizeBitmap(const System::BitmapInfo & info, Data::Archive::View data, UInt w, UInt h);

}
