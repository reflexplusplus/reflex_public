#include "../../../../include/reflex/glx/functions/dialogs.h"




//
//impl

const Reflex::WString::View Reflex::GLX::kSelectFile = L"Select File";

const Reflex::WString::View Reflex::GLX::kSelectFolder = L"Select Folder";

Reflex::WString Reflex::GLX::ShowFileDialog(bool save, const ArrayView <WString> & filters, const WString::View & path, const WString::View & title)
{
	WString dir = path;

	WString filename;

	if (!System::IsDirectory(dir))
	{
		auto split = File::SplitFilename(path);

		auto desktop = System::GetPath(System::kPathDesktop);

		dir = split.a ? File::ResolveExistingFolder(split.a) : ToView(desktop);

		filename = split.b;
	}

	if (filename && filters) filename = File::CorrectExtension(filename, filters.GetFirst());

	GLX::Detail::ContextRestorer context;

	if (save)
	{
		auto path = System::GetSavePath(title, filters ? filters.GetFirst() : Reflex::Detail::GetNullInstance<WString>(), dir, filename);

		if (path && filters && !Search(filters, File::GetExtension(path)))
		{
			path = File::CorrectExtension(path, filters.GetFirst());
		}

		return path;
	}
	else
	{
		return System::GetOpenPath(title, filters, dir, filename);
	}
}

Reflex::WString Reflex::GLX::ShowFolderDialog(bool save, const WString::View & defaultdir, const WString::View & title)
{
	Detail::ContextRestorer context;

	return System::GetFolder(title, File::ResolveExistingFolder(defaultdir), save);
}
