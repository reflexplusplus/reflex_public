#include "../../../../include/reflex_ext/glx/functions/dialogs.h"




//
//impl

Reflex::WString Reflex::GLX::ShowFileDialog(Data::PropertySet & prefs, Key32 id, bool save, const ArrayView <WString> & filters, const WString::View & suggested_, const WString::View & title)
{
	WString suggested = suggested_;

	auto last = Data::GetWString(prefs, id, System::GetPath(System::kPathDesktop));

	if (!File::Exists(suggested))
	{
		if (auto suggested_filename = File::SplitFilename(suggested).b)
		{
			suggested = Join(File::SplitFilename(last).a, suggested_filename);
		}
	}

	//auto [path, filename] = File::SplitFilename(last);

	//auto [suggested_path, suggested_filename] = File::SplitFilename(suggested);

	//if (suggested_path) path = suggested_path;

	//if (suggested_filename) filename = suggested_filename;

	if (auto filepath = ShowFileDialog(save, filters, suggested, title))
	{
		Data::SetWString(prefs, id, filepath);

		return filepath;
	}

	return {};
}

Reflex::WString Reflex::GLX::ShowFolderDialog(Data::PropertySet & prefs, Key32 id, bool save, const WString::View & defaultdir, const WString::View & title)
{
	auto dir = Data::GetWString(prefs, id, defaultdir);

	auto rtn = ShowFolderDialog(save, dir, title);

	if (rtn) Data::SetWString(prefs, id, rtn);

	return rtn;
}
