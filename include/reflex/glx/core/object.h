#pragma once

#include "context.h"




//
//Secondary API (Primary: GLX::Object)

REFLEX_NS(Reflex::GLX::Core)

class Object;

class Renderer;	//TODO -> GLX

REFLEX_END




//
//Core::Object

class Reflex::GLX::Core::Object :
	public Node <GLX::Object,Data::PropertySet>,
	public Reflex::Detail::LegacyWeakReferenceTarget <GLX::Object>
{
public:

	//types

	using AccommodateFn = FunctionPointer <void(GLX::Object & object, bool & isresponsive, System::fSize & contentsize)>;

	using AlignFn = FunctionPointer <void(GLX::Object & object, bool isresponsive, Float & contenth)>;



	//lifetime

	virtual ~Object();



	//properties

	bool ChildrenHaveZIndex() const { return m_draw_with_zindex; }

	Int8 GetZIndex() const;



	//location

	TRef <GLX::WindowClient> GetWindow();

	ConstTRef <GLX::WindowClient> GetWindow() const;


	void SetParent(GLX::Object & parent);

	void InsertBefore(GLX::Object & object);

	void InsertAfter(GLX::Object & object);

	void Detach();


	TRef <GLX::Object> GetParent() { return m_parent; }

	ConstTRef <GLX::Object> GetParent() const { return m_parent; }


	void SendBottom();

	void SendTop();



	//content

	void Clear();



	//coordinates

	void SetPosition(const System::fPoint & point);

	void SetSize(const System::fSize & size);

	void SetRect(const System::fRect & rect);

	const System::fRect & GetRect() const;


	void SetScale(const System::fSize & scale);

	const System::fSize & GetScale() const;



	//mouse

	void SetMouseCursor(System::MouseCursor mouse_cursor);

	System::MouseCursor GetMouseCursor() const;



	//state

	void Focus();



	//enable callbacks

	void EnableOnClock(bool enable = true);

	void EnableOnAttachDetachWindow(bool enable = true);



	//update

	void RebuildLayout();

	void Accommodate();		//request accommodate

	void Realign();			//request realign / redraw

	void Redraw();			//set redraw flag on all parents


	void Update();			//request OnUpdate


	void Refresh();			//flush



	//draw

	void Draw(RenderContext & ctx);				//for draw to texture

	TRef <GLX::Object> FindMouseOver(MouseAction mouseaction, UInt8 flags, const System::fPoint & position);



	//renderer (move to GLX, core should just have draw fn)

	TRef <Renderer> GetRenderer() { return m_renderer; }

	ConstTRef <Renderer> GetRenderer() const { return m_renderer; }



	//computed layout  ( set in OnAccomodate (and OnAlign for height, if responsive) )

	const bool isresponsive = false;

	const System::fSize contentsize;



	//disambig

	using Node::Empty;



protected:

	//lifetime

	Object();



	//renderer TODO merge with GLX StandardLayout, Core::Object just has set draw fn

	void SetRenderer(Renderer & renderer, Int8 zindex);	//TODO INPUT MODEL clean up, OnAlign returns or sets this



	//callbacks (-> TODO move to InputModel)

	virtual void OnClock(Float delta) {}

	virtual void OnAttachWindow() {}

	virtual void OnDetachWindow() {}


	virtual void OnUpdate() {}


	virtual void OnFocus() {}

	virtual void OnLoseFocus() {}


	virtual Trap OnMouseOver(MouseAction mouseaction, UInt8 flags) { return kTrapPassive; }

	virtual void OnMouseEnter(GLX::Object & previous) {}

	virtual void OnMouseLeave() {}


	virtual Trap OnMouseClick(UInt8 flags) { return kTrapThru; }

	virtual void OnMouseDrag(System::fPoint drag) {}

	virtual void OnMouseRelease() {}

	virtual void OnMouseWheel(System::fPoint delta, bool inverted) {}


	virtual bool OnKeyPress(System::KeyCode keycode, bool repeat) { return false; }

	virtual void OnKeyRelease(System::KeyCode keycode) {}

	virtual bool OnCharacter(WChar character) { return false; }


	virtual bool OnDragDropTender(Reflex::Object & data) { return false; }

	virtual void OnDragDropEnter(Reflex::Object & data) {}

	virtual void OnDragDropLeave(Reflex::Object & data) {}

	virtual void OnDragDropReceive(Reflex::Object & data) {}

	virtual bool OnDragDropReceiveExternal(const Reflex::Object & data) { return false; }


	virtual void OnBuildLayout(AccommodateFn & accommodate, AlignFn & align) = 0;

	virtual void OnReleaseData() override;



private:

	friend struct Accessor;

	friend Desktop;

	friend WindowClient;

	struct Internal;


	void FastRefresh();



	TRef <GLX::WindowClient> m_window;

	TRef <GLX::Object> m_parent;	//parent optimisation


	Flags8 m_attach_flags;

	Flags8 m_upward_flags;

	Flags8 m_refresh_flags;

	Flags8 m_refresh_flags_guard;

	System::MouseCursor m_mouse_cursor;

	
	Int8 m_zindex;	//self

	bool m_draw_with_zindex;//children

	bool m_redraw;


	System::fRect m_rect;

	System::fSize m_scale;


	AccommodateFn m_accommodate = nullptr;

	AlignFn m_align = nullptr;

	Reference <Renderer> m_renderer;


	static const FunctionPointer <void(Object&)> * st_updatefns, * st_refreshfns/*, * st_refreshguardfns*/;

};

REFLEX_STATIC_ASSERT(Reflex::kIsObject<Reflex::GLX::Core::Object>);




//
//GLX::Core::Renderer -> TODO move to GLX

class Reflex::GLX::Core::Renderer : public System::Renderer::Graphic
{
public:

	static Renderer & null;

	Renderer(bool responsive) : responsive(responsive) {}

	virtual void OnAccommodate(System::fSize & contentsize) {}

	virtual void OnAlign(const System::fSize & size, Float & contenth) {}

	virtual void OnRedraw(RenderContext & ctx) {}


	const bool responsive;		//TEMP TRANSITION, unifying with Layer state

};




//
//impl

REFLEX_SET_TRAIT(Reflex::GLX::Core::Object, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Reflex::GLX::Core::Renderer, IsSingleThreadExclusive);

REFLEX_INLINE Reflex::TRef <Reflex::GLX::WindowClient> Reflex::GLX::Core::Object::GetWindow()
{
	return m_window;
}

REFLEX_INLINE Reflex::ConstTRef <Reflex::GLX::WindowClient> Reflex::GLX::Core::Object::GetWindow() const
{
	return m_window;
}

REFLEX_INLINE Reflex::System::MouseCursor Reflex::GLX::Core::Object::GetMouseCursor() const
{
	return m_mouse_cursor;
}

REFLEX_INLINE const Reflex::System::fRect & Reflex::GLX::Core::Object::GetRect() const
{
	return m_rect;
}

REFLEX_INLINE const Reflex::System::fSize & Reflex::GLX::Core::Object::GetScale() const
{
	return m_scale;
}

REFLEX_INLINE void Reflex::GLX::Core::Object::Draw(RenderContext & ctx)
{
	REFLEX_ASSERT(Context::IsActive());

	if (m_redraw) m_renderer->OnRedraw(ctx);

	m_renderer->Render(ctx.transform, { 1.0f, 1.0f, 1.0f, 1.0f });
	
	m_redraw = false;
}

REFLEX_INLINE void Reflex::GLX::Core::Object::FastRefresh()
{
	REFLEX_ASSERT(Context::IsActive());

	UInt8 guard = m_refresh_flags_guard.GetWord();

	guard = UInt8(~guard);

	auto & flags = m_refresh_flags.GetWord();

	(*st_updatefns[flags & 3 & guard])(*this);

	(*st_refreshfns[flags & guard])(*this);
}

REFLEX_INLINE void Reflex::GLX::Core::Object::Refresh()
{
	Context context;

	FastRefresh();
}
