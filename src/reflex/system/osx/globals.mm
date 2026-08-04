#include "globals.h"
#include "window.h"
#include "../common/apple_utils.hpp"
#include "../../../../include/reflex/file/functions/path.h"
#include <mach-o/dyld.h>




//
//

const Reflex::System::Platform Reflex::System::kPlatform = Reflex::System::kPlatformMacOS;

Reflex::Detail::Module::Member <Reflex::System::OSX::Globals> Reflex::System::OSX::globals(Common::g_module);

bool Reflex::System::OSX::g_enableretina = true;

Reflex::System::Common::UTF8 Reflex::System::OSX::ResolveBundlePath(const WString & path)
{
	typedef FunctionPointer <WString(const WString&,const WString::View&)> FunctionPointer;

	FunctionPointer MakeOsxPath = [](const WString & folder, const WString::View & filename)
	{
		return WString(File::RemoveExtension(filename));
	};

	FunctionPointer MakeMeldaProductionsPath = [](const WString & folder, const WString::View & filename)
	{
		return Join(File::RemoveExtension(filename), L".bin");
	};

	FunctionPointer MakeIzotopePath = [](const WString & folder, const WString::View & filename)
	{
		return WString(L"PluginHooksVST");
	};

	FunctionPointer MakeFallbackPath = [](const WString & folder, const WString::View &)
	{
		auto itr = AutoRelease(DirectoryIterator::Create(folder, true));

		DirectoryIterator::Item item;

		while (itr->GetNext(item))
		{
			if (!item.is_directory) return item.filename;
		}

		return WString();
	};

	WString folder = Join(path, kPathDelimiter, L"Contents", kPathDelimiter, L"MacOS", kPathDelimiter);

	auto filename = File::SplitFilename(path).b;

	const FunctionPointer fns[] = { MakeOsxPath, MakeMeldaProductionsPath, MakeIzotopePath, MakeFallbackPath };

	for (auto & i : fns)
	{
		WString t = Join(folder, (i)(folder, filename));

		auto utf8 = Common::ToUTF8(t);

		if (Common::POSIX::Exists(utf8)) return utf8;
	}

	auto frameworkpath = Common::ToUTF8(Join(path, kPathDelimiter, File::SplitExtension(filename).a));

	if (Common::POSIX::Exists(frameworkpath))
	{
		return frameworkpath;
	}
	else
	{
		return Common::ToUTF8(path);
	}
}

void Reflex::System::Common::SetInstanceHandle(void * unused)
{
}

Reflex::System::OSX::Globals::Globals()
	: m_systemid(0)
	, m_debugger([]()
	{
		//https://developer.apple.com/library/archive/qa/qa1361/_index.html

		struct kinfo_proc info;

		info.kp_proc.p_flag = 0;

		int mib[4];
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PID;
		mib[3] = getpid();

		size_t size = sizeof(info);

		sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

		return (info.kp_proc.p_flag & P_TRACED) != 0;
	}())
	, m_modifier_flags(0)
	, m_mousebuttonstate(0)
	, m_timer(0)
{
	//thread

	pthread_attr_t attributes;

	pthread_attr_init(&attributes);

	sched_param param;

	pthread_attr_getschedparam(&attributes, &param);

	m_threadpriorities[0] = sched_get_priority_min(SCHED_FIFO);

	m_threadpriorities[1] = param.sched_priority;

	m_threadpriorities[2] = sched_get_priority_max(SCHED_FIFO);



	//systemid

	char buffer[40] = {0};

	const char * root = "/";

	DASessionRef session = DASessionCreate(kCFAllocatorDefault);

	CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8*)root, 1, true);

  	DADiskRef diskref = DADiskCreateFromVolumePath(kCFAllocatorDefault, session, url);

    if (CFDictionaryRef dictionary = DADiskCopyDescription(diskref))
    {
		CFTypeRef value = CFDictionaryGetValue(dictionary, kDADiskDescriptionVolumeUUIDKey);

        CFStringRef string = CFStringCreateWithFormat(kCFAllocatorDefault, 0, CFSTR("%@"), value);

		CFStringGetCString(string, buffer, 40, kCFStringEncodingMacRoman);

		auto view = ToView((const char*)buffer);
		
        m_systemid = Reflex::Detail::MakeHash<UInt64>(view);

		CFRelease(string);

		CFRelease(dictionary);
	}

	CFRelease(diskref);

	CFRelease(url);

	CFRelease(session);



	//paths

	m_pathids[kPathTemp] = {NSCachesDirectory, NSUserDomainMask};
	m_pathids[kPathApplicationData] = {NSApplicationSupportDirectory, NSLocalDomainMask};
	m_pathids[kPathUserData] = { NSApplicationSupportDirectory, NSUserDomainMask};
	m_pathids[kPathUserDocuments] = { NSDocumentDirectory, NSUserDomainMask };
	m_pathids[kPathDesktop] = { NSDesktopDirectory, NSUserDomainMask};



	//highres time

	m_highrestimefactor = 1.0;

	mach_timebase_info_data_t info;

	kern_return_t err = mach_timebase_info( &info );

	if(err == KERN_SUCCESS) m_highrestimefactor = ((double(info.numer) / double(info.denom)) / 1000000.0);



	//key

	MemClear(m_keystate, kNumKeyCode * sizeof(bool));

	MemClear(m_keytrap, kNumKeyCode * sizeof(bool));

	InitKeyCodes();



	//screens info

	CGDisplayRegisterReconfigurationCallback([](CGDirectDisplayID display, uint32_t flags, void * client)
	{
		globals->CollectScreenInfo();

		globals->m_signals[kNotificationChangeDisplays].Notify();
	}, 0);

	CollectScreenInfo();



	//start main timer

	StartTimer(15);
}

Reflex::System::OSX::Globals::~Globals()
{
	StopTimer();
}

void Reflex::System::OSX::Globals::InitKeyCodes()
{
	auto Register = [](Sequence < WChar, Pair<KeyCode,bool> > & keycodes, WChar w, KeyCode keycode, bool printable)
	{
		keycodes.Insert(w, {keycode, printable});
	};


	//printable

	Register(m_unichar2keycodes, '0', kKeyCode0, true);

	UInt32 keycode = kKeyCode1;
	for (WChar idx = '1'; idx <= '9'; ++idx) Register(m_unichar2keycodes, idx, KeyCode(keycode++), true);

	keycode = kKeyCodeA;
	for (WChar idx = 'a'; idx <= 'z'; ++idx) Register(m_unichar2keycodes, idx, KeyCode(keycode++), true);

	keycode = kKeyCodeA;
	for (WChar idx = 'A'; idx <= 'Z'; ++idx) Register(m_unichar2keycodes, idx, KeyCode(keycode++), true);

	Register(m_unichar2keycodes, ' ', kKeyCodeSpace, true);

	Register(m_unichar2keycodes, '+', kKeyCodeNumericPlus, true);
	Register(m_unichar2keycodes, '-', kKeyCodeNumericMinus, true);
	Register(m_unichar2keycodes, '/', kKeyCodeNumericDivide, true);
	Register(m_unichar2keycodes, '/', kKeyCodeSlash, true);
	Register(m_unichar2keycodes, '*', kKeyCodeNumericMultiply, true);

	Register(m_unichar2keycodes, '=', kKeyCodePlus, true);

	Register(m_unichar2keycodes, '[', kKeyCodeBracketOpen, true);
	Register(m_unichar2keycodes, ']', kKeyCodeBracketClose, true);
	Register(m_unichar2keycodes, '{', kKeyCodeBracketOpen, true);
	Register(m_unichar2keycodes, '}', kKeyCodeBracketClose, true);


	//non printable

	REFLEX_LOOP_TYPE (WChar, idx, 32) Register(m_unichar2keycodes, idx, kKeyCodeNull, false);

	m_unichar2keycodes.Acquire(3) = {kKeyCodeEnter, false};	//numeric enter
	m_unichar2keycodes.Acquire(9) = {kKeyCodeTab, false};
	m_unichar2keycodes.Acquire(25) = {kKeyCodeTab, false};//shift+tab
	m_unichar2keycodes.Acquire(13) = {kKeyCodeEnter, false};
	m_unichar2keycodes.Acquire(27) = {kKeyCodeEscape, false};

	Register(m_unichar2keycodes, 127, kKeyCodeBackspace, false);

	REFLEX_LOOP (idx, 12) Register(m_unichar2keycodes, 63236 + idx, KeyCode(kKeyCodeF1 + idx), false);

	Register(m_unichar2keycodes, 63232, kKeyCodeUp, false);
	Register(m_unichar2keycodes, 63233, kKeyCodeDown, false);
	Register(m_unichar2keycodes, 63234, kKeyCodeLeft, false);
	Register(m_unichar2keycodes, 63235, kKeyCodeRight, false);

	Register(m_unichar2keycodes, 63273, kKeyCodeHome, false);
	Register(m_unichar2keycodes, 63275, kKeyCodeEnd, false);
	Register(m_unichar2keycodes, 63276, kKeyCodePageUp, false);
	Register(m_unichar2keycodes, 63277, kKeyCodePageDown, false);
	Register(m_unichar2keycodes, 63302, kKeyCodeInsert, false);
	Register(m_unichar2keycodes, 63272, kKeyCodeDelete, false);

	Register(m_unichar2keycodes, 63248, kKeyCodeNull, false);//print screen
	Register(m_unichar2keycodes, 63249, kKeyCodeNull, false);//num lock
}

Reflex::Pair <Reflex::System::KeyCode,bool> Reflex::System::OSX::Globals::TranslateKey(unichar code)
{
	Pair <KeyCode,bool> null = {kKeyCodeNull, true};

	return *m_unichar2keycodes.SearchValue(code, &null);
}

void Reflex::System::OSX::Globals::ProcessKey(Window::Client & client, KeyCode keycode, bool value, bool repeat)
{
	bool & keystate = m_keystate[keycode];

	if (keystate != value)
	{
		keystate = value;

		if (keystate)
		{
			client.OnKeyPress(keycode, repeat);
		}
		else
		{
			client.OnKeyRelease(keycode);
		}
	}
}

void Reflex::System::OSX::Globals::CollectScreenInfo()
{
	Int32 & density = m_pixeldensity;

	Int32 & height = m_desktopheight;

	m_nsscreens = MakeObjCRef([NSScreen screens]);

	density = 2;

	height = 0;

	if (NSAppKitVersionNumber >= NSAppKitVersionNumber10_7)
	{
		REFLEX_LOOP(idx, UInt([m_nsscreens count]))
		{
			NSScreen * screen = [m_nsscreens objectAtIndex:idx];

			density = Min(density, Truncate([screen backingScaleFactor]));

			height = Max(height, Truncate([screen frame].size.height));
		}
	}
}

void Reflex::System::OSX::Globals::SetRefreshRate(UInt32 ms)
{
	globals->StopTimer();

	globals->StartTimer(ms);
}

void Reflex::System::OSX::Globals::StartTimer(UInt32 ms)
{
	REFLEX_ASSERT(m_timer == 0);

	CFRunLoopTimerContext context;

	context.version = 0;
	context.info = this;
	context.retain = 0;
	context.release = 0;
	context.copyDescription = 0;

	m_timer = CFRunLoopTimerCreate(NULL, 0, Float64(ms) / 1000.0, 0, 0, &Globals::OnTimer, &context);

	CFRunLoopAddTimer(CFRunLoopGetMain(), m_timer, kCFRunLoopCommonModes);
}

void Reflex::System::OSX::Globals::StopTimer()
{
	REFLEX_ASSERT(m_timer != 0);

	CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), m_timer, kCFRunLoopCommonModes);

	CFRelease(m_timer);

	m_timer = 0;
}

void Reflex::System::OSX::Globals::OnTimer(CFRunLoopTimerRef timer, void * info)
{
	globals->m_signals[kNotificationClock].Notify();
}




//
//public API

Reflex::TRef <Reflex::Object> Reflex::System::CreateListener(Notification notification, void * client, void(*callback)(void*))
{
	return OSX::globals->m_signals[notification].Create(client, callback);
}

Reflex::CString Reflex::System::GetOperatingSystemVersion()
{
	//TODO remove junk (version and build)
	/*
	NSOperatingSystemVersion version = [[NSProcessInfo processInfo] operatingSystemVersion];

	NSInteger major = version.majorVersion;
	NSInteger minor = version.minorVersion;
	NSInteger patch = version.patchVersion;
	NSString *versionString = [NSString stringWithFormat:@"%ld.%ld.%ld", (long)major, (long)minor, (long)patch];
	*/

	NSProcessInfo * process_info = [NSProcessInfo processInfo];

	NSString * os_version = [process_info operatingSystemVersionString];

	return ToCStringView(os_version);
}

Reflex::UInt64 Reflex::System::GetSystemID()
{
	return OSX::globals->m_systemid;
}

Reflex::Array <Reflex::System::iRect> Reflex::System::GetScreens()
{
	auto nsscreens = OSX::globals->m_nsscreens;

	auto n = UInt([nsscreens count]);

	Array <System::iRect> rtn;

	auto ptr = Extend(rtn, n).data;

	REFLEX_LOOP(idx, n)
	{
		NSRect nsrect = [[nsscreens objectAtIndex:idx] frame];

		iRect rect = { {Truncate(nsrect.origin.x), Truncate(nsrect.origin.y)}, {Truncate(nsrect.size.width), Truncate(nsrect.size.height)} };

		*ptr++ = rect;
	}

	return rtn;
}

Reflex::Int32 Reflex::System::GetMaxPixelDensity()
{
	return OSX::g_enableretina ? OSX::globals->m_pixeldensity : 1;
}

bool Reflex::System::IsDarkTheme()
{
	return [NSApp.effectiveAppearance.name containsString:@"Dark"];
}

Reflex::Float Reflex::System::GetFontScale()
{
	return NSFont.systemFontSize / 13.0f;	//default font size is 13
}

Reflex::UInt8 Reflex::System::GetModifierKeys()
{
	return OSX::globals->m_modifier_flags;
}

void Reflex::System::SetClipboard(const WString & text)
{
	NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];

	[pasteboard clearContents];

	auto stringref = OSX::MakeNSStringRef(text);

	[pasteboard setString:stringref forType:NSPasteboardTypeString];
}

Reflex::WString Reflex::System::GetClipboard()
{
	NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];

	NSString * nsstring = [pasteboard stringForType:NSPasteboardTypeString];

	return ToWString(nsstring);
}

Reflex::WString Reflex::System::GetPath(Path path)
{
	WChar buffer[256] = {0};

	auto & ids = OSX::globals->m_pathids[path];

	NSString * nsstring = [NSSearchPathForDirectoriesInDomains(ids.a, ids.b, YES) objectAtIndex:0];

	UInt length = OSX::NSStringToWChar(nsstring, buffer, 254);

	buffer[length] = '/';

	return WString::View(buffer, length + 1);
}

Reflex::WString Reflex::System::GetExecutablePath()
{
	WString rtn;

	uint32_t size = 0;

	_NSGetExecutablePath(nullptr, &size);

	Array <UInt8> buffer(size);

	_NSGetExecutablePath(Reinterpret<char>(buffer.GetData()), &size);

	if (size)
	{
		buffer.Pop();

		Data::DecodeUTF8(rtn, buffer);
	}

	return rtn;
}

bool Reflex::System::Open(const WString & path)
{
	constexpr WString::View kHTTP = L"http";

	bool ok = false;

	auto workspace = [NSWorkspace sharedWorkspace];

	auto string = OSX::MakeNSStringRef(path);

	if (StartsWith(path, kHTTP))
	{
		ok = [workspace openURL:[NSURL URLWithString:string]];
	}
	else
	{
		ok = [workspace openFile:string];
	}

	return ok;
}

bool Reflex::System::Share(const ArrayView<WString>& items, const WString& text)
{
	REFLEX_ASSERT_MAINTHREAD("System::Share");

	NSMutableArray * share_items = [NSMutableArray arrayWithCapacity:items.size + (text ? 1 : 0)];

	if (text)
	{
		[share_items addObject:OSX::MakeNSStringRef(text)];
	}

	for (const auto & item : items)
	{
		if (IsAbsolutePath(item))
		{
			[share_items addObject:[NSURL fileURLWithPath:OSX::MakeNSStringRef(item)]];
		}
		else
		{
			NSURL * url = [NSURL URLWithString:ToNSString(item)];

			if (!url)
			{
				return false;
			}

			[share_items addObject:url];
		}
	}

	if (!share_items.count)
	{
		return false;
	}

	NSWindow * window = NSApp.keyWindow ?: NSApp.mainWindow;

	if (!window && NSApp.windows.count)
	{
		window = NSApp.windows.firstObject;
	}

	NSView * view = window.contentView;

	if (!view)
	{
		return false;
	}

	auto picker = MakeOwnedObjCRef([[NSSharingServicePicker alloc] initWithItems:share_items]);

	[picker.Get() showRelativeToRect:view.bounds ofView:view preferredEdge:NSMinYEdge];

	return true;
}

bool Reflex::System::IsDebuggerPresent()
{
	return OSX::globals->m_debugger;
}

void Reflex::System::DisableCrashReporter()
{
	REFLEX_LOCAL(void,CrashSignalHandler)(int signal)
	{
		NSApplication * nsapp = [NSApplication sharedApplication];

		[nsapp terminate:nil];
	}
	REFLEX_END

	//kern_return_t kret = task_set_exception_ports(mach_task_self(), EXC_MASK_ALL, MACH_PORT_NULL, EXCEPTION_STATE_IDENTITY, MACHINE_THREAD_STATE);

	//Int32 signals[] = {SIGABRT, SIGILL, SIGSEGV, SIGFPE, SIGBUS, SIGPIPE, SIGKILL, SIGSTOP};

	constexpr Int32 signals[] = {SIGABRT, SIGILL, SIGSEGV, SIGFPE, SIGBUS, SIGPIPE};

	REFLEX_LOOP (idx, GetArraySize(signals)) signal(signals[idx], &CrashSignalHandler::Call);
}
