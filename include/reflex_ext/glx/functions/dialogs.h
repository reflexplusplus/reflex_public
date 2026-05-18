#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	WString ShowFileDialog(Data::PropertySet & prefs, Key32 id, bool save, const ArrayView <WString> & filters, const WString::View & previous = {}, const WString::View & title = kSelectFile);

	WString ShowFolderDialog(Data::PropertySet & prefs, Key32 id, bool can_create = true, const WString::View & defaultdir = System::GetPath(System::kPathDesktop), const WString::View & title = kSelectFolder);

}
