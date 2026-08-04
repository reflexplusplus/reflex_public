#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Bootstrap
{

	inline bool IsPlugin() { return System::kEnvironmentType == System::kEnvironmentTypeAudioPlugin; }

	
	Data::PropertySet ParseCmdlineArgs(ArrayView <CString::View> cmdline, bool typed, UInt8 type_flags = 0);

}
