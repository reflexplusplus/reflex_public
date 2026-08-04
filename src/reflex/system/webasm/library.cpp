#include "library.h"




//
//internal

void reflexOnAnimationClock()
{
	Reflex::System::WebASM::TheLibrary::Get()->EmitClock();
}

Reflex::System::WebASM::Library::Library() 
	: m_core(Reflex::Start())
{
	jsInitSystem();
}

Reflex::System::WebASM::Library::~Library() 
{
	jsDeinitSystem();
}




//
//public API

const bool & Reflex::System::kIsAwake = Reflex::System::WebASM::TheLibrary::IsAwake();

const Reflex::System::Platform Reflex::System::kPlatform = Reflex::System::kPlatformWebAssembly;

Reflex::TRef <Reflex::Object> Reflex::System::Start(void * hinstance)
{
	return WebASM::TheLibrary::Acquire();
}

void Reflex::System::DebugLog(const char *string, bool error) 
{
	if (error)
	{
		jsConsoleError(string);
	}
	else
	{
		jsConsoleLog(string);
	}
}

void Reflex::System::DebugBreak() 
{
	jsConsoleLog("Reflex::System::DebugBreak called");
}

void Reflex::System::Log(const char *string) 
{
	jsConsoleLog(string);
}

Reflex::TRef <Reflex::Object> Reflex::System::CreateListener(Notification id, void * client, void(*callback)(void*))
{
	return WebASM::TheLibrary::Get()->CreateListener(id, client, callback);
}

Reflex::UInt64 Reflex::System::GetTime()
{
	return time(0);
}

Reflex::Float64 Reflex::System::GetElapsedTime()
{
	return jsGetPerformanceTimer();
}

Reflex::CString Reflex::System::GetOperatingSystemVersion()
{
	return "WebAssembly";
}

Reflex::UInt32 Reflex::System::GetNumProcessor()
{
	return REFLEX_NUM_VIRTUAL_PROCESSOR;
}

Reflex::UInt32 Reflex::System::GetProcessID()
{
	return 0;
}

Reflex::WString Reflex::System::GetPath(Path path)
{
	constexpr const char * kPaths[kNumPath] = { "temp/", "desktop/", "app_data/", "user_data/", "user_documents/" };

	return ToWString(ToView(kPaths[path]));
}

Reflex::WString Reflex::System::GetExecutablePath()
{
	return {};
}

Reflex::WString Reflex::System::GetCurrentDirectory()
{
	return GetPath(kPathApplicationData);
}

bool Reflex::System::SetCurrentDirectory(const WString & path)
{
	DEV_ERROR("System::SetCurrentDirectory not implemented");

	return false;
}

bool Reflex::System::GetFileAttributesEx(const WString & path, Tuple <UInt64,UInt64,UInt64,UInt64> & size_created_modified_accessed)
{
	DEV_ERROR("System::GetFileAttributesEx not implemented");

	return false;
}

bool Reflex::System::Open(const WString & cmd)
{
	DEV_ERROR("System::Open not implemented");

	return false;
}

bool Reflex::System::Share(const ArrayView<WString>&, const WString&)
{
	DEV_ERROR("System::Share not implemented");

	return false;
}
