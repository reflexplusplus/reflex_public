#pragma once

#include "sdk.h"
#include "../common/instance/plugin.h"




//
//declarations

@class NS_Window;

@class NS_View;

REFLEX_NS(Reflex::System::OSX)

class Window;

REFLEX_END




//
//Window

class Reflex::System::OSX::Window : public Reflex::System::Window
{
public:

	REFLEX_OBJECT(Window, System::Window);

	struct Global;

	typedef Reflex::The <Global> TheGlobal;


	Window(UInt32 flags, bool ontop, void * host_window);

	virtual ~Window() override;


	virtual void SetClient(TRef <Client> client) override;

	virtual TRef <Client> GetClient() override;


	virtual void Attach(System::Window & parent) override;

	virtual void Detach() override;


	virtual void SetTitle(const WString & label) override;


	virtual void SetDisplayMode(WindowDisplay mode) override;

	virtual void SetRect(const iRect & rect) override;

	virtual void SendTop() override;


	virtual void EnableInput(bool enable) override;

	virtual void SetMouseCursor(MouseCursor mousecursor) override;

	virtual void SetMousePosition(const iPoint & position) override;


	virtual void BeginDragDropFiles(const ArrayView <WString> & filenames) override;


	virtual TRef < ObjectOf <RawBitmap> > CreateExportBitmapBuffer(UInt8 flags = 0) const override;

	virtual void ExportBitmap(TRef < ObjectOf <RawBitmap> > buffer) const override;


	virtual void * GetSystemHandle() override { return (__bridge void*)m_nsview; }


	static NSUInteger ConvertStyleFlags(UInt32 flags);

	static void ProcessMouseMove(NS_View * nsview, NSEvent * event, const NSPoint & position);

	static void ProcessMouseDown(NS_View * nsview, NSEvent * event, UInt8 rmb);

	static void ProcessMouseUp(NS_View * nsview, NSEvent * event, UInt8 rmb);

	static bool ProcessKeyDown(Client & client, NS_View * nsview, NSEvent * event);

	static void UpdateModifierFlags(Client & client, NSEvent * event);

	static void DeferWorkaround(void * pself);
	

	NS_View * m_nsview;
	
	
	static double st_eventsuppressioninterval;

};

struct Reflex::System::OSX::Window::Global : public Reflex::Object
{
	Global();

	~Global();

	void InitCursor(MouseCursor cursor, const ArrayView <UInt8> & item, const NSPoint & point);

	void InitCursor(MouseCursor cursor, const ArrayView <UInt8> & lores, const ArrayView <UInt8> & hires, const NSPoint & point);


	CGEventSourceRef m_eventsourceref;

	ObjCRef <NSCursor*> m_osxcursor[System::kNumMouseCursor];
	
	bool m_first_window = true;
};




//
//NS_Window

@interface NS_Window : NSWindow
{
@public

	NS_View * m_nsview;

}

- (void) dealloc;

- (BOOL) canBecomeMainWindow;
- (BOOL) canBecomeKeyWindow;
- (void) setStyleMask:(NSUInteger)styleMask;
- (BOOL) windowShouldClose:(id)window;

@end




//
//NS_View

@interface NS_View : NSView <NSDraggingSource, NSDraggingDestination>
{
@public

	Reflex::System::OSX::Window::Global * m_global;

	NSView * m_host_view;

	Reflex::Reference <Reflex::System::Window::Client> m_client;

	NSWindow * m_nswindow;


	Reflex::System::WindowDisplay m_displaymode;

	bool m_ontop;

	bool m_fullscreen;

	bool m_can_be_fullscreen;	//another apple workaround

	bool m_ignore_theme;

	Reflex::System::iRect m_xywh;

	Reflex::Int32 m_dpifactor;


	Reflex::System::ObjCRef <NSTrackingArea*> m_trackingarea;

	NSCursor * m_mousecursor;

	NSPoint m_mousepos;

	NSEvent * m_mousedownevent;


	bool m_queuemousecursor;

}

- (void) dealloc;
- (id) init:(Reflex::System::OSX::Window*)window :(Reflex::System::Window::Client*)client :(Reflex::UInt32)style :(bool)ontop :(void*) host_window;

- (BOOL) isOpaque;
- (BOOL) isFlipped;
- (BOOL) acceptsFirstMouse:(NSEvent*)event;
- (BOOL) acceptsFirstResponder;

- (void) viewDidMoveToWindow;
- (void) viewWillStartLiveResize;
- (void) onResize:(NSNotification*) notification;
- (void) updateTrackingAreas;
- (BOOL) performKeyEquivalent:(NSEvent*)event;
- (void) keyDown:(NSEvent*)event;
- (void) keyUp:(NSEvent*)event;
- (void) flagsChanged:(NSEvent*)event;
- (void) mouseEntered:(NSEvent*)event;
- (void) mouseExited:(NSEvent*)event;
- (void) mouseDown:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;
- (void) mouseUp:(NSEvent*)event;
- (void) rightMouseUp:(NSEvent*)event;
- (void) mouseMoved:(NSEvent*)event;
- (void) mouseDragged:(NSEvent*)event;
- (void) rightMouseDragged:(NSEvent*)event;
- (void) scrollWheel:(NSEvent*)event;
- (void) drawRect:(NSRect)rect;

- (NSDragOperation) draggingEntered:(id <NSDraggingInfo>)sender;
- (BOOL) performDragOperation:(id < NSDraggingInfo >)sender;
- (void) concludeDragOperation:(id<NSDraggingInfo>)sender;

//window related

- (void) windowDidMiniaturize:(NSNotification *) notification;
- (void) windowDidDeminiaturize:(NSNotification *) notification;
- (void) windowDidMove:(NSNotification *) notification;
- (void) windowDidBecomeKey:(NSNotification *) notification;
- (void) windowDidResignKey:(NSNotification *) notification;

//fixes

- (void) RefreshMouseCursor;

@end
