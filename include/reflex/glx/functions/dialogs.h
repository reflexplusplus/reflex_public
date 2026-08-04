#pragma once

#include "../[require].h"




//
//public

namespace Reflex::GLX
{

	extern const WString::View kSelectFile;

	extern const WString::View kSelectFolder;


	WString ShowFileDialog(bool save, const ArrayView <WString> & filters, const WString::View & previous = {}, const WString::View & title = kSelectFile);

	WString ShowFolderDialog(bool can_create = true, const WString::View & defaultdir = System::GetPath(System::kPathDesktop), const WString::View & title = kSelectFolder);

}




//
//detail

REFLEX_NS(Reflex::GLX::Detail)

struct ContextRestorer
{
	REFLEX_INLINE ~ContextRestorer()
	{
		//workaround for legacy opengl context sharing issues, need to set current after a modal dialog

		if (module.IsInitalised()) Core::g_renderer->BeginAccess();
	}
};

REFLEX_END
