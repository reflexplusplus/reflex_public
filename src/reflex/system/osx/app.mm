#include "globals.h"




#ifdef REFLEX_SYSTEM_AUDIO
#include "../common/instance/audioapp.cpp"
#include "coreaudio.mm"
typedef Reflex::System::Common::AudioAppBase InstanceType;
#else
#include "../common/instance/app.cpp"
typedef Reflex::System::Common::NonAudioApp InstanceType;
#endif

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

ArrayView <const char *> g_args;

bool g_has_quit = false;

REFLEX_END_INTERNAL




//
//ReflexAppDelegate

@interface ReflexAppDelegate : NSObject <NSApplicationDelegate>
{
	NSApplication * m_nsapp;

	InstanceType * m_reflex_instance;
}
- (void) applicationDidFinishLaunching:(NSNotification *)notification;
- (void) applicationWillTerminate:(NSNotification *)notification;
+ (void) menuAction:(id)sender;
@end

@implementation ReflexAppDelegate

void addMenuitem(NSMenu * menu, UInt32 idx, NSString * label, NSString * keycode)
{
	auto item = [menu addItemWithTitle: label action: @selector(menuAction:) keyEquivalent: keycode];

	item.target = [ReflexAppDelegate class];

	[item setRepresentedObject:[NSNumber numberWithInt:idx]];
}

- (void) applicationDidFinishLaunching: (NSNotification *)notification
{
	REFLEX_USE(Reflex);


	//start nsapp

	[[NSProcessInfo processInfo] disableSuddenTermination];

	m_nsapp = [NSApplication sharedApplication];



	//begin reflex

	root_module.Init();

	Array <CString::View> args;

	if (System::OSX::g_args)
	{
		auto ptr = Extend(args, System::OSX::g_args.size).data;

		for (auto & i : System::OSX::g_args)
		{
			*ptr++ = i;
		}

#if REFLEX_DEBUG
		if (args.GetFirst() == "-NSDocumentRevisionsDebugMode")
		{
			args.Remove(0, 2);
		}
#endif
	}

#ifdef REFLEX_SYSTEM_AUDIO
	m_reflex_instance = System::OSX::CreateCoreAudio().Adr();
#else
	m_reflex_instance = REFLEX_CREATE(InstanceType);
#endif

	if (auto global = m_reflex_instance->Initialise(args))
	{
		auto menubar = [[NSMenu alloc] init];

		auto menu = [[NSMenu alloc] init];

		auto menubaritem = [[NSMenuItem alloc] init];

		[menubar addItem:menubaritem];

		[menubaritem setSubmenu:menu];

		if (auto & menuitems = m_reflex_instance->config.app_menu)
		{
			REFLEX_LOOP(idx, menuitems.GetSize())
			{
				auto & item = menuitems[idx];

				auto label = System::OSX::MakeNSStringRef(item.a);

				WChar raw[2] = { item.b, 0 };

				auto keycode = System::OSX::MakeNSStringRef(raw + 0);

				addMenuitem(menu, idx, label, keycode);
			}

			[menu addItem: [NSMenuItem separatorItem]];
		}

		[menu addItemWithTitle: @"Hide" action: @selector(hide:) keyEquivalent: @"h"];

		[menu addItem: [NSMenuItem separatorItem]];

		[menu addItemWithTitle: @"Quit" action: @selector(terminate:) keyEquivalent: @"q"];

		[m_nsapp setMainMenu:menubar];
	}
	else
	{
		System::App::Quit();
	}
}

- (void) applicationWillTerminate: (NSNotification *)notification
{
	m_reflex_instance->Deinitialise();
}

+ (void)menuAction:(id)sender
{
	int idx = [[sender representedObject] intValue];

	auto app_delegate = [[NSApplication sharedApplication] delegate];

	auto instance = Reflex::Cast<ReflexAppDelegate>(app_delegate)->m_reflex_instance;

	if (auto & menuitems = instance->config.app_menu)
	{
		menuitems[idx].c();
	}
}

@end




//
//main

void exit()
{
#if !__has_feature(objc_arc)
	[[NSAutoreleasePool alloc] init];
#endif

	REFLEX_USE(Reflex);

	root_module.Deinit();
}

REFLEX_EXPORT int main(int argc, char *argv[])
{
	REFLEX_USE(Reflex);

	struct Compare
	{
		static bool eq(const char * a, CString::View b)
		{
			return ToView(a) == b;
		}
	};

	auto & args = System::OSX::g_args;
	
	args = { argv + 1, UInt(argc) - 1 };

	//XCODE_WORKAROUND

	if constexpr (REFLEX_DEBUG)
	{
		constexpr CString::View kDebugModeArg = "-NSDocumentRevisionsDebugMode";

		if (auto idx = Search<Compare>(args, kDebugModeArg))
		{
			args = Nudge(args, idx.value);
		}
	}

#if !__has_feature(objc_arc)
	[[NSAutoreleasePool alloc] init];
#endif

	auto delegate = [ReflexAppDelegate alloc];

	[[NSApplication sharedApplication] setDelegate:delegate];

	atexit(exit);

	return NSApplicationMain(argc, (const char **)argv);
}




//
//public api

const Reflex::System::EnvironmentType Reflex::System::kEnvironmentType = Reflex::System::kEnvironmentTypeDesktopApp;

void Reflex::System::App::Quit()
{
	constexpr auto DoQuit = [](CFRunLoopTimerRef, void *)
	{
		[[NSApplication sharedApplication] terminate:nil];
	};

	if (SetFiltered(OSX::g_has_quit, true))
	{
		auto timer = CFRunLoopTimerCreate(NULL, 0, 0, 0, 0, DoQuit, 0);

		CFRunLoopAddTimer(CFRunLoopGetMain(), timer, kCFRunLoopCommonModes);
	}
}
