#include "[require].h"




//
//impl

Reflex::TRef <Reflex::System::DynamicLibrary> Reflex::System::DynamicLibrary::Create(const WString & filename) 
{
	DEV_ERROR("System::DynamicLibrary not implemented");

	return {};
}

Reflex::TRef <Reflex::System::DynamicLibrary> Reflex::System::DynamicLibrary::CreateFromBundle(const WString & filename, bool loadbundle) 
{
	return Create(filename);
}
