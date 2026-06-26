#include "window.h"
#include "globals.h"

#include "resources.cpp"




//
//window:global implementation

REFLEX_SET_TRAIT(NSPoint,IsRawCopyable)	//for step filter

Reflex::System::OSX::Window::Global::Global()
{
	[[NSApplication sharedApplication] activateIgnoringOtherApps:true];

	m_eventsourceref = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);

	auto image = MakeOwnedObjCRef([[NSImage alloc] initWithSize:NSMakeSize(1, 1)]);

	m_osxcursor[kMouseCursorInvisible] = MakeOwnedObjCRef([[NSCursor alloc] initWithImage:image hotSpot:NSMakePoint(0,0)]);

	m_osxcursor[kMouseCursorArrow] = MakeObjCRef([NSCursor arrowCursor]);
	m_osxcursor[kMouseCursorWait] = MakeObjCRef([NSCursor arrowCursor]);

	InitCursor(kMouseCursorMove, move_png, NSMakePoint(9, 9));
	InitCursor(kMouseCursorLeftRight, leftright_png, NSMakePoint(8, 4));
	InitCursor(kMouseCursorTopBottom, topdown_png, NSMakePoint(4, 8));
	InitCursor(kMouseCursorTopLeftBottomRight, northwest_png, NSMakePoint(6, 6));
	InitCursor(kMouseCursorBottomLeftTopRight, northeast_png, NSMakePoint(6, 6));
	InitCursor(kMouseCursorZoom, zoom_png, ZoomX2_png, NSMakePoint(4, 4));
	InitCursor(kMouseCursorPointer, Hand_png, HandX2_png, NSMakePoint(4, 4));

	m_osxcursor[kMouseCursorDrag] = MakeObjCRef([NSCursor openHandCursor]);
	m_osxcursor[kMouseCursorText] = MakeObjCRef([NSCursor IBeamCursor]);
	m_osxcursor[kMouseCursorBlock] = MakeObjCRef([NSCursor arrowCursor]);
}

Reflex::System::OSX::Window::Global::~Global()
{
	CFRelease(m_eventsourceref);
}

void Reflex::System::OSX::Window::Global::InitCursor(MouseCursor cursor, const ArrayView <UInt8> & item, const NSPoint & point)
{
	auto nsdata = MakeOwnedObjCRef([[NSData alloc] initWithBytesNoCopy:RemoveConst(item.data) length:item.size freeWhenDone:false]);

	auto nsimage = MakeOwnedObjCRef([[NSImage alloc] initWithData:nsdata]);

	m_osxcursor[cursor] = MakeOwnedObjCRef([[NSCursor alloc] initWithImage:nsimage hotSpot:point]);
}

void Reflex::System::OSX::Window::Global::InitCursor(MouseCursor cursor, const ArrayView <UInt8> & lores, const ArrayView <UInt8> & hires, const NSPoint & point)
{
	auto lo_data = MakeOwnedObjCRef([[NSData alloc] initWithBytesNoCopy:RemoveConst(lores.data) length:lores.size freeWhenDone:false]);

	auto lo_rep = MakeObjCRef([NSBitmapImageRep imageRepWithData:lo_data]);

	auto hi_data = MakeOwnedObjCRef([[NSData alloc] initWithBytesNoCopy:RemoveConst(hires.data) length:hires.size freeWhenDone:false]);

	auto hi_rep = MakeObjCRef([NSBitmapImageRep imageRepWithData:hi_data]);

	NSSize size = [lo_rep size];

	[hi_rep setSize: size];

	auto nsimage = MakeOwnedObjCRef([[NSImage alloc] initWithSize:size]);

	[nsimage addRepresentation:lo_rep];

	[nsimage addRepresentation:hi_rep];

	m_osxcursor[cursor] = MakeOwnedObjCRef([[NSCursor alloc] initWithImage:nsimage hotSpot:point]);
}

double Reflex::System::OSX::Window::st_eventsuppressioninterval = 0.25f;

Reflex::System::OSX::Window::Window(UInt32 style, bool ontop, void * host_window)
{
	m_nsview = [[NS_View alloc] init:this :&System::Window::Client::null :style :ontop :host_window];
}

Reflex::System::OSX::Window::~Window()
{
	auto globals = Reflex::System::OSX::globals.Adr();

	auto & client = m_nsview->m_client;

	client->OnSetOwner(nullptr);

	client.Clear();


	if (globals->m_capturewindow == m_nsview) globals->m_mousebuttonstate = 0;


	[[NSNotificationCenter defaultCenter] removeObserver:m_nsview];

	m_nsview->m_trackingarea.Clear();


	NSWindow * nswindow = m_nsview->m_nswindow;

	if (nswindow && m_nsview->m_fullscreen)
	{
		[nswindow toggleFullScreen:0];
	}

	for (NSView * subview in [m_nsview subviews])
	{
		[subview removeFromSuperview];
	}

	[m_nsview removeFromSuperview];	//prevent os (live) from doing mouseevent after closed

	if (auto host_view = m_nsview->m_host_view)		//JUCE_EMBED
	{
		[host_view setSubviews:[NSArray array]];	//JUCE_EMBED
	}
	else
	{
		if (nswindow)
		{
			static_cast<NS_Window*>(nswindow)->m_nsview = 0;

			[nswindow close];

			REFLEX_OBJC_RELEASE(nswindow);
		}

		m_nsview->m_nswindow = 0;
	}

	Release(*m_nsview->m_global);
}

void Reflex::System::OSX::Window::SetClient(TRef <Client> client)
{
	m_nsview->m_client = client;

	client->OnSetOwner(this);
}

Reflex::TRef <Reflex::System::Window::Client> Reflex::System::OSX::Window::GetClient()
{
	return m_nsview->m_client;
}

void Reflex::System::OSX::Window::Attach(System::Window & parent)
{
	if (!m_nsview->m_host_view)	//JUCE_EMBED
	{
		if (auto osxparent = DynamicCast<OSX::Window>(parent))
		{
			auto nsparent = osxparent->m_nsview->m_nswindow;

			[nsparent addChildWindow:m_nsview->m_nswindow ordered:NSWindowAbove];
		}
	}
}

void Reflex::System::OSX::Window::Detach()
{
	if (!m_nsview->m_host_view)	//JUCE_EMBED
	{
		NSWindow * nswindow = m_nsview->m_nswindow;

		[[nswindow parentWindow] removeChildWindow:nswindow];
	}
}

void Reflex::System::OSX::Window::SetTitle(const WString & label)
{
	if (!m_nsview->m_host_view)	//JUCE_EMBED
	{
		NSString * nsstring = [[NSString alloc] initWithBytes:label.GetData() length:label.GetSize() * sizeof(WChar) encoding:NSUTF32LittleEndianStringEncoding];

		[m_nsview->m_nswindow setTitle:nsstring];

		REFLEX_OBJC_RELEASE(nsstring);
	}
}

void Reflex::System::OSX::Window::EnableInput(bool enable)
{
}

void Reflex::System::OSX::Window::SetDisplayMode(WindowDisplay mode)
{
	if (m_nsview->m_host_view) return;	//JUCE_EMBED

	auto globals = Reflex::System::OSX::globals.Adr();

	auto client = m_nsview->m_client;

	auto nswindow = m_nsview->m_nswindow;

	auto & mb = globals->m_mousebuttonstate;

	m_nsview->m_displaymode = mode;

	switch (mode)
	{
	case kWindowDisplayMinimised:
		[nswindow miniaturize:nil];
		break;

	case kWindowDisplayWindowed:
		if (m_nsview->m_fullscreen)
		{
			REFLEX_LOOP(idx, 2) if (BitCheck(mb, idx)) client->OnMouseUp(idx);

			mb = 0;

			[nswindow deminiaturize:nil];

			[nswindow toggleFullScreen:nil];

			[nswindow setLevel:m_nsview->m_ontop ? NSPopUpMenuWindowLevel : NSNormalWindowLevel];
		}
		else
		{
			[nswindow deminiaturize:nil];

			[m_nsview onResize:nullptr];

			[nswindow makeKeyAndOrderFront:nil];	//workaround: if done after exit fullscreen, input is lost for unknown reason
		}
		break;

	case kWindowDisplayFullScreen:
		if (!m_nsview->m_fullscreen)
		{
			REFLEX_LOOP(idx, 2) if (BitCheck(mb, idx)) client->OnMouseUp(idx);

			mb = 0;

			[nswindow deminiaturize:nil];			//bigSur workaround, for when (re)starting in fullscreen
			[nswindow toggleFullScreen:nil];
			[nswindow makeKeyAndOrderFront:nil];	//bigSur workaround
		}
		break;
	}
}

void Reflex::System::OSX::Window::SetRect(const iRect & rect)
{
	if (m_nsview->m_host_view)	//JUCE_EMBED
	{
		m_nsview->m_xywh = { {0, 0}, rect.size };

		auto nssize = NSMakeSize(rect.size.w, rect.size.h);

		[m_nsview setFrameSize:nssize];
	}
	else
	{
		m_nsview->m_xywh = rect;

		auto x = rect.origin.x;
		auto y = rect.origin.y;
		auto w = rect.size.w;
		auto h = rect.size.h;

		NSRect screenrect = [[NSScreen mainScreen] frame];

		NSPoint point = NSMakePoint(x, screenrect.size.height - (y + h));

		NSRect rect = NSMakeRect(x, y, w, h);

		rect = NSMakeRect(x, screenrect.size.height - (y + h), w, h);

		NSWindow * window = m_nsview->m_nswindow;

		[window setContentSize:rect.size];

		[window setFrameOrigin:point];
	}
}

void Reflex::System::OSX::Window::SendTop()
{
	NSWindow * window = m_nsview->m_nswindow;

	if (!m_nsview->m_host_view)	//JUCE_EMBED
	{
		[window makeKeyAndOrderFront:nil];	//TEST was orderFront
	}

	[window makeFirstResponder:m_nsview];
}

void Reflex::System::OSX::Window::SetMouseCursor(MouseCursor mousecursor)
{
	NSCursor * cursor = m_nsview->m_global->m_osxcursor[mousecursor];

	m_nsview->m_mousecursor = cursor;

	[m_nsview RefreshMouseCursor];
}

void Reflex::System::OSX::Window::SetMousePosition(const iPoint & position)
{
	auto globals = Reflex::System::OSX::globals.Adr();

	auto rect = m_nsview->m_xywh;

	if (m_nsview->m_host_view)
	{
		auto nswindow = m_nsview->m_nswindow;

		NSRect nsrect = [m_nsview convertRect:m_nsview.bounds toView:nil];

		nsrect = [nswindow convertRectToScreen:nsrect];

		rect.origin.x = Truncate(nsrect.origin.x);

		rect.origin.y = globals->m_desktopheight - Truncate(nsrect.origin.y + nsrect.size.height);
	}

	CGPoint point = {Float32(rect.origin.x + position.x), Float32(rect.origin.y + position.y)};

	CGWarpMouseCursorPosition(point);

	[m_nsview RefreshMouseCursor];

	//CGAssociateMouseAndMouseCursorPosition(true);
}

void Reflex::System::OSX::Window::BeginDragDropFiles(const ArrayView <WString> & filenames)
{
	NSEvent * mousedownevent = m_nsview->m_mousedownevent;

	const UInt n = filenames.size;

	if (!(True(mousedownevent) && n)) return;

	NSMutableArray<NSDraggingItem *> * items = [NSMutableArray arrayWithCapacity:n];

	NSImage * drag_icon = nil;
	NSSize icon_size = NSMakeSize(64, 64);

	{
		auto first_path_ref = MakeNSStringRef(filenames[0]);

		drag_icon = [[NSWorkspace sharedWorkspace] iconForFile:first_path_ref.Get()];

		if (drag_icon) icon_size = drag_icon.size;
	}

	NSPoint p = [m_nsview convertPoint:mousedownevent.locationInWindow fromView:nil];

	NSRect frame = NSMakeRect(p.x - icon_size.width * 0.5f, p.y - icon_size.height * 0.5f, icon_size.width, icon_size.height);

	REFLEX_LOOP(idx, n)
	{
		auto path_ref = MakeNSStringRef(filenames[idx]);

		NSURL * url = [NSURL fileURLWithPath:path_ref.Get()];

		auto pb_ref = MakeOwnedObjCRef([[NSPasteboardItem alloc] init]);
		[pb_ref.Get() setString:url.absoluteString forType:NSPasteboardTypeFileURL];

		auto di_ref = MakeOwnedObjCRef([[NSDraggingItem alloc] initWithPasteboardWriter:pb_ref.Get()]);

		// IMPORTANT: every item must have a dragging frame, first draws an icon; rest can be nil contents.
		[di_ref.Get() setDraggingFrame:frame contents:(idx == 0 ? drag_icon : nil)];

		[items addObject:di_ref.Get()];
	}

	auto globals = Reflex::System::OSX::globals.Adr();

	globals->StopTimer();
	globals->StartTimer(25);

	[m_nsview beginDraggingSessionWithItems:items event:mousedownevent source:m_nsview];
}

NSUInteger Reflex::System::OSX::Window::ConvertStyleFlags(UInt32 flags)
{
	bool minimisable = True(flags & kWindowStyleMinimisable);

	bool resizable = True(flags & kWindowStyleResizable);

	bool frame = True(flags & kWindowStyleFrame) || minimisable || resizable;


	NSUInteger osxstyle = 0;

	osxstyle = ((NSWindowStyleMaskTitled | NSWindowStyleMaskClosable) * frame);

	osxstyle |= (NSWindowStyleMaskMiniaturizable * minimisable);

	osxstyle |= (NSWindowStyleMaskResizable * resizable);


	return osxstyle;
}

Reflex::TRef < Reflex::ObjectOf <Reflex::System::RawBitmap> > Reflex::System::OSX::Window::CreateExportBitmapBuffer(UInt8 flags) const
{
	return REFLEX_CREATE(ObjectOf<RawBitmap>, );
}

void Reflex::System::OSX::Window::ExportBitmap(TRef < ObjectOf <RawBitmap> > rtn) const
{
	auto & info = rtn->value.a;

	info.format = kImageFormatRGB;

	if (NSWindow * nswindow = m_nsview->m_nswindow)
	{
		NSRect content = [nswindow frame];

		content = [nswindow contentRectForFrameRect:content];

		CGImageRef imageref = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, CGWindowID([nswindow windowNumber]), kCGWindowImageBoundsIgnoreFraming | kCGWindowImageBestResolution);

		//kCGWindowImageBoundsIgnoreFraming -> this flag doesnt work, need to manually workaround

		auto image_w = CGImageGetWidth(imageref);

		auto image_h = CGImageGetHeight(imageref);

		auto bpr = CGImageGetBytesPerRow(imageref);

		//UInt32 bpp = CGImageGetBitsPerPixel(imageref);

		if (image_w * image_h)
		{
			info.size = { Truncate(content.size.width), Truncate(content.size.height) };

			info.pixdensity = UInt(image_w / info.size.w);

			Int32 & dpi = info.pixdensity;


			CGDataProviderRef provider = CGImageGetDataProvider(imageref);

			NSData * nsdata = (__bridge id)CGDataProviderCopyData(provider);

			REFLEX_OBJC_RELEASE(nsdata);


			Array <UInt8> & data = rtn->value.b;

			data.SetSize(info.size.w * dpi * info.size.h * dpi * 3);


			const UInt8 * prow = Cast<UInt8>([nsdata bytes]);

			if ((image_h/dpi) > info.size.h)	//FIX for the fact that kCGWindowImageBoundsIgnoreFraming doesnt work
			{
				//prow += Truncate(content.origin.x) * 4 * dpi;	(assume no left border, could change in OSX v?)

				prow += (((image_h/dpi) - info.size.h) * bpr * dpi);
			}

			UInt xn = info.size.w * dpi;

			UInt8 * prgb = data.GetData();

			REFLEX_LOOP(idx, info.size.h * dpi)
			{
				const UInt8 * prgba = prow;

				REFLEX_LOOP(x, xn)
				{
					(*prgb++) = prgba[2];
					(*prgb++) = prgba[1];
					(*prgb++) = prgba[0];

					REFLEX_ASSERT(prgb <= (data.GetData() + data.GetSize()));

					prgba += 4;

					REFLEX_ASSERT(prgba <= ((UInt8*)[nsdata bytes] + [nsdata length]));
				}

				prow += bpr;
			}
		}

		CGImageRelease(imageref);
	}
	else
	{
		info.size = { 0, 0 };

		info.pixdensity = 1;

		rtn->value.b.Clear();
	}
}

REFLEX_INLINE void Reflex::System::OSX::Window::ProcessMouseMove(NS_View * nsview, NSEvent * nsevent, const NSPoint & position)
{
	if (SetFiltered(nsview->m_mousepos, position))
	{
		auto globals = Reflex::System::OSX::globals.Adr();

		auto & client = *nsview->m_client;

		if (globals->m_mousebuttonstate) UpdateModifierFlags(client, nsevent);

		client.OnMouseMove({ToInt32(position.x), ToInt32(nsview->m_xywh.size.h - position.y)});
	}

	if (SetFiltered(nsview->m_queuemousecursor, false))
	{
		[nsview->m_mousecursor set];
	}
}

REFLEX_INLINE void Reflex::System::OSX::Window::ProcessMouseDown(NS_View * nsview, NSEvent * nsevent, UInt8 rmb)
{
	auto globals = Reflex::System::OSX::globals.Adr();

	auto & mb = globals->m_mousebuttonstate;

	if (!mb)
	{
		auto global = TheGlobal::Get();

		[nsview->m_nswindow makeFirstResponder:nsview];	//Plugin needs this bullshit

		nsview->m_mousedownevent = nsevent;

		REFLEX_OBJC_RETAIN(nsevent);

		globals->m_capturewindow = nsview;

		mb = BitSet(mb, rmb);

		nsview->m_client->OnMouseDown(rmb, [nsevent clickCount] > 1);

		st_eventsuppressioninterval = CGEventSourceGetLocalEventsSuppressionInterval(global->m_eventsourceref);

		CGEventSourceSetLocalEventsSuppressionInterval(global->m_eventsourceref, 0.0f);

		[nsview RefreshMouseCursor];
	}
}

REFLEX_INLINE void Reflex::System::OSX::Window::ProcessMouseUp(NS_View * nsview, NSEvent * nsevent, UInt8 rmb)
{
	auto globals = Reflex::System::OSX::globals.Adr();

	auto & mb = globals->m_mousebuttonstate;

	if (BitCheck(mb, rmb))
	{
		auto global = TheGlobal::Get();

		mb = 0;

		nsview->m_client->OnMouseUp(rmb);

		CGEventSourceSetLocalEventsSuppressionInterval(global->m_eventsourceref, st_eventsuppressioninterval);

		REFLEX_OBJC_RELEASE(nsview->m_mousedownevent);

		nsview->m_mousedownevent = 0;

		[nsview RefreshMouseCursor];
	}
}

Reflex::Float64 g_timestamp = 0.0;

REFLEX_INLINE bool Reflex::System::OSX::Window::ProcessKeyDown(Client & client, NS_View * nsview, NSEvent * event)
{
	auto globals = Reflex::System::OSX::globals.Adr();

	auto keycodes = [event charactersIgnoringModifiers];

	if ([keycodes length])
	{
		auto keyinfo = globals->TranslateKey([keycodes characterAtIndex:0]);

		auto keycode = keyinfo.a;

		bool & trap = globals->m_keytrap[keycode];

		if (SetFiltered(g_timestamp, [event timestamp]))
		{
			bool standalone = !nsview->m_host_view;

			bool cmd = True(globals->m_modifier_flags & kModifierKeySystem);

			if (standalone && cmd && keycode == kKeyCodeQ)
			{
				client.OnRequestClose();

				trap = true;
			}
			else
			{
				globals->m_keystate[keycode] = true;

				if ((trap = client.OnKeyPress(keycode, [event isARepeat])))
				{
					if (auto characters = [event characters])
					{
						if (keyinfo.b && !cmd) client.OnCharacter([characters characterAtIndex:0]);
					}
				}

				trap = trap || (standalone && !cmd);
			}
		}

		return trap;
	}

	return false;
}

REFLEX_INLINE void Reflex::System::OSX::Window::UpdateModifierFlags(Client & client, NSEvent * event)
{
	auto globals = Reflex::System::OSX::globals.Adr();

	UInt32 macos_modifier_flags = UInt32(event.modifierFlags);

	auto & modifier_flags = globals->m_modifier_flags;

	auto current = modifier_flags;

	if (SetFiltered(modifier_flags, UInt8((macos_modifier_flags >> 17) & 0xF)))
	{
		//modifierkeys = flags;

		REFLEX_LOOP(idx, 4)
		{
			if (BitCheck(modifier_flags, idx) > BitCheck(current, idx))
			{
				client.OnKeyPress(kKeyCodeNull, false);
			}
			else if (BitCheck(modifier_flags, idx) < BitCheck(current, idx))
			{
				client.OnKeyRelease(kKeyCodeNull);
			}
		}
	}
}




//
//nswindow implementation

@implementation NS_Window
{
}

- (void) dealloc
{
	if (m_nsview) m_nsview->m_nswindow = 0;

#if !__has_feature(objc_arc)
	[super dealloc];

	self = 0;
#endif
}

- (BOOL) canBecomeMainWindow
{
	return true;
}

- (BOOL) canBecomeKeyWindow
{
	return true;
}

- (void) setStyleMask:(NSUInteger)style
{
	REFLEX_USE(Reflex);

	if (m_nsview)
	{
		bool fullscreen = True(style & NSWindowStyleMaskFullScreen) && m_nsview->m_can_be_fullscreen;

		if (SetFiltered(m_nsview->m_fullscreen, fullscreen))
		{
			int flags[] = {NSApplicationPresentationDefault, NSApplicationPresentationHideMenuBar|NSApplicationPresentationHideDock};

			[[NSApplication sharedApplication] setPresentationOptions:flags[fullscreen]];
		}

		m_nsview->m_displaymode = fullscreen ? System::kWindowDisplayFullScreen : System::kWindowDisplayWindowed;

		[super setStyleMask:style];
	}
}

- (BOOL) windowShouldClose:(id)window
{
	m_nsview->m_client->OnRequestClose();

	return false;
}

@end




//
//nsview implementation

void HACK_AlignSubViews(NSView * nsview)
{
	NSSize size = [nsview frame].size;

	NSRect frame = {0,0, size.width, size.height};

	NSArray * subviews = [nsview subviews];

    for (NSView * subview in subviews)
    {
		[subview setFrame:frame];
	}
}

@implementation NS_View
{
}

- (void)dealloc
{
#if !__has_feature(objc_arc)
	[super dealloc];

	self = 0;
#endif
}

- (id) init:(Reflex::System::OSX::Window*)window :(Reflex::System::Window::Client*)client :(Reflex::UInt32)style :(bool)ontop :(void*)host_window
{
	REFLEX_USE(Reflex);

	REFLEX_USE(System);



	//global

	m_global = The<OSX::Window::Global>::Acquire().Adr();

	Retain(*m_global);

	bool mainwindow = SetFiltered(m_global->m_first_window, false);



	//self

	m_nswindow = 0;

	m_client = TRef(client);

	//m_trackingarea.Clear = 0;


	m_displaymode = kWindowDisplayMinimised;

	m_fullscreen = false;

	m_can_be_fullscreen = false;

	m_ignore_theme = true;

	Int32 * prect = &m_xywh.origin.x;

	REFLEX_LOOP(idx, 4) prect[idx] = 0;

	m_dpifactor = 1;


	m_mousecursor = m_global->m_osxcursor[kMouseCursorArrow];

	m_mousepos = NSMakePoint(-1.0f, -1.0f);



	//nsview

	if ((self = [self initWithFrame:NSMakeRect(0, 0, 0, 0)]))
	{
		if (host_window)
		{
			m_host_view = (__bridge NSView*)(host_window);

			//this is for VST case, not AU

			[m_host_view addSubview:self];

			[self setAutoresizingMask:(NSViewWidthSizable | NSViewHeightSizable)];
		}
		else
		{
			NSUInteger osxstyle = OSX::Window::ConvertStyleFlags(style);

			auto nswindow = [[NS_Window alloc] initWithContentRect:NSMakeRect(0,0,0,0) styleMask:osxstyle backing:NSBackingStoreBuffered defer:false];

			nswindow->m_nsview = self;

			m_nswindow = nswindow;

			if (style == (kWindowStyleFrame|kWindowStyleResizable|kWindowStyleMinimisable))
			{
				m_can_be_fullscreen = mainwindow;

				if (m_can_be_fullscreen) [m_nswindow setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
			}

			m_ontop = ontop;

			[m_nswindow setLevel:ontop ? NSPopUpMenuWindowLevel : NSNormalWindowLevel];

			[m_nswindow setContentView:self];	//done by opengl

			[m_nswindow setReleasedWhenClosed:false];

			[m_nswindow setOpaque:true];

			[m_nswindow setTabbingMode:NSWindowTabbingModeDisallowed];

			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:m_nswindow];

			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object:m_nswindow];

			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidMiniaturize:) name:NSWindowDidMiniaturizeNotification object:m_nswindow];

			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidDeminiaturize:) name:NSWindowDidDeminiaturizeNotification object:m_nswindow];

			[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidMove:) name:NSWindowDidMoveNotification object:m_nswindow];

			[m_nswindow makeFirstResponder:self];
		}

		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onResize:) name:NSViewFrameDidChangeNotification object:self];



		//enable drag

		[self registerForDraggedTypes:@[NSPasteboardTypeFileURL,NSPasteboardTypeRTF,NSPasteboardTypeString]];

		//values
	}

	return self;
}

- (BOOL) isOpaque{return true;}
- (BOOL) isFlipped{return false;}
- (BOOL) acceptsFirstMouse:(NSEvent*)event {return YES;}
- (BOOL) acceptsFirstResponder{return YES;}

- (void) viewDidMoveToWindow
{
	REFLEX_USE(Reflex);

	m_nswindow = [self window];

	if (m_nswindow)		//also gets called on leaving window!
	{
		m_dpifactor = System::OSX::g_enableretina ? Truncate([m_nswindow backingScaleFactor]) : 1;

		[m_nswindow makeFirstResponder:self];

		if (m_host_view)
		{
			auto notification_center = [NSNotificationCenter defaultCenter];

			[notification_center addObserver:self selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object:m_nswindow];

			[notification_center addObserver:self selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object:m_nswindow];

			if (SetFiltered(m_displaymode, System::kWindowDisplayWindowed)) [self onResize:nullptr];
		}
	}
}

- (void)viewDidChangeEffectiveAppearance
{
	REFLEX_USE(Reflex)

	[super viewDidChangeEffectiveAppearance];

	if (SetFiltered(m_ignore_theme, false)) return;

	System::OSX::globals->m_signals[System::kNotificationChangeDisplays].Notify();
}

- (void)viewWillStartLiveResize
{
	REFLEX_USE(Reflex);

	auto imin = m_client->OnGetContentSize();

	NSSize min = {Float32(imin.w), Float32(imin.h)};

	[m_nswindow setContentMinSize:min];
}

- (void)mouseEntered:(NSEvent*)event
{
	if (!Reflex::System::OSX::globals->m_mousebuttonstate)
	{
		m_client->OnMouseEnter();

		[self RefreshMouseCursor];
	}

	//[self RefreshMouseCursor];
}

- (void)mouseExited:(NSEvent*)event
{
	if (!Reflex::System::OSX::globals->m_mousebuttonstate)
	{
		m_client->OnMouseLeave();
	}
	else
	{
		[self RefreshMouseCursor];
	}
}

-(void)cursorUpdate:(NSEvent*)event
{
	[m_mousecursor set];
}

- (void)mouseDown:(NSEvent*)event
{
	Reflex::System::OSX::Window::ProcessMouseDown(self, event, false);
}

- (void)rightMouseDown:(NSEvent*)event
{
	Reflex::System::OSX::Window::ProcessMouseDown(self, event, true);
}

- (void)mouseUp:(NSEvent*)event
{
	Reflex::System::OSX::Window::ProcessMouseUp(self, event, false);
}

- (void)rightMouseUp:(NSEvent*)event
{
	Reflex::System::OSX::Window::ProcessMouseUp(self, event, true);
}

- (void)mouseMoved:(NSEvent*)event
{
 	NSPoint nsmousepos = [self convertPoint:event.locationInWindow fromView:0]; //this is buggy see reaper with scrollbar

	Reflex::System::OSX::Window::ProcessMouseMove(self, event, nsmousepos);
}

- (void)mouseDragged:(NSEvent*)event
{
	[self mouseMoved:event];
}

- (void)rightMouseDragged:(NSEvent*)event
{
	[self mouseMoved:event];
}

- (void)scrollWheel:(NSEvent*)event
{
	REFLEX_USE(Reflex);

	const Float32 kLineStep = 32.0f;

	const bool precise = [event hasPreciseScrollingDeltas];

	const bool inverted = [event isDirectionInvertedFromDevice];

	auto dx = Float32(precise ? [event scrollingDeltaX] : [event deltaX] * kLineStep);

	auto dy = Float32(precise ? [event scrollingDeltaY] : [event deltaY] * kLineStep);

	System::fPoint delta = { dx, dy };

	if (Reinterpret<UInt64>(delta) == 0) return;

	m_client->OnMouseWheel(delta, precise, inverted);
}

-(void)keyDown:(NSEvent*)event
{
	bool trap = Reflex::System::OSX::Window::ProcessKeyDown(*m_client, self, event);

	if (!trap) [super keyDown:event];
}

- (BOOL)performKeyEquivalent:(NSEvent*)event
{
	return Reflex::System::OSX::Window::ProcessKeyDown(*m_client, self, event);
}

-(void)keyUp:(NSEvent*)event
{
	auto globals = Reflex::System::OSX::globals.Adr();

	bool * keystate = globals->m_keystate;

	NSString * characters = [event characters];

	if ([characters length])
	{
		auto keyinfo = globals->TranslateKey([characters characterAtIndex:0]);

		auto keycode = keyinfo.a;

		if (keystate[keycode])
		{
			keystate[keycode] = false;

			m_client->OnKeyRelease(keycode);
		}

		globals->m_keytrap[keycode] = false;
	}

	[super keyUp:event];
}

- (void)flagsChanged:(NSEvent*)event
{
	Reflex::System::OSX::Window::UpdateModifierFlags(*m_client, event);

	[super flagsChanged:event];
}

- (void)drawRect:(NSRect)rect
{
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
	return NSDragOperationGeneric;
}

- (BOOL)performDragOperation:(id < NSDraggingInfo >)sender;
{
	REFLEX_USE(Reflex);

	NSPoint nsmousepos = [self convertPoint:[sender draggingLocation] fromView:nil];

	Reflex::System::OSX::Window::ProcessMouseMove(self, [NSApp currentEvent], nsmousepos);

	NSPasteboard * pasteboard = [sender draggingPasteboard];

	NSArray<NSURL *> * urls = [pasteboard readObjectsForClasses:@[[NSURL class]] options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];

	if (urls.count > 0)
	{
		auto files_ref = Make< ObjectOf < Array <WString> > >();

		for (NSURL * url in urls)
		{
			files_ref->value.Push(System::ToWString(url.path));
		}

		m_client->OnDrop(files_ref);
	}
	else if (NSString * nsstring = [pasteboard stringForType:NSPasteboardTypeString])
	{
		auto cstring_ref = Make< ObjectOf <CString> >(System::ToCStringView(nsstring));

		m_client->OnDrop(cstring_ref);
	}

	return YES;
}

- (void)concludeDragOperation:(id<NSDraggingInfo>)sender
{
	REFLEX_USE(Reflex);

	[m_nswindow makeKeyAndOrderFront:self];

	System::OSX::globals->m_mousebuttonstate = 0;
}

- (NSDragOperation)draggingSession:(NSDraggingSession *)session sourceOperationMaskForDraggingContext:(NSDraggingContext)context
{
	return NSDragOperationCopy;
}

- (void)windowDidMiniaturize:(NSNotification *) notification
{
	REFLEX_USE(Reflex);

	if (SetFiltered(m_displaymode, System::kWindowDisplayMinimised)) [self onResize:nullptr];
}

- (void)windowDidDeminiaturize:(NSNotification *) notification
{
	REFLEX_USE(Reflex);

	if (SetFiltered(m_displaymode, System::kWindowDisplayMinimised)) [self onResize:nullptr];
}

- (void)windowDidBecomeKey:(NSNotification *) notification
{
	DEV_LOG("windowDidBecomeKey");

	auto & client = *m_client;

	Reflex::System::OSX::Window::UpdateModifierFlags(client, [NSApp currentEvent]);

	[m_nswindow makeFirstResponder:self];

	client.OnSetFocus();
}

- (void)windowDidResignKey:(NSNotification *) notification
{
	DEV_LOG("windowDidResignKey");

	auto globals = Reflex::System::OSX::globals.Adr();

	auto & client = *m_client;

	[self mouseUp:0];

	[self rightMouseUp:0];

	REFLEX_LOOP(keycode, Reflex::System::kNumKeyCode)
	{
		globals->ProcessKey(client, Reflex::System::KeyCode(keycode), false, false);
	}

	client.OnLoseFocus();
}

- (void)updateTrackingAreas
{
	DEV_LOG("NSView updateTrackingAreas");

	[self removeTrackingArea:m_trackingarea];

	int trackingoptions = NSTrackingActiveAlways | NSTrackingMouseEnteredAndExited | NSTrackingCursorUpdate | NSTrackingMouseMoved;

	m_trackingarea = Reflex::System::MakeOwnedObjCRef([[NSTrackingArea alloc] initWithRect:[self frame] options:trackingoptions owner:self userInfo:nil]);

	[self addTrackingArea:m_trackingarea];

	[super updateTrackingAreas];
}

- (void)windowDidMove:(NSNotification *) notification
{
	[self onResize:nullptr];
}

- (void)onResize:(NSNotification *) notification
{
	REFLEX_USE(Reflex);

	if (m_host_view)
	{
		NSRect rect = [self frame];

		if (rect.size.width * rect.size.height)
		{
			m_xywh = { {0,0}, { Truncate(rect.size.width), Truncate(rect.size.height) } };

			m_client->OnSetRect(m_displaymode, m_xywh, { { 0,0 }, m_xywh.size }, m_dpifactor);
		}
	}
	else
	{
		auto globals = Reflex::System::OSX::globals.Adr();

		NSRect rect = [m_nswindow contentRectForFrameRect:[m_nswindow frame]];

		m_xywh.origin = { Truncate(rect.origin.x), globals->m_desktopheight - Truncate(rect.origin.y + rect.size.height) };

		m_xywh.size = { Truncate(rect.size.width), Truncate(rect.size.height) };

		m_client->OnSetRect(m_displaymode, m_xywh, { { 0,0 }, m_xywh.size }, m_dpifactor);

		HACK_AlignSubViews(self);	//for when used as vst window by some shitty plugins
	}

	m_mousepos = NSMakePoint(-1.0f, -1.0f);

	NSPoint nsmousepos = [m_nswindow convertPointFromScreen:[NSEvent mouseLocation]];

	System::OSX::Window::ProcessMouseMove(self, [NSApp currentEvent], [self convertPoint:nsmousepos fromView:0]);
}

-(void) RefreshMouseCursor
{
	[m_mousecursor set];

	[m_nswindow invalidateCursorRectsForView:self];

	m_queuemousecursor = true;
}

@end
