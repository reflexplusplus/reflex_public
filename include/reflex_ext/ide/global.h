#pragma once

#include "resource_group.h"




//
//Primary API

namespace Reflex::IDE
{

	TRef <Object> Start(File::ResourcePool & resourcepool, Data::PropertySet & prefs = Data::PropertySet::null);


	extern const bool & kIsAwake;

	extern const WString::View kC, kH, kGLX, kTXT, kJSON, kXML;

}
