#pragma once

#include "detail/type.h"




//
//Experimental API

namespace Reflex::VM
{

	TRef <Object> Start();

	TypeID GenerateScriptTypeID(const StaticString & symbol, const WString::View & path);

	Reflex::Detail::DynamicTypeRef AcquireObjectType(Reflex::Detail::DynamicTypeRef base, const CString::View & ns_symbol, const WString::View & filepath = {});	//all types are global

	constexpr CString::View kNamespaceDelimiter = "::";

}
