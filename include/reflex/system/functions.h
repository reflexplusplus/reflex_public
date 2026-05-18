#pragma once

#include "defines.h"




//
//Secondary API

namespace Reflex::System
{

	//info

	CString GetOperatingSystemVersion();

	UInt64 GetSystemID();

	UInt32 GetNumProcessor();

	UInt64 GetTime();

	Float64 GetElapsedTime();



	//notifications

	TRef <Object> CreateListener(Notification id, void * client, FunctionPointer <void(void*)> callback);



	//file

	bool SetFileTime(const WString & path, UInt64 file_time);

	bool GetFileAttributesEx(const WString & path, Tuple <UInt64,UInt64,UInt64,UInt64> & size_created_modified_accessed);	//size, created, modified, accessed

	bool GetFileAttributes(const WString & path, Tuple <UInt64,UInt64> & size_time);	//size, modified time


	bool Rename(const WString & from, const WString & to);

	bool Delete(const WString & path);

	bool MakeDirectory(const WString & path);


	bool Exists(const WString & path);

	bool IsDirectory(const WString & path);


	bool IsAbsolutePath(const WString::View & path);


	WString GetPath(Path path);


	WString GetCurrentDirectory();

	bool SetCurrentDirectory(const WString & path);


	bool Open(const WString & path);

	bool Share(const ArrayView <WString> & paths, const WString & extra_text = {});



	//ui

	Int32 GetMaxPixelDensity();

	Array <iRect> GetScreens();


	CString GetLanguage();

	bool IsDarkTheme();

	Float GetFontScale();


	UInt8 GetModifierKeys();	//TODO move to mouse and key events


	void SetClipboard(const WString & text);

	WString GetClipboard();



	//debug

	void DisableCrashReporter();

	bool IsDebuggerPresent();

	void DebugLog(bool brk, const char * msg);

	void Log(const char * string);

	extern FunctionPointer <void(const char * msg)> g_on_debug_break;

}




//
//impl

inline bool Reflex::System::GetFileAttributes(const WString & path, Tuple <UInt64,UInt64> & size_time)
{
	Tuple <UInt64,UInt64,UInt64,UInt64> attributes;

	auto r = GetFileAttributesEx(path, attributes);

	size_time = { attributes.a, attributes.c };

	return r;
}

#if (!REFLEX_DEBUG)
inline void Reflex::System::DebugLog(bool brk, const char * msg) {}
#endif
