#include "library.h"
#include "window.h"
//#import <QuickLook/QuickLook.h>




//
//objc

// https://clang.llvm.org/docs/AutomaticReferenceCounting.html#arc-objects-operands-casts

@interface ReflexDisplayLinkReceiver() {
	CADisplayLink* m_displaylink;
}

- (instancetype)init;
- (void)dealloc;
@end

@implementation ReflexDisplayLinkReceiver

- (instancetype)init {
	if ((self = [super init])) {
		m_displaylink = [CADisplayLink displayLinkWithTarget:self selector:@selector(updateFrame)];
		[m_displaylink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
	}

	return self;
}

- (void)dealloc {
	[m_displaylink invalidate];
}

- (void)updateFrame {
	Reflex::System::iOS::Library::OnTimer(0, nullptr);
}
@end

@interface QLSingleFileDataSource : NSObject<QLPreviewControllerDataSource>

@property (nonatomic, strong) NSURL *fileURL;

@end

@implementation QLSingleFileDataSource

- (NSInteger)numberOfPreviewItemsInPreviewController:(QLPreviewController *)controller {
	return 1;
}

- (id<QLPreviewItem>)previewController:(QLPreviewController *)controller previewItemAtIndex:(NSInteger)index {
	return self.fileURL;
}

@end

REFLEX_BEGIN_INTERNAL(Reflex::System::iOS)

constexpr Float64 kClockInterval = 1.0 / 60;

constexpr bool kUseVsyncForSystemTimer = false;

REFLEX_END_INTERNAL

// MARK: Reflex::System::iOS

void Reflex::System::Common::SetInstanceHandle(void * unused)
{
}

Reflex::Detail::Module::Member <Reflex::System::iOS::Library> Reflex::System::iOS::globals(Common::g_module);

Reflex::System::iOS::Library::Library()
{
	CFRunLoopTimerContext context;
	context.version = 0;
	context.info = this;
	context.retain = 0;
	context.release = 0;
	context.copyDescription = 0;

	if (kUseVsyncForSystemTimer) {
		m_displaylinkreceiver = MakeOwnedObjCRef([[ReflexDisplayLinkReceiver alloc] init]);
	}
	else {
		m_timer = CFRunLoopTimerCreate(
		   /* allocator */ kCFAllocatorDefault,
		   /* fireDate  */ CFAbsoluteTimeGetCurrent() + kClockInterval,
		   /* interval  */ kClockInterval,
		   /* flags     */ 0,
		   /* order     */ 0,
		   /* callout   */ &Library::OnTimer,
		   /* context   */ &context);
		CFRunLoopAddTimer(CFRunLoopGetMain(), m_timer, kCFRunLoopCommonModes);
	}
}

Reflex::System::iOS::Library::~Library() {
	if (m_timer) {
		CFRunLoopRemoveTimer(CFRunLoopGetMain(), m_timer, kCFRunLoopCommonModes);
		CFRelease(m_timer);
	}
}

void Reflex::System::iOS::Library::OnTimer(CFRunLoopTimerRef timer, void* info) {
	if (auto window = Window::st_self) {
		window->ProcessPendingTasks();
	}

	iOS::globals->m_signals[kNotificationClock].Notify();
}

// MARK: Reflex::System

const Reflex::System::Platform Reflex::System::kPlatform = kPlatformIOS;

Reflex::CString Reflex::System::GetOperatingSystemVersion() {
	return ToCStringView([[UIDevice currentDevice] systemVersion]);
}

Reflex::UInt64 Reflex::System::GetSystemID() {
	auto uuid = UIDevice.currentDevice.identifierForVendor;

	auto uuid_string = uuid.UUIDString.UTF8String;

	return Reflex::Detail::MakeHash<UInt64>(ToView(uuid_string));
}

Reflex::WString Reflex::System::GetPath(Path path)
{
	NSSearchPathDirectory directoryType;

	switch (path)
	{
		case kPathDesktop:
			directoryType = NSDesktopDirectory;
			break;

		case kPathApplicationData:
			directoryType = NSApplicationSupportDirectory;
			break;

		case kPathUserData:
			directoryType = NSLibraryDirectory;
			break;

		case kPathUserDocuments:
			directoryType = NSDocumentDirectory;
			break;

		case kPathTemp:
			directoryType = NSCachesDirectory;
			break;

		default:
			return {};
	}

	NSArray* possibleURLs = [NSFileManager.defaultManager URLsForDirectory:directoryType inDomains:NSUserDomainMask];

	if (possibleURLs.count > 0)
	{
		NSURL* first = possibleURLs.firstObject;

		WString path = ToWString(first.path);
		File::Detail::CorrectTrailingStroke(path);
		return path;
	}
	else
	{
		DEV_WARN("System::GetPath no path of type ", UInt(path));

		return {};
	}
}

Reflex::WString Reflex::System::GetExecutablePath()
{
	return {};
}

void Reflex::System::SetClipboard(const WString& string) {
	[[UIPasteboard generalPasteboard] setString:ToNSString(string)];
}

Reflex::WString Reflex::System::GetClipboard() {
	return ToWString([[UIPasteboard generalPasteboard] string]);
}

Reflex::Array<Reflex::System::iRect> Reflex::System::GetScreens() {
	auto screenSize = UIScreen.mainScreen.bounds.size;

	iRect wholeScreen = { {0, 0}, {int(screenSize.width), int(screenSize.height)} };

	return ToView(wholeScreen);
}

Reflex::TRef<Reflex::Object> Reflex::System::CreateListener(Notification notification, void* client, void (*callback)(void*)) {
	return iOS::globals->m_signals[notification].Create(client, callback);
}

Reflex::Int32 Reflex::System::GetMaxPixelDensity() {
	return Int32(UIScreen.mainScreen.scale);
}

bool Reflex::System::IsDarkTheme() {
	return UIScreen.mainScreen.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark;
}

Reflex::Float Reflex::System::GetFontScale() {
	auto baseFont = [UIFont systemFontOfSize:32.0];
	auto scaledFont = [UIFontMetrics.defaultMetrics scaledFontForFont:baseFont];
	return scaledFont.pointSize / 32.0;
}

Reflex::UInt8 Reflex::System::GetModifierKeys() {
	return 0;
}

bool Reflex::System::Open(const WString& path) //TODO need bool arg for "prefer_preview"
{
	REFLEX_ASSERT_MAINTHREAD("System::Open");

	constexpr auto OpenExternalUrl = [](NSURL * url)
	{
		if ([UIApplication.sharedApplication canOpenURL:url])
		{
			[UIApplication.sharedApplication openURL:url options:@{} completionHandler:nil];

			return true;
		}

		return false;
	};

	constexpr auto ShareOrOpen = [](UIViewController * _Nonnull vc, NSURL * _Nonnull fileURL)
	{
		ObjCRef<UIActivityViewController*> a = MakeOwnedObjCRef([[UIActivityViewController alloc] initWithActivityItems:@[fileURL] applicationActivities:nil]);

		a.Get().popoverPresentationController.sourceView = vc.view;

		[vc presentViewController:a animated:YES completion:nil];
	};

//	constexpr auto QuickLook = [](UIViewController * _Nonnull vc, NSURL * _Nonnull fileURL) {
//		if (![QLPreviewController canPreviewItem:fileURL]) return false;
//
//		ObjCRef<QLSingleFileDataSource*> delegate = MakeOwnedObjCRef([QLSingleFileDataSource new]);
//		ObjCRef<QLPreviewController*> ql = MakeOwnedObjCRef([QLPreviewController new]);
//
//		delegate.Get().fileURL = fileURL;
//		ql.Get().dataSource = delegate;
//
//		[vc presentViewController:ql animated:YES completion:NULL];
//
//		return YES;
//	};

	constexpr auto QuickLook = [](UIViewController * _Nonnull vc, NSURL * _Nonnull fileURL)
	{
		return false;
	};

	if (GetThreadID() != kMainThreadID)
	{
		DEV_WARN("Reflex::System::Open: must be run from main thread");
		return false;
	}

	auto input = ToNSString(path);

	if (NSURL * url = [NSURL URLWithString:input])
	{
		if (OpenExternalUrl(url))
		{
			return true;
		}
		else if (auto view_controller = iOS::Window::st_viewcontroller)
		{
			if (!url.isFileURL) url = [NSURL fileURLWithPath:input];

			bool is_dir = false;

			if (![NSFileManager.defaultManager fileExistsAtPath:url.path isDirectory:&is_dir]) return false;

			if (is_dir) return false;

			// !untested TODO test in test Project with files

			if (!QuickLook(view_controller, url))
			{
				ShareOrOpen(view_controller, url);
			}

			return true;
		}
	}

	return false;
}

bool Reflex::System::Share(const ArrayView <WString> & items, const WString & text)
{
	REFLEX_ASSERT_MAINTHREAD("System::Share");

	auto view_controller = iOS::Window::st_viewcontroller;

	if (!view_controller) return false;

	NSMutableArray * share_items = [NSMutableArray arrayWithCapacity:items.size + (text ? 1 : 0)];

	if (text)
	{
		[share_items addObject:ToNSString(text)];
	}

	for (const auto & item : items)
	{
		if (IsAbsolutePath(item))
		{
			[share_items addObject:[NSURL fileURLWithPath:ToNSString(item)]];
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

	ObjCRef<UIActivityViewController*> activity = MakeOwnedObjCRef([[UIActivityViewController alloc] initWithActivityItems:share_items applicationActivities:nil]);

	if (auto popover = activity.Get().popoverPresentationController)
	{
		popover.sourceView = view_controller.view;
		popover.sourceRect = view_controller.view.bounds;
	}

	[view_controller presentViewController:activity animated:YES completion:nil];
	
	return true;
}

// MARK: Debug
#if (REFLEX_DEBUG)
void Reflex::System::DebugLog(bool brk, const char * text) {
	NSLog(@"%s", text);

	if (brk) g_on_debug_break(text);
}
#endif

void Reflex::System::Log(const char* text) {
	NSLog(@"%s", text);
}
