#pragma once

#include "common.h"




//
//Secondary API

namespace Reflex::Bootstrap
{

	TRef <Global> StartVmApp(System::App::Configuration & config, const CString::View & vendor, const CString::View & product, Key32 resources_subdomain, const char * entry, const WString::View & main, const WString::View & view);

}
