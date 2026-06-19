#pragma once

#include "abstract_viewbar.h"




//
//Primary API

namespace Reflex::GLX
{

	class AbstractViewPort;


	class Scroller;

	class Zoomable;

}




//
//AbstractViewPort

class Reflex::GLX::AbstractViewPort : public Object
{
public:

	REFLEX_OBJECT(GLX::AbstractViewPort,Object);

	static AbstractViewPort & null;



	//types

	struct ViewState;

	using ViewBarCtr = FunctionPointer <TRef<AbstractViewBar>(AbstractViewPort&)>;



	//standard viewbars

	static const ViewBarCtr kNullBar, kScrollBar, kZoomBar;



	//lifetime

	~AbstractViewPort();



	//setup

	void SetViewBarCtr(bool y, ViewBarCtr ctr);

	void InvertScrollAxis(bool enable = true);



	//content

	void SetContent(TRef <Object> content, Key32 style_id = kcontent);

	TRef <Object> GetContent();

	ConstTRef <Object> GetContent() const { return *RemoveConst(this)->GetContent(); }



	//visible area

	[[nodiscard]] TRef <Reflex::Object> CreateListener(const Function <void()> & callback);


	void SetMinView(Size size);

	Size GetMinView() const;


	Size GetExtent() const;


	void SetView(const Rect & view);

	const Rect & GetView() const;


	Size GetPixelsPerUnit() const;



	//scroll

	void StartScroll(bool yaxis, Float offset);

	void StopScroll(bool yaxis);


	void Reveal(bool yaxis, Float offset, Float range, Float padding, bool animate = true);


	void EnableAutoScroll(Float amount = 0.95f, bool scoped = false);

	void DisableAutoScroll();



	//components

	ConstTRef <Object> GetBody() const;

	TRef <AbstractViewBar> GetViewBar(bool y);



	//info

	const bool zoomable;

	const Pair <bool> inverted;



	//view

	const TRef <ViewState> view_state;



protected:

	AbstractViewPort(bool zoomable);

	void InvertAxis(bool invertx, bool inverty);

	virtual void OnSetStyle(const Style & style) override;

	virtual bool OnEvent(Object & src, Event & e) override;



public: //TODO merge & cleanup
	
	struct Impl { UInt8 bytes[576]; } m_impl;	//TODO / TRANSTIONAL move to protected

};

REFLEX_SET_TRAIT(Reflex::GLX::AbstractViewPort, IsSingleThreadExclusive);




//
//Scroller

class Reflex::GLX::Scroller : public AbstractViewPort
{
public:

	REFLEX_OBJECT(GLX::Scroller, AbstractViewPort);

	static Scroller & null;

	Scroller();

};

REFLEX_SET_TRAIT(Reflex::GLX::Scroller, IsSingleThreadExclusive);




//
//Zoomable

class Reflex::GLX::Zoomable : public AbstractViewPort
{
public:

	REFLEX_OBJECT(GLX::Zoomable, AbstractViewPort);

	static Zoomable & null;

	Zoomable();

	[[deprecated]] Zoomable(bool invertx, bool inverty);

	using AbstractViewPort::InvertAxis;

};
	
REFLEX_SET_TRAIT(Reflex::GLX::Zoomable, IsSingleThreadExclusive);


	

//
//impl

struct Reflex::GLX::AbstractViewPort::ViewState :
	public Reflex::Object,
	public Reflex::State,
	public Signal <>
{
	static ViewState & null;

	
	ViewState(AbstractViewPort & owner);

	virtual void SetMinView(const Size & size) = 0;

	virtual Size GetMinView() const = 0;

	virtual Size GetExtent() const = 0;

	virtual void SetView(const Rect & visible) = 0;

	virtual const Rect & GetView() const = 0;

	virtual const Size & GetPixelsPerUnit() const = 0;


	const TRef <AbstractViewPort> owner;
};

REFLEX_SET_TRAIT(Reflex::GLX::AbstractViewPort::ViewState, IsSingleThreadExclusive);

inline void Reflex::GLX::AbstractViewPort::SetMinView(Size size)
{
	view_state->SetMinView(size);
}

inline Reflex::GLX::Size Reflex::GLX::AbstractViewPort::GetMinView() const
{
	return view_state->GetMinView();
}

inline Reflex::GLX::Size Reflex::GLX::AbstractViewPort::GetExtent() const
{
	return view_state->GetExtent();
}

inline void Reflex::GLX::AbstractViewPort::SetView(const Rect & visible)
{
	view_state->SetView(visible);
}

inline const Reflex::GLX::Rect & Reflex::GLX::AbstractViewPort::GetView() const
{
	return view_state->GetView();
}

inline Reflex::GLX::Size Reflex::GLX::AbstractViewPort::GetPixelsPerUnit() const
{
	return view_state->GetPixelsPerUnit();
}

inline Reflex::GLX::Scroller::Scroller()
	: AbstractViewPort(false)
{
}

inline Reflex::GLX::Zoomable::Zoomable()
	: AbstractViewPort(true)
{
}

inline Reflex::GLX::Zoomable::Zoomable(bool invertx, bool inverty)
	: AbstractViewPort(true)
{
	InvertAxis(invertx, inverty);
}
