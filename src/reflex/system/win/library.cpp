#include "library.h"




//
//Win globals

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

REFLEX_INLINE UInt64 GetUInt64(DWORD lo, DWORD hi)
{
	Pair <UInt32> result = { lo, hi };

	return Reinterpret<UInt64>(result);
}

REFLEX_INLINE UInt64 ToTimestamp(FILETIME & filetime)
{
	return (GetUInt64(filetime.dwLowDateTime, filetime.dwHighDateTime) / 10000000ul) - 11644473600ul;
}

#if REFLEX_INCLUDE_UI
constexpr WString::View kHiddenWindowClass = L"Reflex::System::Library";
#endif

REFLEX_END_INTERNAL

HINSTANCE Reflex::System::Win::Library::st_hinstance = nullptr;

volatile bool Reflex::System::Win::Library::st_internet_ready = false;

volatile decltype (&GetFileAttributesExW) Reflex::System::Win::Library::GetFileAttributesWindowsBugWorkaround = &GetFileAttributesExW;

#if REFLEX_INCLUDE_UI
Reflex::UInt8 Reflex::System::Win::Library::st_modifier_key_flags = 0;

Reflex::UInt8 Reflex::System::Win::Library::kMsgMap[676] = {0};
#endif

Reflex::System::Win::Library::Library()
	: m_debugger(::IsDebuggerPresent())
	, m_internet_deinit(0)
	, m_internet_handle(0)
#if REFLEX_INCLUDE_UI
	, m_maxpixeldensity(1)
	, m_enable_truefullscreen(false)
	, m_hwnd_hidden(0)
#endif
{
	//init

	if (!st_hinstance)
	{
		//library case

		auto status = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)&st_hinstance, (HMODULE*)&st_hinstance);

		if (status == 0 || st_hinstance == nullptr)
		{
			st_hinstance = GetModuleHandle(nullptr);
		}
	}


#if !REFLEX_INCLUDE_UI
	st_internet_ready = true;
#endif



	//systemid

	auto systemid = Reinterpret<UInt32>(&m_systemid);


	DWORD serial;

	GetVolumeInformationA("C:/", 0, 0, &serial, 0, 0, 0, 0);

	systemid[0] = serial;


	typedef Tuple <Int32,Int32,Int32,Int32> CPUID;

	CPUID cpui;

	REFLEX_STATIC_ASSERT(sizeof(cpui) == 16);

	__cpuid(&cpui.a, 0x80000000);

	auto nExIds_ = cpui.a;

	char brand[64];

	memset(brand, 0, sizeof(brand));

	Array <CPUID> extdata_;

	extdata_.Allocate(48);

	for (int i = 0x80000000; i <= nExIds_; ++i)
	{
		__cpuidex(&cpui.a, i, 0);

		extdata_.Push(cpui);
	}

	if (nExIds_ >= 0x80000004)
	{
		memcpy(brand, &extdata_[2], sizeof(cpui));
		memcpy(brand + 16, &extdata_[3], sizeof(cpui));
		memcpy(brand + 32, &extdata_[4], sizeof(cpui));
	}

	systemid[1] = Reflex::Detail::MakeHash<UInt32>(CString::View(brand + 0));


	//time

	SYSTEMTIME reference;

	reference.wYear = 2001;
	reference.wMonth = 1;
	reference.wDay = 1;
	reference.wDay = 1;
	reference.wHour = 0;
	reference.wMinute = 0;
	reference.wSecond = 0;
	reference.wMilliseconds = 0;

	FILETIME m_reference_ft;

	SystemTimeToFileTime(&reference, &m_reference_ft);


	LARGE_INTEGER timerfrequency;

	QueryPerformanceFrequency(&timerfrequency);

	st_timermult = Float64(1.0 / Float64(timerfrequency.QuadPart));



#if REFLEX_INCLUDE_UI
	//ole (drag & drop / clipboard)
	
	OleInitialize(0);


	//global classes

	WNDCLASS wndclass;
	wndclass.lpszClassName = kHiddenWindowClass.data;
	wndclass.style = 0;
	wndclass.lpfnWndProc = &Win::Library::WinProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = st_hinstance;
	wndclass.hIcon = LoadIcon(st_hinstance, MAKEINTRESOURCE(0));
	wndclass.hCursor = LoadCursor(0, 0);	//NULL
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = 0;	//NULL

	RegisterClass(&wndclass);

	wndclass.lpszClassName = kVisibleWindowClass.data;
	wndclass.style = CS_DBLCLKS | CS_OWNDC;

	RegisterClass(&wndclass);



	//key

	InitKeyCodes();



	//mouse

	m_mouseover = 0;

	m_hcursor[kMouseCursorInvisible] = LoadCursor(NULL, NULL);
	m_hcursor[kMouseCursorArrow] = LoadCursor(NULL, IDC_ARROW);
	m_hcursor[kMouseCursorWait] = LoadCursor(NULL, IDC_WAIT);
	m_hcursor[kMouseCursorMove] = LoadCursor(NULL, IDC_SIZEALL);
	m_hcursor[kMouseCursorLeftRight] = LoadCursor(NULL, IDC_SIZEWE);
	m_hcursor[kMouseCursorTopBottom] = LoadCursor(NULL, IDC_SIZENS);
	m_hcursor[kMouseCursorTopLeftBottomRight] = LoadCursor(NULL, IDC_SIZENWSE);
	m_hcursor[kMouseCursorBottomLeftTopRight] = LoadCursor(NULL, IDC_SIZENESW);
	m_hcursor[kMouseCursorPointer] = LoadCursor(NULL, IDC_HAND);
	m_hcursor[kMouseCursorDrag] = LoadCursor(NULL, IDC_SIZEALL);
	m_hcursor[kMouseCursorText] = LoadCursor(NULL, IDC_IBEAM);
	m_hcursor[kMouseCursorBlock] = LoadCursor(NULL, IDC_NO);
	m_hcursor[kMouseCursorZoom] = LoadCursor(st_hinstance, MAKEINTRESOURCE(8));



	//window msgs

	MemClear(kMsgMap, sizeof(kMsgMap));

	kMsgMap[WM_NCLBUTTONDOWN] = kNCLBUTTONDOWN;
	kMsgMap[WM_MOUSELEAVE] = kMOUSELEAVE;
	kMsgMap[WM_MOUSEMOVE] = kMOUSEMOVE;
	kMsgMap[WM_LBUTTONDOWN] = kLBUTTONDOWN;
	kMsgMap[WM_RBUTTONDOWN] = kRBUTTONDOWN;
	kMsgMap[WM_LBUTTONDBLCLK] = kLBUTTONDBLCLK;
	kMsgMap[WM_RBUTTONDBLCLK] = kRBUTTONDBLCLK;
	kMsgMap[WM_LBUTTONUP] = kLBUTTONUP;
	kMsgMap[WM_RBUTTONUP] = kRBUTTONUP;
	kMsgMap[WM_MOUSEWHEEL] = kMOUSEWHEEL_Y;
	kMsgMap[WM_MOUSEHWHEEL] = kMOUSEWHEEL_X;

	kMsgMap[WM_KEYDOWN] = kKEYDOWN;
	kMsgMap[WM_KEYUP] = kKEYUP;
	kMsgMap[WM_SYSKEYUP] = kSYSKEYUP;
	kMsgMap[WM_SYSKEYDOWN] = kSYSKEYDOWN;
	kMsgMap[WM_CHAR] = kCHAR;
	kMsgMap[WM_SYSCHAR] = kSYSKEYDOWN;

	kMsgMap[WM_SETFOCUS] = kSETFOCUS;
	kMsgMap[WM_KILLFOCUS] = kKILLFOCUS;
	kMsgMap[WM_GETMINMAXINFO] = kGETMINMAXINFO;
	kMsgMap[WM_WINDOWPOSCHANGED] = kWINDOWPOSCHANGED;
	kMsgMap[WM_SIZING] = kSIZING;
	kMsgMap[WM_EXITSIZEMOVE] = kEXITSIZEMOVE;
	kMsgMap[WM_PAINT] = kPAINT;
	kMsgMap[WM_ERASEBKGND] = kERASEBKGND;
	kMsgMap[WM_CLOSE] = kCLOSE;

	kMsgMap[WM_IME_STARTCOMPOSITION] = kIME_STARTCOMPOSITION;
	kMsgMap[WM_IME_ENDCOMPOSITION] = kIME_ENDCOMPOSITION;
	kMsgMap[WM_IME_COMPOSITION] = kIME_COMPOSITION;
	kMsgMap[WM_IME_KEYLAST] = kIME_KEYLAST;

	kMsgMap[WM_DESTROY] = kDESTROY;

	kMsgMap[WM_SYSCOMMAND] = kSYSCOMMAND;



	//plugin hooks

	m_hooks[0] = 0;

	m_hooks[1] = 0;



	//hidden window for clock

	m_hwnd_hidden = CreateWindowW(kHiddenWindowClass.data, L"", WS_POPUP, 0, 0, 128, 128, NULL, NULL, st_hinstance, NULL);

	StartClock(15);
#endif
}

Reflex::System::Win::Library::~Library()
{
#if REFLEX_INCLUDE_UI
	StopClock();

	DestroyWindow(m_hwnd_hidden);

	UnregisterClass(kVisibleWindowClass.data, st_hinstance);

	UnregisterClass(kHiddenWindowClass.data, st_hinstance);
#else
	OleUninitialize();
#endif

	if (m_internet_handle) (*m_internet_deinit)(m_internet_handle);
}

#if REFLEX_INCLUDE_UI
void Reflex::System::Win::Library::StartClock(UInt ms)
{
	SetTimer(m_hwnd_hidden, reinterpret_cast<UIntNative>(this), ms, 0);		//33 ~ 30hz, 20 = 50hz, 16 = 60hz
}

void Reflex::System::Win::Library::StopClock()
{
	KillTimer(m_hwnd_hidden, reinterpret_cast<UIntNative>(this));
}

LRESULT CALLBACK Reflex::System::Win::Library::WinProc(HWND hwnd, UINT msg, WPARAM vk, LPARAM lparam)
{
	Library::st_internet_ready = true;

	switch (msg & g_launch_workaround)
	{
	case WM_TIMER:
		{
			globals->m_signals[kNotificationClock].Notify();

			return 1;
		}

	//case WM_SYSCOMMAND:
	//	if (vk == SC_KEYMENU) return 0;

	case WM_ERASEBKGND:
		return 1;

	case WM_DEVICECHANGE:
		REFLEX_ASSERT_MAINTHREAD("System::Win::Library::WinProc");
		globals->m_signals[kNotificationChangeDevices].Notify();
		return 1;

	case WM_DISPLAYCHANGE: {
		REFLEX_ASSERT_MAINTHREAD("System::Win::Library::WinProc");
		globals->m_screens.Clear();
		globals->m_signals[kNotificationChangeDisplays].Notify();
		return 1;
	}

	case WM_SETTINGCHANGE:
		// Theme changed (light/dark mode)
		if (lparam && wcscmp((LPCWSTR)lparam, L"ImmersiveColorSet") == 0) 
		{
			globals->m_signals[kNotificationChangeDisplays].Notify();
		}
		break;
	};

	return DefWindowProc(hwnd, msg, vk, lparam);
}

void Reflex::System::Win::Library::InitKeyCodes()
{
	MemClear(m_vk_to_keycode_modifier, sizeof(m_vk_to_keycode_modifier));

	MemClear(m_keycode_to_vk, sizeof(m_keycode_to_vk));

	BindModifierKey(VK_SHIFT, kModifierKeyShift);
	BindModifierKey(VK_CONTROL, kModifierKeyCtrl);
	BindModifierKey(VK_MENU, kModifierKeyAlt);
	BindModifierKey(VK_LWIN, kModifierKeySystem);
	//BindModifierKey(VK_RMENU, kModifierKeyAltGr);

	BindKeyCode(VK_ESCAPE, kKeyCodeEscape);
	BindKeyCode(VK_RETURN, kKeyCodeEnter);
	BindKeyCode(VK_SPACE, kKeyCodeSpace);
	BindKeyCode(VK_TAB, kKeyCodeTab);
	BindKeyCode(VK_BACK, kKeyCodeBackspace);

	BindKeyCode(VK_INSERT, kKeyCodeInsert);
	BindKeyCode(VK_DELETE, kKeyCodeDelete);
	BindKeyCode(VK_HOME, kKeyCodeHome);
	BindKeyCode(VK_END, kKeyCodeEnd);
	BindKeyCode(VK_PRIOR, kKeyCodePageUp);
	BindKeyCode(VK_NEXT, kKeyCodePageDown);

	BindKeyCode(VK_UP, kKeyCodeUp);
	BindKeyCode(VK_DOWN, kKeyCodeDown);
	BindKeyCode(VK_LEFT, kKeyCodeLeft);
	BindKeyCode(VK_RIGHT, kKeyCodeRight);

	BindKeyCode(0x31, kKeyCode1);
	BindKeyCode(0x32, kKeyCode2);
	BindKeyCode(0x33, kKeyCode3);
	BindKeyCode(0x34, kKeyCode4);
	BindKeyCode(0x35, kKeyCode5);
	BindKeyCode(0x36, kKeyCode6);
	BindKeyCode(0x37, kKeyCode7);
	BindKeyCode(0x38, kKeyCode8);
	BindKeyCode(0x39, kKeyCode9);
	BindKeyCode(0x30, kKeyCode0);

	BindKeyCode(VK_NUMPAD0, kKeyCode0);
	BindKeyCode(VK_NUMPAD1, kKeyCode1);
	BindKeyCode(VK_NUMPAD2, kKeyCode2);
	BindKeyCode(VK_NUMPAD3, kKeyCode3);
	BindKeyCode(VK_NUMPAD4, kKeyCode4);
	BindKeyCode(VK_NUMPAD5, kKeyCode5);
	BindKeyCode(VK_NUMPAD6, kKeyCode6);
	BindKeyCode(VK_NUMPAD7, kKeyCode7);
	BindKeyCode(VK_NUMPAD8, kKeyCode8);
	BindKeyCode(VK_NUMPAD9, kKeyCode9);

	BindKeyCode(VK_F1, kKeyCodeF1);
	BindKeyCode(VK_F2, kKeyCodeF2);
	BindKeyCode(VK_F3, kKeyCodeF3);
	BindKeyCode(VK_F4, kKeyCodeF4);
	BindKeyCode(VK_F5, kKeyCodeF5);
	BindKeyCode(VK_F6, kKeyCodeF6);
	BindKeyCode(VK_F7, kKeyCodeF7);
	BindKeyCode(VK_F8, kKeyCodeF8);
	BindKeyCode(VK_F9, kKeyCodeF9);
	BindKeyCode(VK_F10, kKeyCodeF10);
	BindKeyCode(VK_F11, kKeyCodeF11);
	BindKeyCode(VK_F12, kKeyCodeF12);

	BindKeyCode(VK_DIVIDE, kKeyCodeNumericDivide);
	BindKeyCode(VK_MULTIPLY, kKeyCodeNumericMultiply);
	BindKeyCode(VK_SUBTRACT, kKeyCodeNumericMinus);
	BindKeyCode(VK_ADD, kKeyCodeNumericPlus);

	UInt n = ('Z' - 'A') + 1;

	REFLEX_LOOP(idx, n) BindKeyCode(65 + idx, KeyCode(kKeyCodeA + idx));

	BindKeyCode(189, kKeyCodeMinus);
	BindKeyCode(187, kKeyCodePlus);
	BindKeyCode(VK_OEM_2, kKeyCodeSlash);

	BindKeyCode(219, kKeyCodeBracketOpen);
	BindKeyCode(221, kKeyCodeBracketClose);
}

REFLEX_INLINE void Reflex::System::Win::Library::BindKeyCode(UInt32 vk, KeyCode keycode)
{
	REFLEX_ASSERT(keycode < kNumKeyCode);

	REFLEX_ASSERT(vk < 256);

	m_vk_to_keycode_modifier[vk].a = keycode;

	m_keycode_to_vk[keycode] = UInt16(vk);
}

REFLEX_INLINE void Reflex::System::Win::Library::BindModifierKey(UInt32 vk, ModifierKeys flag)
{
	REFLEX_ASSERT(vk < 256);

	m_vk_to_keycode_modifier[vk].b = flag;
}
#endif

void Reflex::System::Common::SetInstanceHandle(void * system_instance)
{
	Win::Library::st_hinstance = reinterpret_cast<HINSTANCE>(system_instance);
}

const Reflex::System::Platform Reflex::System::kPlatform = Reflex::System::kPlatformWindows;

Reflex::TRef <Reflex::Object> Reflex::System::CreateListener(Notification id, void * client, void(*callback)(void*))
{
#ifdef REFLEX_SYSTEM_CONSOLE
	return {};
#else
	return Win::globals->m_signals[id].Create(client, callback);
#endif
}

Reflex::UInt64 Reflex::System::GetSystemID()
{
	return Win::globals->m_systemid;
}

Reflex::CString Reflex::System::GetOperatingSystemVersion()
{
	Tuple <const char*, UInt64, DWORD> version[3] = { {"CurrentMajorVersionNumber", 0,4}, {"CurrentMinorVersionNumber", 0,4}, {"CurrentBuildNumber",0,8} };

	HKEY hKey;

	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		for (auto & i : version)
		{
			RegQueryValueExA(hKey, i.a, NULL, NULL, (LPBYTE)&i.b, &i.c);
		}

		RegCloseKey(hKey);
	}

	const char * build = &Reinterpret<char>(version[2].b);

	return Join(ToCString(UInt32(version[0].b)), '.', ToCString(UInt32(version[1].b)), '.', build);
}

Reflex::UInt32 Reflex::System::GetNumProcessor()
{
	SYSTEM_INFO system_info;

	GetSystemInfo(&system_info);

	return system_info.dwNumberOfProcessors;
}

Reflex::UInt32 Reflex::System::GetProcessID()
{
	return UInt32(::GetCurrentProcessId());
}

Reflex::UInt64 Reflex::System::GetTime()
{
	return time(0);
}

Reflex::Float64 Reflex::System::GetElapsedTime()
{
	LARGE_INTEGER timerdelta;

	QueryPerformanceCounter(&timerdelta);

	return Float64(timerdelta.QuadPart) * Win::Library::st_timermult;
}

Reflex::WString Reflex::System::GetPath(Path path)
{
	auto GetPathImpl = [](int csidl, WChar * buffer)
	{
		SHGetFolderPath(0, csidl, 0, 0, buffer);

		WString output(buffer);
		
		File::Detail::CorrectStrokes(output);

		output.Push(kPathDelimiter);

		return output;
	};

	WChar buffer[MAX_PATH];

	switch (path)
	{
	case kPathDesktop:
		return GetPathImpl(CSIDL_DESKTOPDIRECTORY, buffer);

	case kPathApplicationData:
		return GetPathImpl(CSIDL_COMMON_APPDATA, buffer);

	case kPathUserData:
		return GetPathImpl(CSIDL_APPDATA, buffer);

	case kPathUserDocuments:
		return GetPathImpl(CSIDL_MYDOCUMENTS, buffer);

	case kPathTemp:
	{
		GetTempPathW(MAX_PATH, buffer);

		WString output(buffer + 0);
		
		File::Detail::CorrectStrokes(output);

		return output;
	}
	};

	return {};
}

Reflex::WString Reflex::System::GetExecutablePath()
{
	WChar buffer[MAX_PATH] = {};

	auto length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

	WString output(WString::View(buffer, length));

	File::Detail::CorrectStrokes(output);

	return output;
}

bool Reflex::System::Exists(const WString & path)
{
	return GetFileAttributesW(path.GetData()) != INVALID_FILE_ATTRIBUTES;
}

bool Reflex::System::IsDirectory(const WString & path)
{
	UInt32 flags = GetFileAttributesW(path.GetData());

	if (flags == INVALID_FILE_ATTRIBUTES) return false;

	return Reflex::True(flags & FILE_ATTRIBUTE_DIRECTORY);
}

bool Reflex::System::GetFileAttributesEx(const WString & path, Tuple <UInt64,UInt64,UInt64,UInt64> & info)	//size, created, modified, accessed
{
	WIN32_FILE_ATTRIBUTE_DATA inf;

	if (GetFileAttributesExW(path.GetData(), GetFileExInfoStandard, &inf))
	{
		//auto directory = inf.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

		info.a = Win::GetUInt64(inf.nFileSizeLow, inf.nFileSizeHigh);

		info.b = Win::ToTimestamp(inf.ftCreationTime);

		info.c = Win::ToTimestamp(inf.ftLastWriteTime);

		info.d = Win::ToTimestamp(inf.ftLastAccessTime);

		REFLEX_ASSERT(info.c);

		return true;
	}

	return false;
}

bool Reflex::System::SetFileTime(const WString & path, UInt64 timestamp)
{
	//FILE_SHARE_READ | FILE_SHARE_WRITE
	HANDLE handle = CreateFileW(path.GetData(), GENERIC_WRITE, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle != INVALID_HANDLE_VALUE)
	{
		timestamp = (timestamp + 11644473600ul) * 10000000ul;

		REFLEX_STATIC_ASSERT(sizeof(FILETIME) == sizeof(UInt64));

		bool result = ::SetFileTime(handle, 0, 0, &Reinterpret<FILETIME>(timestamp));

		CloseHandle(handle);

		return result;
	}

	return false;
}

bool Reflex::System::Rename(const WString & from, const WString & to)
{
	if (from == to) return true;
	
	if (MoveFileExW(from.GetData(), to.GetData(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) == TRUE)
	{
		Delete(from);	//former Win32 bug TODO REVIEW, may not exist now since switch to MOVEFILE_REPLACE_EXISTING

		return true;
	}

	return false;
}

bool Reflex::System::Delete(const WString & path)
{
	if (DeleteFileW(path.GetData()))
	{
		return true;
	}
	else
	{
		return True(RemoveDirectoryW(path.GetData()));
	}

	//recycle
	//{
	//	SHFILEOPSTRUCT fileop;

	//	Std::MemClear(&fileop, sizeof(SHFILEOPSTRUCT));

	//	Std::WString file(path);
	//
	//	file.Append(WChar(0));

	//	fileop.wFunc = FO_DELETE;
	//
	//	fileop.pFrom = file.GetData();
	//
	//	fileop.fFlags = FOF_ALLOWUNDO | FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;

	//	return SHFileOperation(&fileop) == 0;
	//}
}

bool Reflex::System::MakeDirectory(const WString & path)
{
	WString temp(path);
	
	File::Detail::RemoveTrailingStroke(temp);

	return CreateDirectoryW(temp.GetData(), 0) != 0;
}

bool Reflex::System::Open(const WString & path)
{
	Win::Library::g_launch_workaround = 0;

	auto rtn = reinterpret_cast<long long>(ShellExecuteW(0, L"open", path.GetData(), 0 /*args.GetData()*/, NULL, SW_SHOWNORMAL));

	Win::Library::g_launch_workaround = kMaxUInt32;

	return rtn  > 32;
}

bool Reflex::System::Share(const ArrayView<WString> & urls, const WString & desc)
{
	//TODO support windows "share experience"

	if (urls)
	{
		return Open(urls.GetFirst());
	}

	return false;
}

void Reflex::System::DisableCrashReporter()
{
	//SetUnhandledExceptionFilter(&NullCrashHandler);

	SetErrorMode(SEM_NOGPFAULTERRORBOX);
}

Reflex::WString Reflex::System::GetCurrentDirectory()
{
	WChar buffer[MAX_PATH];

	GetCurrentDirectoryW(MAX_PATH, buffer);

	WString rtn(buffer + 0);
	
	File::Detail::CorrectStrokes(rtn);

	rtn.Push(kPathDelimiter);

	return rtn;
}

bool Reflex::System::SetCurrentDirectory(const WString & path)
{
	return SetCurrentDirectoryW(path.GetData()) != 0;
}

bool Reflex::System::IsAbsolutePath(const WString::View & path)
{
	if (path.size > 2)
	{
		return (path[1] == L':' && path[2] == kPathDelimiter);
	}

	return false;
}
