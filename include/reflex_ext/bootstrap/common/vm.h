#pragma once

#include "[require].h"
#include "reflex/vm.h"





//
//Secondary API

namespace Reflex::Bootstrap
{

	struct ScriptExternals : public Object
	{
		Array < Tuple < VM::StaticString, TRef <Object> > > objects;
	};

	extern const VM::Module g_externals;

}