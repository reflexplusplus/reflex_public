#include "[require].h"

Reflex::UInt32 Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 button_flags)
{
	return 0;
}

Reflex::WString Reflex::System::GetOpenPath(const WString & title, const ArrayView <WString> & filters, const WString & dir, const WString & filename)
{
	return {};
}

Reflex::WString Reflex::System::GetSavePath(const WString & title, const WString & filter, const WString & dir, const WString & filename)
{
	return {};
}

Reflex::WString Reflex::System::GetFolder(const WString & title, const WString & dir, bool can_create)
{
	return {};
}

Reflex::Reference <Reflex::System::Task> Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 button_flags, const Function<void(UInt32 clicked_button)> & callback)
{
	return Task::null;
}

Reflex::Reference <Reflex::System::Task> Reflex::System::SelectExternalResource(const ArrayView <WString> & mime_types, ExternalResourceRef::AccessMode access_type, bool allow_multiple, const Function<void(const Array<Reference<ExternalResourceRef>> & urls)> & callback)
{
	return Task::null;
}

Reflex::Reference <Reflex::System::Task> Reflex::System::CreateExternalResource(const ArrayView <WString> & mime_types, ExternalResourceRef::AccessMode access_type, const WString::View & suggested_name, const Function<void(const Array<Reference<ExternalResourceRef>> & urls)> & callback)
{
	return Task::null;
}

bool Reflex::System::ShowVirtualKeyboard(VirtualKeyboardInputType type, const WString & textbuffer, Pair <UInt> selection, const Function <void(const WString &, Pair <UInt>)> & ondone)
{
	return false;
}

void Reflex::System::DismissVirtualKeyboard()
{
}