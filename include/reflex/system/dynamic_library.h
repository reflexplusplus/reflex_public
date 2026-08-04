#pragma once

#include "types.h"




//
//Primary API

namespace Reflex::System
{

	class DynamicLibrary;

}




//
//DynamicLibrary

class Reflex::System::DynamicLibrary : public Object
{
public:

	REFLEX_OBJECT(System::DynamicLibrary, Object);

	static DynamicLibrary & null;



	//lifetime

	[[nodiscard]] static TRef <DynamicLibrary> Create(const WString & path);

	[[nodiscard]] static TRef <DynamicLibrary> CreateFromBundle(const WString & path, bool load_bundle);	//OSX ONLY



	//interface

	virtual bool Status() const = 0;

	virtual void * GetBundleRef() const = 0;													//OSX ONLY

	virtual void * GetFunction(const CString & function) const = 0;

};
