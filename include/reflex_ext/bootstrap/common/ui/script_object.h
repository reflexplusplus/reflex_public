#pragma once

#include "[require].h"





//
//Secondary API

namespace Reflex::Bootstrap
{

	[[nodiscard]] Unretained <IDE::ResourceGroup> CreateScriptObject(const WString::View & path, UInt8 context_flags, const ArrayView <ConstRef<VM::Module>> & modules, const ArrayView < Tuple <CString::View, AlreadyRetained<Object>> > & externals, const Function <void(VM::Context & context, GLX::Object & object)> & on_create);

}
