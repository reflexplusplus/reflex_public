#include "../library.h"




//
//win::app implementation

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

void ConvertClipboard(const CString::View & ref, WString & clipboard)
{
	clipboard = Reflex::ToWString(ref);
}

void ConvertClipboard(const WString::View & ref, WString & clipboard)
{
	clipboard = ref;
}

template <class CHARACTER> void ReadClipboard(UINT format, WString & clipboard)
{
	if (HANDLE mem = GetClipboardData(format))
	{
		if (auto copy = GlobalLock(mem))
		{
			ConvertClipboard(Cast<CHARACTER>(copy), clipboard);

			GlobalUnlock(mem);
		}
	}
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprect, LPARAM dwData)
{
	Int32 pixdensity = globals->m_maxpixeldensity;

	iRect & irect = globals->m_screens.Push();

	irect.origin.x = lprect->left / pixdensity;

	irect.origin.y = lprect->top / pixdensity;

	irect.size.w = (lprect->right - irect.origin.x) / pixdensity;

	irect.size.h = (lprect->bottom - irect.origin.y) / pixdensity;

	return TRUE;
}

void SetClipboardImpl(Int fmt, const ArrayView <UInt8> & bytes)
{
	OpenClipboard(NULL);

	EmptyClipboard();

	HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes.size);

	auto copy = GlobalLock(mem);

	MemCopy(bytes.data, copy, bytes.size);

	GlobalUnlock(mem);

	SetClipboardData(fmt, mem);

	CloseClipboard();
}

REFLEX_END_INTERNAL

Reflex::Int32 Reflex::System::GetMaxPixelDensity()
{
	return Win::globals->m_maxpixeldensity;
}

Reflex::Array <Reflex::System::iRect> Reflex::System::GetScreens()
{
	auto & screens = Win::globals->m_screens;

	if (!screens) EnumDisplayMonitors(NULL, NULL, &Win::MonitorEnumProc, 0);

	return screens;
}

Reflex::CString Reflex::System::GetLanguage() 
{
	WCHAR name[LOCALE_NAME_MAX_LENGTH] = { 0 };

	if (GetUserDefaultLocaleName(name, LOCALE_NAME_MAX_LENGTH) > 0)
	{
		WCHAR lang[16] = { 0 };
		if (GetLocaleInfoEx(name, LOCALE_SISO639LANGNAME, lang, 16) > 0)
		{
			const WChar * plang = lang;

			return ToCString(ToView(plang));
		}
	}

	return "en";
}

bool Reflex::System::IsDarkTheme()
{
	HKEY handle;
	DWORD value = 1; // (default to light mode)
	DWORD data_size = sizeof(value);

	auto reg_open_result = RegOpenKeyExW(
		HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
		0,
		KEY_READ,
		&handle);

	if (reg_open_result == ERROR_SUCCESS)
	{
		RegQueryValueExW(
			handle,
			L"AppsUseLightTheme",
			nullptr,
			nullptr,
			Reinterpret<BYTE>(&value),
			&data_size);

		RegCloseKey(handle);
	}

	return value == 0;
}

Reflex::Float Reflex::System::GetFontScale()
{
	return 1.0f;
}

Reflex::UInt8 Reflex::System::GetModifierKeys()
{
	return Win::Library::st_modifier_key_flags;
}

void Reflex::System::SetClipboard(const WString & string)
{
	constexpr WString::View kEOL = L"\r\n";

	auto itr = ToView(string);

	WString copy;

	copy.Reserve(itr.size);
			
	while (itr)
	{
		auto line = Data::Detail::ReadLine(itr);

		copy.Append(line);

		if ((line.data + line.size) != (itr.data + itr.size))
		{
			copy.Append(kEOL);
		}
	}

	Win::SetClipboardImpl(CF_UNICODETEXT, { Reinterpret<UInt8>(copy.GetData()), UInt((copy.GetSize() + 1) * sizeof(WChar)) });
}

Reflex::WString Reflex::System::GetClipboard()
{
	WString rtn;

	OpenClipboard(0);

	UINT format = 0;

	while (format = EnumClipboardFormats(format))
	{
		switch(format)
		{
		case CF_UNICODETEXT:
			Win::ReadClipboard<WChar>(CF_UNICODETEXT, rtn);
			break;

		case CF_TEXT:
			if (!rtn) Win::ReadClipboard<char>(CF_TEXT, rtn);
			break;
		};
	}

	CloseClipboard();

	return rtn;
}
