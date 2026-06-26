#pragma once

#include "[require].h"
#include "../common/notification.h"




//
//declarations

REFLEX_NS(Reflex::System::Win)

struct Library;

#if REFLEX_INCLUDE_UI
class Window;
class PluginWindow;

enum WindowMsg
{
	kDEFAULT,

	kNCLBUTTONDOWN,

	kMOUSELEAVE,
	kMOUSEMOVE,
	kLBUTTONDOWN,
	kRBUTTONDOWN,
	kLBUTTONDBLCLK,
	kRBUTTONDBLCLK,
	kLBUTTONUP,
	kRBUTTONUP,
	kMOUSEWHEEL_X,
	kMOUSEWHEEL_Y,

	kSYSKEYDOWN,
	kKEYDOWN,
	kSYSKEYUP,
	kKEYUP,
	kCHAR,

	kSETFOCUS,
	kKILLFOCUS,

	kGETMINMAXINFO,
	kWINDOWPOSCHANGED,
	kSIZING,
	kEXITSIZEMOVE,

	kPAINT,
	kERASEBKGND,

	kCLOSE,

	kIME_STARTCOMPOSITION,
	kIME_ENDCOMPOSITION,
	kIME_COMPOSITION,
	kIME_KEYLAST,

	kDESTROY,

	kSYSCOMMAND,

	kNumWindowMsg,
};
#endif

extern HANDLE st_console;

struct Library
{
	//lifetime

	Library();

	~Library();



#if REFLEX_INCLUDE_UI
	void InitKeyCodes();

	void BindKeyCode(UInt32 vk, KeyCode keycode);

	void BindModifierKey(UInt32 vk, ModifierKeys modifier);

	void StartClock(UInt ms);

	void StopClock();

	static LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#endif


	//shared runtime state

	UInt64 m_systemid;

	const volatile bool m_debugger;

	WChar m_application[MAX_PATH];

	WString m_systempaths[kNumPath];

#ifndef REFLEX_SYSTEM_CONSOLE
	Common::Signal m_signals[kNumNotification];
#endif



	//time

	SYSTEMTIME m_current;

	FILETIME m_reference_ft;

	static inline /*const*/ Float64 st_timermult = 0.0;



	//internet

	BOOL (__stdcall *m_internet_deinit)(HINTERNET);

	HINTERNET m_internet_handle;

	static volatile bool st_internet_ready;

	static volatile decltype (&GetFileAttributesExW) GetFileAttributesWindowsBugWorkaround;



	//process / module state

	static HINSTANCE st_hinstance;

	static inline UInt g_launch_workaround = kMaxUInt32;


#if REFLEX_INCLUDE_UI
	HWND m_hwnd_hidden;

	Array <iRect> m_screens;

	Int32 m_maxpixeldensity;

	bool m_enable_truefullscreen;

	HCURSOR m_hcursor[System::kNumMouseCursor];

	Window * m_mouseover;

	Pair <KeyCode,ModifierKeys> m_vk_to_keycode_modifier[256];

	UInt16 m_keycode_to_vk[kNumKeyCode];

	Array <PluginWindow*> m_plugin_windows;

	HHOOK m_hooks[2];

	UInt8 m_kbs[256];

	static UInt8 st_modifier_key_flags;

	static UInt8 kMsgMap[676];

	static constexpr WString::View kVisibleWindowClass = L"Reflex::System::Window";
#endif
};

inline Reflex::Detail::Module::Member <Library> globals(Common::g_module);

REFLEX_END
