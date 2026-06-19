#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::Bootstrap
{

	inline bool IsPlugin() { return System::kEnvironmentType == System::kEnvironmentTypeAudioPlugin; }

}
