#pragma once

#include "sdk.h"
#include "../common/notification.h"




//
//declarations

REFLEX_NS(Reflex::System::OSX)

class Globals;

extern Reflex::Detail::Module::Member <Globals> globals;

REFLEX_END




//
//OSX::Globals

class Reflex::System::OSX::Globals
{
public:

	Globals();

	~Globals();


	void InitKeyCodes();

	Pair <KeyCode,bool> TranslateKey(unichar keycode);

	void ProcessKey(Window::Client & client, KeyCode keycode, bool value, bool repeat);

	void StartTimer(UInt32 ms);

	void StopTimer();

	void CollectScreenInfo();

	static void SetRefreshRate(UInt32 ms);

	static void OnTimer(CFRunLoopTimerRef timer, void * info);

	template <class TYPE> static bool OpenFileDialog(TYPE * dialog, const WString & directory, const WString & filename, const Array <WString> & filters);



	WString m_arguments;


	UInt64 m_systemid;

	Float64 m_highrestimefactor;

	int m_threadpriorities[3];

	bool m_debugger;


	ObjCRef <NSArray<NSScreen*>*> m_nsscreens;

	Int32 m_pixeldensity;

	Int32 m_desktopheight;


	bool m_keystate[kNumKeyCode];

	bool m_keytrap[kNumKeyCode];

	UInt8 m_modifier_flags;


	NSView * m_capturewindow;

	UInt8 m_mousebuttonstate;


	CFRunLoopTimerRef m_timer;

	Common::Signal m_signals[kNumNotification];


	Tuple <NSSearchPathDirectory,NSSearchPathDomainMask> m_pathids[kNumPath];

	Sequence < WChar, Pair<KeyCode,bool> > m_unichar2keycodes;

};
