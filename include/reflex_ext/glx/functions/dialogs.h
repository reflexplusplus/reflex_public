#pragma once

#include "reflex/glx.h"




//
//Addon API

namespace Reflex::GLX
{

	WString ShowFileDialog(Data::PropertySet & prefs, Key32 id, bool save, ArrayView <WString> filters, WString::View suggested = {}, WString::View title = kSelectFile);

	WString ShowFolderDialog(Data::PropertySet & prefs, Key32 id, bool can_create = true, WString::View default_directory = System::GetPath(System::kPathDesktop), WString::View title = kSelectFolder);

}
