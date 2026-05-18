#pragma once

#include "types.h"




//
//Secondary API

namespace Reflex::System
{

	enum WindowStyles : UInt8;

	enum WindowDisplay : UInt8;

	enum WindowInput : UInt8;

	enum MouseCursor : UInt8;

	enum ScreenOrientation : UInt8;

	class Window;

}




//
//WindowStyles

enum Reflex::System::WindowStyles : Reflex::UInt8
{
	kWindowStyleFrame = 1,
	kWindowStyleMinimisable = 2,
	kWindowStyleResizable = 4,
	kWindowStyleCloseable = 8,
};




//
//WindowDisplay

enum Reflex::System::WindowDisplay : Reflex::UInt8
{
	kWindowDisplayMinimised,
	kWindowDisplayWindowed,
	kWindowDisplayFullScreen,
};




//
//WindowInput

enum Reflex::System::WindowInput : Reflex::UInt8
{
	kWindowInputNone,
	kWindowInputPassive,	//default, receive input when focused
	kWindowInputActive,		//enable this to actively steal keys from host
};




//
//MouseCursor

enum Reflex::System::MouseCursor : Reflex::UInt8
{
	kMouseCursorInvisible,
	kMouseCursorArrow,
	kMouseCursorWait,
	kMouseCursorMove,
	kMouseCursorLeftRight,
	kMouseCursorTopBottom,
	kMouseCursorTopLeftBottomRight,
	kMouseCursorBottomLeftTopRight,
	kMouseCursorPointer,
	kMouseCursorDrag,
	kMouseCursorText,
	kMouseCursorBlock,
	kMouseCursorZoom,

	kNumMouseCursor,
};




//
//ScreenOrientation

enum Reflex::System::ScreenOrientation : Reflex::UInt8
{
	kScreenOrientationDefault,
	kScreenOrientationPortrait,
	kScreenOrientationLandscape,
};



//
//Window

class Reflex::System::Window : public Object
{
public:

	REFLEX_OBJECT(System::Window, Object);

	static Window & null;



	//types

	class Client;



	//lifetime

	[[nodiscard]] static TRef <Window> Create(UInt32 styleflags, bool topmost = false, void * host_window = nullptr);



	//callbacks

	virtual void SetClient(TRef <Client> client) = 0;

	virtual TRef <Client> GetClient() = 0;



	//location

	virtual void Attach(Window & parent) = 0;

	virtual void Detach() = 0;



	//setup

	virtual void SetTitle(const WString & label) = 0;



	//view

	virtual void SetDisplayMode(WindowDisplay mode) = 0;

	virtual void SetRect(const iRect & rect) = 0;

	virtual void SendTop() = 0;



	//input

	virtual void EnableInput(bool enable) = 0;

	virtual void SetMouseCursor(MouseCursor mouse_cursor) = 0;

	virtual void SetMousePosition(const iPoint & point) = 0;



	//drag

	virtual void BeginDragDropFiles(const ArrayView <WString> & filenames) = 0;



	//screencapture

	virtual TRef < ObjectOf <RawBitmap> > CreateExportBitmapBuffer(UInt8 flags = 0) const = 0;

	virtual void ExportBitmap(TRef < ObjectOf <RawBitmap> > buffer) const = 0;



	//system window pointer

	virtual void * GetSystemHandle() = 0;

};




//
//Window::Client

class Reflex::System::Window::Client : public Object
{
public:

	static Client & null;

	virtual void OnSetOwner(Window * window) = 0;


	virtual ScreenOrientation OnGetScreenOrientation() = 0;

	virtual iSize OnGetContentSize() = 0;

	
	virtual void OnSetRect(WindowDisplay displaymode, const iRect & rect, const iRect & interactable, Int32 dpifactor) = 0;


	virtual void OnSetFocus() = 0;

	virtual void OnLoseFocus() = 0;


	virtual void OnMouseEnter() = 0;

	virtual void OnMouseLeave() = 0;

	virtual void OnMouseMove(iPoint position) = 0;

	virtual void OnMouseDown(bool rmb, bool dbl) = 0;

	virtual void OnMouseUp(bool rmb) = 0;

	virtual void OnMouseWheel(fPoint delta, bool high_res, bool inverted) = 0;


	virtual void OnTouchBegin(UIntNative finger_id, fPoint position) = 0;

	virtual void OnTouchMove(UIntNative finger_id, fPoint position) = 0;

	virtual void OnTouchEnd(UIntNative finger_id) = 0;

	virtual void OnTouchCancel(UIntNative finger_id) = 0;


	virtual bool OnKeyPress(KeyCode keycode, bool repeat) = 0;

	virtual void OnKeyRelease(KeyCode keycode) = 0;

	virtual bool OnCharacter(WChar character) = 0;


	virtual void OnDrop(const Object & object) = 0;


	virtual void OnRequestClose() = 0;

};
