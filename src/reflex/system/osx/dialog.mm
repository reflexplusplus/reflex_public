#include "globals.h"
#include "window.h"




REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

template <class TYPE> bool OpenFileDialog(TYPE * dialog, const WString & directory, const WString & filename, const Array <WString> & filters)
{
	NSApplication * app = [NSApplication sharedApplication];

	[dialog setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces];

	[dialog setAllowsOtherFileTypes:false];

	#if (!REFLEX_INCLUDE_UI)
	[app setActivationPolicy:NSApplicationActivationPolicyRegular];

	[app activateIgnoringOtherApps:YES];

	[dialog makeKeyAndOrderFront:nil];
	#endif

	NSWindow * keywindow = [app keyWindow];

	NSWindow * mainwindow = [app mainWindow];

	[keywindow resignKeyWindow];

	[mainwindow resignMainWindow];

	auto nsdirectory = MakeNSStringRef(directory);

	auto nsfilename = MakeNSStringRef(filename);

	bool rtn = false;

	dialog.directoryURL = [NSURL fileURLWithPath:nsdirectory];
	dialog.nameFieldStringValue = nsfilename;

	if (UInt32 nfilter = filters.GetSize())
	{
		ObjCRef <NSString*> stringrefs[nfilter];
		NSString * strings[nfilter];

		REFLEX_LOOP(idx, nfilter)
		{
			stringrefs[idx] = MakeNSStringRef(filters[idx]);

			strings[idx] = stringrefs[idx].Get();
		}

		dialog.allowedFileTypes = [NSArray arrayWithObjects:strings count:nfilter];

		if constexpr (kIsType<TYPE,NSOpenPanel>)
		{
			dialog.canChooseFiles = YES;
			dialog.canChooseDirectories = NO;
			dialog.allowsMultipleSelection = NO;
		}
	}
	else
	{
		[dialog setAllowedFileTypes:0];
	}

	rtn = ([dialog runModal] == NSModalResponseOK);

	[keywindow makeKeyWindow];

	[mainwindow makeMainWindow];

	return rtn;
}

REFLEX_END_INTERNAL

Reflex::UInt32 Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & msg, UInt32 flags)
{
	return 0;
}

Reflex::WString Reflex::System::GetSavePath(const WString & title, const WString & filters, const WString & dir, const WString & filename)
{
	WString rtn;

	auto dialog = [NSSavePanel savePanel];

	auto nstitle = OSX::MakeNSStringRef(title);

	[dialog setTitle:nstitle];
	[dialog setMessage:nstitle];
	[dialog setTitle:nstitle];

	if (OSX::OpenFileDialog<NSSavePanel>(dialog, dir, filename, {filters}))
	{
		NSURL * url = [dialog URL];

		rtn = ToWString([url path]);
	}

	return rtn;
}

Reflex::WString Reflex::System::GetOpenPath(const WString & title, const ArrayView <WString> & filters, const WString & dir, const WString & filename)
{
	WString rtn;

	auto dialog = [NSOpenPanel openPanel];

	auto nstitle = OSX::MakeNSStringRef(title);

	[dialog setTitle:nstitle];
	[dialog setCanChooseFiles:true];
	[dialog setCanChooseDirectories:false];
	[dialog setCanCreateDirectories:false];

	if (OSX::OpenFileDialog<NSOpenPanel>(dialog, dir, filename, filters))
	{
		NSArray * files = [dialog URLs];

		if ([files count])
		{
			NSURL * url = [files objectAtIndex:0];

			rtn = ToWString([url path]);
		}
	}

	return rtn;
}

Reflex::WString Reflex::System::GetFolder(const WString & title, const WString & dir, bool cancreate)
{
	WString rtn;

	auto dialog = [NSOpenPanel openPanel];

	auto nstitle = OSX::MakeNSStringRef(title);

	[dialog setTitle:nstitle];
	[dialog setCanChooseFiles:false];
	[dialog setCanChooseDirectories:true];
	[dialog setCanCreateDirectories:cancreate];

	if (OSX::OpenFileDialog<NSOpenPanel>(dialog, dir, {}, {}))
	{
		NSArray * files = [dialog URLs];

		if ([files count])
		{
			NSURL * url = [files objectAtIndex:0];

			rtn = ToWString([url path]);

			rtn.Push(L'/');
		}
	}

	return rtn;
}

Reflex::Reference <Reflex::System::Task> Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 buttonflags, const Function <void(UInt32 clickedButton)> & callback)
{
	// TODO: [Florian] implement for desktop
	DEV_WARN("ShowMessageBox Not supported on desktop platforms");
	return Task::null;
}

Reflex::Reference <Reflex::System::Task> Reflex::System::SelectExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, bool allowMultiple, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback)
{
	// TODO: [Florian] implement for desktop
	DEV_WARN("SelectExternalResource Not supported on desktop platforms");
	return Task::null;
}

Reflex::Reference <Reflex::System::Task> Reflex::System::CreateExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, const WString::View& suggestedName, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback)
{
	// TODO: [Florian] implement for desktop
	DEV_WARN("CreateExternalResource Not supported on desktop platforms");
	return Task::null;
}

Reflex::TRef <Reflex::System::ExternalResourceRef> Reflex::System::ExternalResourceRef::Locate(const ArrayView<UInt8>& token) 
{
	// TODO: [Florian] implement for desktop
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
