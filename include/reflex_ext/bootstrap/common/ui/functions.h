#pragma once

#include "detail.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	void SetStyleSheet(GLX::Object & view, const WString::View & path, FunctionPointer <TRef<Data::PropertySet>(GLX::Object & view)> create_options = &Detail::CreateDefaultStylesheetOptions);


	void SetClipboard(Key32 type, const Data::Archive::View & data);

	Data::Archive::View GetClipboard(Key32 type);

}




//
//impl

inline void Reflex::Bootstrap::SetStyleSheet(GLX::Object & view, const WString::View & path, FunctionPointer <TRef<Data::PropertySet>(GLX::Object & view)> create_options)
{
	Detail::SetStyle(view, path, {}, create_options);
}
