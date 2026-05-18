#pragma once

#include "types.h"




//
//Secondary API

namespace Reflex::System
{

	UInt32 ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 button_flags);

	WString GetOpenPath(const WString & title, const ArrayView <WString> & filters, const WString & dir = {}, const WString & filename = {});

	WString GetSavePath(const WString & title, const WString & filter, const WString & dir = {}, const WString & filename = {});

	WString GetFolder(const WString & title, const WString & dir = {}, bool can_create = true);


	Reference <Task> ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 button_flags, const Function<void(UInt32 clicked_button)> & callback);

	Reference <Task> SelectExternalResource(const ArrayView <WString> & mime_types, ExternalResourceRef::AccessMode access_type, bool allow_multiple, const Function<void(const Array<Reference<ExternalResourceRef>> & urls)> & callback);

	// Note: the first element in mime_types determines type of resulting file, the remaining types will tell which file types are shown in the picker
	Reference <Task> CreateExternalResource(const ArrayView <WString> & mime_types, ExternalResourceRef::AccessMode access_type, const WString::View & suggested_name, const Function<void(const Array<Reference<ExternalResourceRef>> & urls)> & callback);


	enum VirtualKeyboardInputType : Reflex::UInt8
	{
		kVirtualKeyboardInputNormal,
		kVirtualKeyboardInputEmail,
		kVirtualKeyboardInputPassword,
		kVirtualKeyboardInputNumber,
		kVirtualKeyboardInputPhoneNumber,
		kVirtualKeyboardInputURL,
		kVirtualKeyboardInputMultiLine,
	};

	bool ShowVirtualKeyboard(VirtualKeyboardInputType type, const WString & textbuffer, Pair <UInt> selection, const Function <void(const WString &, Pair <UInt>)> & ondone);

	void DismissVirtualKeyboard();

}
