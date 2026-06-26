#include "sdk.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

REFLEX_END_INTERNAL

Reflex::UInt32 Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 buttonflags)
{
	DEV_WARN("ShowMessageBox not supported");

	return 0;
}

Reflex::WString Reflex::System::GetOpenPath(const WString & title, const ArrayView <WString> & filters, const WString & dir, const WString & filename)
{
	DEV_WARN("GetOpenPath not supported");

	return {};
}

Reflex::WString Reflex::System::GetSavePath(const WString & title, const WString & filter, const WString & dir, const WString & filename)
{
	DEV_WARN("GetSavePath not supported");

	return {};
}

Reflex::WString Reflex::System::GetFolder(const WString & title, const WString & root, bool cancreate)
{
	DEV_WARN("GetFolder not supported");

	return {};
}

Reflex::Reference<Reflex::System::Task> Reflex::System::ShowMessageBox(UInt32 type, const WString& title, const ArrayView <WString>& text, UInt32 buttonflags, const Function <void(UInt32)>& callback) 
{
	DEV_WARN("ShowMessageBox not supported");

	return {};
}

Reflex::Reference<Reflex::System::Task> Reflex::System::SelectExternalResource(const ArrayView <WString> & mime_types, ExternalResourceRef::AccessMode accessType, bool allowMultiple, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback) 
{
	DEV_WARN("SelectExternalResource not supported");

	return {};
}

Reflex::Reference<Reflex::System::Task> Reflex::System::CreateExternalResource(const ArrayView <WString> & mime_types, ExternalResourceRef::AccessMode accessType, const WString::View & suggestedName, const Function<void(const Array<Reference<System::ExternalResourceRef>> & urls)> & callback) 
{
	DEV_WARN("CreateExternalResource not supported");

	return {};
}

Reflex::TRef<Reflex::System::ExternalResourceRef> Reflex::System::ExternalResourceRef::Locate(const ArrayView <UInt8> & token) 
{
	DEV_WARN("ExternalResourceRef not supported");

	return {};
}
