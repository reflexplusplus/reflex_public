#include "library.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

HWND st_folderwindow = 0;

const WChar * st_browsefolderlocation = 0;

REFLEX_NOINLINE WString ToWindowsPath(const WString::View & path)
{
	return Replace(path, kPathDelimiter, WChar(92));
}

WString GetFilename(bool save, const WString & title, const WString & dir, const ArrayView <WString> & filters, const WString & default_)
{
	OPENFILENAME ofn;

	MemClear(&ofn, sizeof(OPENFILENAME));

	ofn.lStructSize = sizeof(OPENFILENAME);

	//NOTE nulling this was the only way to workaround the GetFileTime bug

	ofn.hwndOwner = GetForegroundWindow();


	WChar buffer[512] = { 0 };

	RawStringCopy(default_.GetData(), buffer, 512);

	ofn.lpstrFile = buffer;

	ofn.nMaxFile = 512;

	ofn.nFilterIndex = 0;

	ofn.lpstrTitle = title.GetData();


	WString directory(dir);
	
	File::Detail::RemoveTrailingStroke(directory);
	
	directory = ToWindowsPath(directory);

	ofn.lpstrInitialDir = directory.GetData();


	if (save)
	{
		ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	}
	else
	{
		ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
	}

	WChar tnull = kMaxUInt16;

	WString filter;

	if (filters.size)
	{
		for (auto & ext : filters) filter.Append(Join(L'.', ext, L' '));

		filter.Push(tnull);

		for (auto & ext : filters) filter.Append(Join(L'*', L'.', ext, L';'));
	}
	else
	{
		filter.Append(Join(L'*', L'.', L'*'));

		filter.Push(tnull);

		filter.Append(Join(L'*', L'.', L'*'));
	}

	filter.Push(tnull);

	filter.Push(tnull);

	filter = Reflex::Replace(filter, tnull, WChar(0));

	ofn.lpstrFilter = filter.GetData();

	auto fn = save ? &GetSaveFileNameW : &GetOpenFileNameW;

	if ((*fn)(&ofn) > 0)
	{
		WString output(buffer + 0);
		
		File::Detail::CorrectStrokes(output);
		
		return output;
	}
	else
	{
		return {};
	}
}

int CALLBACK GetFolderCallback(HWND hwnd, UINT msg, LPARAM, LPARAM)
{
	if (msg == BFFM_INITIALIZED)
	{
		SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(st_browsefolderlocation));

		//RECT windowrect, dialogrect;

		//GetWindowRect(Win::st_folderwindow, &windowrect);

		//GetWindowRect(hwnd, &dialogrect);

		//POINT point;

		//point.x = (windowrect.left + ((windowrect.right - windowrect.left) / 2)) - ((dialogrect.right - dialogrect.left) / 2);

		//point.y = (windowrect.top + ((windowrect.bottom - windowrect.top) / 2)) - ((dialogrect.bottom - dialogrect.top) / 2);

		//SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(Win::st_browsefolderlocation));

		//MapWindowPoints(hwnd, GetParent(hwnd), &point, 1);

		//MoveWindow(hwnd, point.x, point.y, dialogrect.right - dialogrect.left, dialogrect.bottom - dialogrect.top, true);
	}

	return 0;
}

REFLEX_END_INTERNAL

Reflex::UInt32 Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 buttonflags)
{
	HWND hwnd = GetForegroundWindow();

	WString msg;

	if (text)
	{
		const WChar delim[2] = {10, 10};

		for (auto & i : text)
		{
			msg.Append(i);

			msg.Append({ delim, 2 });
		}

		msg.Shrink(2);
	}

	//msg.Push(WChar(1)) = WChar(0);	//needs double terminator, avoid assertion

	UINT flags = MB_APPLMODAL | MB_ICONEXCLAMATION;

	return MessageBoxW(hwnd, msg.GetData(), title.GetData(), flags);
}

Reflex::WString Reflex::System::GetOpenPath(const WString & title, const ArrayView <WString> & filters, const WString & dir, const WString & filename)
{
	return Win::GetFilename(false, title, dir, filters, filename);
}

Reflex::WString Reflex::System::GetSavePath(const WString & title, const WString & filter, const WString & dir, const WString & filename)
{
	ArrayView <WString> filters(&filter, 1);

	return Win::GetFilename(true, title, dir, filters, filename);
}

Reflex::WString Reflex::System::GetFolder(const WString & title, const WString & root, bool cancreate)
{
	Win::st_folderwindow = GetForegroundWindow();

	WString location = Win::ToWindowsPath(root);

	Win::st_browsefolderlocation = location.GetData();

	BROWSEINFO browseinfo;

	UInt32 flags[2] = { BIF_USENEWUI | BIF_NONEWFOLDERBUTTON, BIF_USENEWUI };

	browseinfo.hwndOwner = Win::st_folderwindow;
	browseinfo.pidlRoot = 0;
	browseinfo.pszDisplayName = 0;
	browseinfo.lpszTitle = title.GetData();
	browseinfo.ulFlags = flags[UInt8(cancreate)];
	browseinfo.lpfn = &Win::GetFolderCallback;
	browseinfo.lParam = 0;
	browseinfo.iImage = 0;

	PIDLIST_ABSOLUTE result = SHBrowseForFolder(&browseinfo);

	if (result)
	{
		WChar buffer[512] = {0};

		SHGetPathFromIDListW(result, buffer);

		WString output(buffer + 0);
		
		File::Detail::CorrectStrokes(output);
		
		File::Detail::CorrectTrailingStroke(output);
		
		return output;
	}

	return {};
}

Reflex::Reference<Reflex::System::Task> Reflex::System::ShowMessageBox(UInt32 type, const WString& title, const ArrayView <WString>& text, UInt32 buttonflags, const Function <void(UInt32 clickedButton)>& callback) 
{
	// TODO: implement for desktop
	DEV_WARN("ShowMessageBox Not supported on desktop platforms");
	return Task::null;
}

Reflex::Reference<Reflex::System::Task> Reflex::System::SelectExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, bool allowMultiple, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback) 
{
	// TODO: implement for desktop
	DEV_WARN("SelectExternalResource Not supported on desktop platforms");
	return Task::null;
}

Reflex::Reference<Reflex::System::Task> Reflex::System::CreateExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, const WString::View& suggestedName, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback)
{
	// TODO: implement for desktop
	DEV_WARN("CreateExternalResource Not supported on desktop platforms");
	return Task::null;
}

Reflex::TRef<Reflex::System::ExternalResourceRef> Reflex::System::ExternalResourceRef::Locate(const ArrayView<UInt8>& token) 
{
	// TODO: implement for desktop
	DEV_WARN("ExternalResourceRef Not supported on desktop platforms");
	return ExternalResourceRef::null;
}

bool Reflex::System::ShowVirtualKeyboard(VirtualKeyboardInputType type, const WString& textbuffer, Pair<UInt> selection, const Function<void(const WString&, Pair<UInt>)>& ondone)
{
	return false;
}

void Reflex::System::DismissVirtualKeyboard()
{
}
