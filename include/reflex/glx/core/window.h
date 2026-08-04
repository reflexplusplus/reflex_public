#pragma once

#include "object.h"




//
//Secondary API (Primary: GLX::WindowClient)

class Reflex::GLX::Core::WindowClient : public Reflex::Item <GLX::WindowClient,false,System::Window::Client>
{
public:

	//global

	static TRef <GLX::WindowClient> GetCurrent_DEPRECATED();	//TODO remove, only needed by drag and drop



	//lifetime

	WindowClient(bool profile);

	virtual ~WindowClient();



	//view

	void SetTitle(const WString & title);


	void SetBackgroundColour(const System::Colour & colour);

	const System::Colour & GetBackgroundColour() const;


	void SetDisplayMode(System::WindowDisplay state);

	System::WindowDisplay GetDisplayMode() const;


	void SetRect(const System::fRect & rect);

	const System::fRect & GetRect() const;

	Float32 GetPixelDensity() const;


	void SetContent(GLX::Object & object);

	TRef <GLX::Object> GetContent();



	//mouse

	void SetMousePosition(System::fPoint position);

	System::fPoint GetMousePosition() const;


	System::MouseCursor GetMouseCursor() const;



	//drag

	void BeginDragDrop(const ArrayView <WString> & files);



	//force redraw

	void Refresh();



	//debug

	enum DebugCounters
	{
		kDebugCounterClock,
		kDebugCounterUpdate,
		kDebugCounterRebuild,
		kDebugCounterAccommodate,
		kDebugCounterAlign,
		kDebugCounterAlignResponsive,
		
		kNumDebugCounter
	};

	const UInt16 * GetDebugCounters() const;



	//links

	const TRef <System::Window> owner;

	const ConstTRef <Output::Profiler> profiler;



protected:

	//callbacks

	virtual void OnClose() = 0;



	//system callbacks (handled by this class)

	virtual void OnSetOwner(System::Window * window) override;


	virtual void OnSetRect(System::WindowDisplay state, const System::iRect & rect, const System::iRect & interactable, Int32 dpifactor) override;


	virtual void OnSetFocus() final;

	virtual void OnLoseFocus() final;


	virtual void OnMouseEnter() final;

	virtual void OnMouseLeave() final;

	virtual void OnMouseMove(System::iPoint position) final;

	virtual void OnMouseDown(bool rmb, bool dbl) final;

	virtual void OnMouseUp(bool rmb) final;

	virtual void OnMouseWheel(System::fPoint delta, bool high_res, bool inverted) final;


	virtual void OnTouchBegin(UIntNative touch_id, System::fPoint position) final;

	virtual void OnTouchMove(UIntNative touch_id, System::fPoint position) final;

	virtual void OnTouchEnd(UIntNative touch_id) final;

	virtual void OnTouchCancel(UIntNative touch_id) final;


	virtual bool OnKeyPress(System::KeyCode keycode, bool repeat) final;

	virtual void OnKeyRelease(System::KeyCode keycode) final;

	virtual bool OnCharacter(WChar character) final;


	virtual void OnDrop(const Reflex::Object & object) final;


	virtual void OnRequestClose() final;


	virtual void OnUnsetProperty(Address address) override;

	virtual void OnSetProperty(Address address, Object & value) override;

	virtual void OnQueryProperty(Address address, Object * & pointer) const override;


	virtual void OnDestruct() override;



	//lifetime

	WindowClient();	//for null window ctor



private:

	friend struct Accessor;

	friend Core::Object;

	struct Event;



	//storage generic properties

	mutable Data::PropertySet::SequenceType m_properties;



	//graphics

	Reference <System::Renderer::Canvas> m_canvas;

	System::Colour m_background_colour;



	//content

	Reference <GLX::Object> m_content;

	System::WindowDisplay m_display_mode;

	System::fRect m_rect;

	Tuple <System::iSize,Int8,bool> m_canvas_size;



	//mouse & key

	UIntNative m_current_touchid;	//todo multi-touch support

	System::fPoint m_mousepos;

	System::MouseCursor m_mousecursor;



	//debug

	UInt16 m_debug_counters[kNumDebugCounter];

};




//
//impl

inline void Reflex::GLX::Core::WindowClient::SetBackgroundColour(const System::Colour & colour)
{
	m_background_colour = colour;
}

inline const Reflex::System::Colour & Reflex::GLX::Core::WindowClient::GetBackgroundColour() const
{
	return m_background_colour;
}

inline void Reflex::GLX::Core::WindowClient::SetDisplayMode(System::WindowDisplay state)
{
	m_display_mode = state;

	owner->SetDisplayMode(state);
}

inline Reflex::System::WindowDisplay Reflex::GLX::Core::WindowClient::GetDisplayMode() const
{
	return m_display_mode;
}

inline const Reflex::System::fRect & Reflex::GLX::Core::WindowClient::GetRect() const
{
	return m_rect;
}

inline Reflex::Float32 Reflex::GLX::Core::WindowClient::GetPixelDensity() const
{
	return ToFloat32(m_canvas_size.b);
}

inline Reflex::System::MouseCursor Reflex::GLX::Core::WindowClient::GetMouseCursor() const
{
	return m_mousecursor;
}

inline Reflex::System::fPoint Reflex::GLX::Core::WindowClient::GetMousePosition() const
{
	return m_mousepos;
}

inline Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Core::WindowClient::GetContent()
{
	return m_content;
}

inline const Reflex::UInt16 * Reflex::GLX::Core::WindowClient::GetDebugCounters() const
{
	return m_debug_counters;
}
