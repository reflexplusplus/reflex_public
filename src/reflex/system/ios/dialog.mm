#include "window.h"

#include "../common/apple_utils.hpp"

// FIXME: [Florian] put at a better place in common with what's in file.mm
#define SECURITY_MODE 0 // iOS
// #define SECURITY_MODE NSURLBookmarkResolutionWithSecurityScope // macOS

@interface ReflexSystemIosFilePickerDelegate : NSObject<UIDocumentPickerDelegate> {
	Reflex::Function<void(NSArray<NSURL*>*)> callbackRef;
}

@end

@implementation ReflexSystemIosFilePickerDelegate

- (instancetype)initWithCallback:(Reflex::Function<void(NSArray<NSURL*>*)>)callback {
	if ((self = [super init])) {
		callbackRef = callback;
	}
	return self;
}

- (void)documentPickerWasCancelled:(UIDocumentPickerViewController *)controller {
	Reflex::System::InvokeAndClear(callbackRef, nil);
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
	Reflex::System::InvokeAndClear(callbackRef, urls);
}

@end

REFLEX_BEGIN_INTERNAL(Reflex::System)

template<class FN>
struct CallbackTask : public Task
{
	CallbackTask(const Function <FN> & callback)
		: m_completed(false)
		, m_callback(callback)
	{
	}

	template<class ... VARGS> void Invoke(VARGS&& ... args) 
	{
		// Invoke only once, and if it's retained somewhere else (we always retain it in our API call)
		if (!m_completed && GetRetainCount() > 1) 
		{
			m_completed = true;
			
			m_callback(std::forward<VARGS>(args)...);
		}
	}

	bool Completed() const override 
	{
		return m_completed;
	}

	void Wait() override {}


private:
	
	bool m_completed;
	
	Function <FN> m_callback;
};

Array<Reference<System::ExternalResourceRef>> TakeOwnershipOfUrls(NSArray<NSURL*>* urls)
{
	constexpr const char * kWarning = "System::ExternalResourceRef failed to acquire permission for file";

	Array<Reference<System::ExternalResourceRef>> result;

	for (NSURL* resource in urls)
	{
		if (![resource startAccessingSecurityScopedResource])
		{
			Common::output.Warn(kWarning, ToCStringView([resource description]));
			continue;
		}

		NSError* error;
		NSData* bookmarkData = [resource bookmarkDataWithOptions:SECURITY_MODE includingResourceValuesForKeys:nil relativeToURL:nil error:&error];

		if (!error && [bookmarkData length] != 0)
		{
			ArrayView<UInt8> token = { (UInt8*)[bookmarkData bytes], (UInt)[bookmarkData length] };

			result.Push(REFLEX_CREATE(iOS::ExternalResourceRef, token));
		}
		else
		{
			Common::output.Warn(kWarning, ToCStringView([resource description]), ToCStringView([error description]));
		}

		[resource stopAccessingSecurityScopedResource];
	}

	return result;
}

UTType* UtiForMimeType(const WString::View& mimeType) {
	UTType* t = [UTType typeWithMIMEType:ToNSString(mimeType)];
	// dynamic types are created when typeWithMIMEType doesn't map to an existing UTI.
	// Force the user to enter a known type instead of messing with ghost types.
	return [t isDynamic] ? nil : t;
}


// Same reason as below, keep it so that it will be invoked (retained from somewhere other than the API function, meaning that the caller is still alive)
Reference<Task, false> s_asyncTask;
// A bug (?) in the current iOS framework, it does not retain the delegate internally, so we have to make sure that it keeps existing.
ReflexSystemIosFilePickerDelegate* s_filePickerDelegate = nil;

bool g_keyboard_shown = false;

REFLEX_END_INTERNAL

// MEMO: Synchronous API not supported on iOS
Reflex::UInt32 Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 buttonflags) {
	REFLEX_ASSERT_MAINTHREAD("System::ShowMessageBox");

	if (s_asyncTask) {
		DEV_LOG("Already showing a message box");
		return 0;
	}

	// Needs to be kept somewhere, otherwise it won't be called
	s_asyncTask = ShowMessageBox(type, title, text, buttonflags, [] (UInt32 ignoredResult) {
		s_asyncTask.Clear();
	});
	return 1;
}

Reflex::Reference<Reflex::System::Task> Reflex::System::ShowMessageBox(UInt32 type, const WString & title, const ArrayView <WString> & text, UInt32 buttonflags, const Function <void(UInt32 clickedButton)> & callback) {
	REFLEX_ASSERT_MAINTHREAD("System::ShowMessageBox");

	NSMutableString* message = [NSMutableString string];
	REFLEX_FOREACH(line, text) {
		[message appendString:ToNSString(line)];
		if (&line != &text.GetLast()) {
			[message appendString:@"\n"];
		}
	}

	auto task = Make<CallbackTask<void(UInt32)>>(callback);
	auto alert = [UIAlertController alertControllerWithTitle:ToNSString(title) message:message preferredStyle:UIAlertControllerStyleAlert];
	auto defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:^(UIAlertAction* action) {
		task->Invoke(1 /* IDOK */);
	}];
	[alert addAction:defaultAction];

	if (auto view_controller = iOS::Window::st_viewcontroller) {
		[view_controller presentViewController:alert animated:YES completion:nil];
	}
	else {
		task->Invoke(1 /* IDOK */);
	}

	return task;
}

Reflex::WString Reflex::System::GetOpenPath(const WString & title, const ArrayView <WString> & filters, const WString & dir, const WString & filename) {
	DEV_ERROR("Reflex::System::GetOpenPath unsupported on mobile");
	return {};
}

Reflex::WString Reflex::System::GetSavePath(const WString & title, const WString & filter, const WString & dir, const WString & filename) {
	DEV_ERROR("Reflex::System::GetSavePath unsupported on mobile");
	return {};
}

Reflex::WString Reflex::System::GetFolder(const WString & title, const WString & root, bool cancreate) {
	DEV_ERROR("Reflex::System::GetFolder unsupported on mobile");
	return {};
}

Reflex::Reference<Reflex::System::Task> Reflex::System::SelectExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, bool allowMultiple, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback) 
{
	REFLEX_ASSERT_MAINTHREAD("System::SelectExternalResource");

	auto task = Make<CallbackTask<void(const Array<Reference<System::ExternalResourceRef>>&)>>(callback);

	NSMutableArray <UTType *> * utiTypes = [NSMutableArray array];
	REFLEX_FOREACH(mimeType, mime_types) {
		auto uti = UtiForMimeType(mimeType);
		if (uti) [utiTypes addObject:uti];
	}

	// For any type (if none valid has been specified)
	if (utiTypes.count == 0) {
		[utiTypes addObject:[UTType typeWithIdentifier:@"public.data"]];
	}

	auto picker = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:utiTypes asCopy:NO];
	s_filePickerDelegate = [[ReflexSystemIosFilePickerDelegate alloc] initWithCallback:[task] (NSArray<NSURL*>* urls) {
		task->Invoke(TakeOwnershipOfUrls(urls));
	}];

	picker.delegate = s_filePickerDelegate;
	picker.shouldShowFileExtensions = YES;
	picker.modalPresentationStyle = UIModalPresentationFormSheet;

	if (auto view_controller = iOS::Window::st_viewcontroller) {
		[view_controller presentViewController:picker animated:YES completion:nil];
	}
	else {
		task->Invoke(Array<Reference<System::ExternalResourceRef>>());
	}

	return task;
}

Reflex::Reference<Reflex::System::Task> Reflex::System::CreateExternalResource(const ArrayView <WString>& mime_types, ExternalResourceRef::AccessMode accessType, const WString::View& suggestedName, const Function<void(const Array<Reference<System::ExternalResourceRef>>& urls)>& callback) 
{
	REFLEX_ASSERT_MAINTHREAD("System::CreateExternalResource");

	// MEMO: iOS does not support simply creating a file, instead it supports exporting an existing one (from the app's sandbox), so we pick a name, extension, write an empty file and pass it to the export file picker
	auto ext = @"";
	if (mime_types.size >= 1) {
		auto preferredExt = [UtiForMimeType(mime_types[0]) preferredFilenameExtension];

		if (preferredExt) {
			ext = [@"." stringByAppendingString:preferredExt];
		}
	}

	auto fileName = [ToNSString(suggestedName) stringByAppendingString:ext];
	auto tempURL = [NSURL fileURLWithPath:[NSTemporaryDirectory() stringByAppendingPathComponent:fileName]];
	auto asArray = [NSArray arrayWithObject:tempURL];
	[[NSData data] writeToURL:tempURL atomically:YES];

	auto task = Make<CallbackTask<void(const Array<Reference<System::ExternalResourceRef>>&)>>(callback);
	auto picker = [[UIDocumentPickerViewController alloc] initForExportingURLs:asArray asCopy:NO];
	s_filePickerDelegate = [[ReflexSystemIosFilePickerDelegate alloc] initWithCallback:[task] (NSArray<NSURL*>* urls) {
		task->Invoke(TakeOwnershipOfUrls(urls));
	}];

	picker.delegate = s_filePickerDelegate;
	picker.shouldShowFileExtensions = YES;
	picker.modalPresentationStyle = UIModalPresentationFormSheet;

	if (auto view_controller = iOS::Window::st_viewcontroller) {
		[view_controller presentViewController:picker animated:YES completion:nil];
	}
	else {
		task->Invoke(Array<Reference<System::ExternalResourceRef>>());
	}

	return task;
}

bool Reflex::System::ShowVirtualKeyboard(VirtualKeyboardInputType type, const WString & textbuffer, Pair <UInt> selection, const Function < void(const WString &, Pair <UInt>) > & ondone) {
	g_keyboard_shown = true;
	[iOS::Window::st_viewcontroller beginTextInput:type withText:ToNSString(textbuffer) selection:selection onDone:ondone];
	return true;
}

void Reflex::System::DismissVirtualKeyboard() {
	if (SetFiltered(g_keyboard_shown, false)) [iOS::Window::st_viewcontroller dismissTextInput];
}
