#include "../../../../include/reflex_ext/glx/functions/dialogs.h"




//
//impl

Reflex::WString Reflex::GLX::ShowFileDialog(Data::PropertySet & prefs, Key32 id, bool save, ArrayView <WString> filters, WString::View suggested, WString::View title)
{
	auto previous = Data::GetWString(prefs, id);

	auto [suggested_folder, suggested_filename] = File::SplitFilename(suggested);
	auto [previous_folder, previous_filename] = File::SplitFilename(previous);

	WString folder = System::GetPath(System::kPathDesktop);

	if (File::IsDirectory(previous_folder))
	{
		folder = previous_folder;
	}
	else if (File::IsDirectory(suggested_folder))
	{
		folder = suggested_folder;
	}

	WString::View filename = suggested_filename ? suggested_filename : previous_filename;

	auto path = Join(folder, filename);

	if (save && filters && filename)
	{
		if (!Search<CaseInsensitive>(filters, File::GetExtension(filename)))
		{
			path = File::CorrectExtension(path, filters.GetFirst());
		}
	}

	auto selected = ShowFileDialog(save, filters, path, title);

	if (selected)
	{
		Data::SetWString(prefs, id, selected);
	}

	return selected;
}

Reflex::WString Reflex::GLX::ShowFolderDialog(Data::PropertySet & prefs, Key32 id, bool save, WString::View default_directory, WString::View title)
{
	auto dir = Data::GetWString(prefs, id, default_directory);

	auto rtn = ShowFolderDialog(save, dir, title);

	if (rtn) Data::SetWString(prefs, id, rtn);

	return rtn;
}
