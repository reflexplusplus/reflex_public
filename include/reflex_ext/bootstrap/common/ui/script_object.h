#pragma once

#include "[require].h"





//
//Secondary API

namespace Reflex::Bootstrap
{

	TRef <IDE::ResourceGroup> CreateScriptObject(const WString::View & path, const ArrayView <ConstTRef<VM::Module>> & modules, const ArrayView < Tuple <CString::View, TRef<Object>> > & externals, const Function <void(VM::Context & context, GLX::Object & object)> & on_create);

}
