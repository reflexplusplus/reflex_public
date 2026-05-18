#pragma once

#include "layout/modes.h"
#include "event/event.h"
#include "detail/computed_style.h"
#include "animation.h"




//
//Primary API

namespace Reflex::GLX
{

	class Object;

}




//
//Object

class Reflex::GLX::Object :
	public Core::Object,
	public Detail::Countable <MakeKey32("Object")>
{
public:

	REFLEX_OBJECT(GLX::Object, Core::Object);

	static Object & null;



	//types

	class Delegate;



	//lifetime

	Object();

	Object(const Object & object) = delete;

	Object(Object && object) = delete;

	Object(Detail::LayoutModelCtr ctr);

	~Object() override;



	//content

	using Core::Object::Clear;



	//setup

	void SetLayoutModel(TRef <Detail::LayoutModel> layout);

	void SetLayoutModel(Detail::LayoutModelCtr ctr) { SetLayoutModel(ctr(*this)); }

	ConstTRef <Detail::LayoutModel> GetLayoutModel() const { return m_layout; }



	//mouse setup

	void SetMouseOverTrapMode(Trap trap);

	Trap GetMouseOverTrapMode() const;


	void SetMouseClickTrapMode(Trap trap);

	Trap GetMouseClickTrapMode() const;



	//events

	void SetDelegate(Key32 id, TRef <Delegate> dlg);	//experimental

	void ClearDelegate(Key32 id);	//deprecated, use Delegate::Detach


	Delegate * QueryDelegate(Reflex::Detail::DynamicTypeRef object_t);	//experimental

	void EnumerateDelegates(const Function <void(Delegate&)> & visitor);

	Array < Reference <Delegate> > GetDelegates();


	bool ProcessEvent(Object & src, Event & e);

	bool Emit(Event & e);

	TRef <Object> EmitEx(Event & e);



	//style

	void SetStyle(const Style & style);

	ConstTRef <Style> GetStyle() const;



	//state

	void ClearState(Key32 state);

	void SetState(Key32 state);

	bool CheckState(Key32 state) const;

	ConstTRef <Style> GetCurrentStyle() const;			//the current applied style, after states are applied



	//mods

	void SetMod(Key32 id, TRef <Detail::ComputedStyle> cstyle);

	void ClearMod(Key32 id);

	ConstTRef <Detail::ComputedStyle> GetMod(Key32 id) const;

	ConstTRef <Detail::ComputedStyle> GetComputedStyle() const;



	//ADVANCED used by standard layout/style, you probably dont need to use these directly

	//flow/layout (affects children)

	void SetLayoutFlags(UInt8 flags);

	const UInt8 & GetLayoutFlags() const { return m_flowflags.GetWord(); }


	void SetPositioningFlags(UInt8 flags);

	UInt8 GetPositioningFlags() const { return m_positioningflags; }


	void ComputeLayout() const;



	//properties

	Key32 id;



protected:

	using Mods = ObjectOf < Map <Key32, ConstReference <Detail::ComputedStyle> > >;



	//forward to delegate

	virtual bool OnEvent(Object & src, Event & e);	//forwards to delegates

	virtual void OnSetStyle(const Style & style);	//forwards to delegates


	void OnAttachWindow() override;

	void OnDetachWindow() override;

	void OnUpdate() override;				//forwards to delegates



	//final callbacks TODO (1) make all final, requires cleanup of old widgets (2) remove, send event from window

	void OnBuildLayout(AccommodateFn & accommodate, AlignFn & align) override;


	void OnFocus() override;

	void OnLoseFocus() final;

	Trap OnMouseOver(Core::MouseAction mouseaction, UInt8 flags) override;

	void OnMouseEnter(Object & previous) final;

	void OnMouseLeave() final;

	Trap OnMouseClick(UInt8 flags) override;

	void OnMouseDrag(Point drag) override;

	void OnMouseRelease() override;

	void OnMouseWheel(Point delta, bool inverted) final;


	bool OnDragDropTender(Reflex::Object & data) final;

	void OnDragDropEnter(Reflex::Object & data) final;

	void OnDragDropLeave(Reflex::Object & data) final;

	void OnDragDropReceive(Reflex::Object & data) final;

	bool OnDragDropReceiveExternal(const Reflex::Object & data) override;


	bool OnKeyPress(System::KeyCode keycode, bool repeat) final;

	void OnKeyRelease(System::KeyCode keycode) final;

	bool OnCharacter(WChar character) final;



	//Reflex::Object callbacks

	void OnReleaseData() override;



	//advanced (typically called in SetLayout callback)

	Object(TRef <Detail::LayoutModel> layout);



private:

	friend Core::Accessor;

	friend class Library;

	friend Event;

	friend Style;

	
	void ApplyState(const Sequence <Key32> & states);

	UInt8 ComputeStyle();


	struct Delegates : public Reflex::Item<Delegate>::List { using List::Clear; };
	
	Delegates m_delegates;


	Reference <Detail::LayoutModel> m_layout;


	Trap m_mouseover_trap;

	Trap m_mouseclick_trap;

	Flags8 m_flowflags;

	UInt8 m_positioningflags;


	ConstReference <Style> m_style;

	Mods * m_mods;

	ConstReference <Detail::ComputedStyle> m_cstyle;	//actual cstyle after mods

	const Style * m_current_state;

	Float m_transition_z;

	
	#if (REFLEX_DEBUG)
	struct Token : public Reflex::Item <Token>
	{
		Token(GLX::Object & self);

		using Reflex::Item<Token>::Detach;

		GLX::Object & self;
	}
	token;
	static Token::List st_tokens;
	#endif
};

REFLEX_SET_TRAIT(Reflex::GLX::Object, IsSingleThreadExclusive);




//
//Object::Delegate

class Reflex::GLX::Object::Delegate : public Reflex::Item <Object::Delegate>
{
public:

	REFLEX_OBJECT(Delegate, Reflex::Object);

	static Delegate & null;

	template <class TYPE> using PropertyReferenceType = void;


	//location

	using Item::Detach;


	TRef <GLX::Object> GetObject() { return object;  }

	ConstTRef <GLX::Object> GetObject() const { return object; }



protected:

	virtual void OnAttachObject() {}

	virtual void OnDetachObject() {}

	virtual void OnAttachWindow() {}

	virtual void OnDetachWindow() {}

	virtual void OnSetStyle(const Style & style) {}

	virtual void OnUpdate() {}

	virtual bool OnEvent(GLX::Object & source, Event & e) { return false; }

	virtual bool OnMouseOver(Core::MouseAction mouseaction, UInt8 flags) { return false; }


	const TRef <GLX::Object> object;


private:

	friend class GLX::Object;

	friend List;

	friend Item;

	using Item::Attach;


	void OnAttach(List & list);

	void OnDetach(List & list);


	Key32 m_id;

};

REFLEX_SET_TRAIT(Reflex::GLX::Object::Delegate, IsSingleThreadExclusive);




//
//implementation

REFLEX_NS(Reflex::GLX::Detail)

void LogEventStep(GLX::Object & src, Event & e, GLX::Object & receiver);

using LegacyWeakReferenceObject = ObjectOf <Core::WeakReference>;

extern TRef <Allocator> gAllocator;

REFLEX_END

#if !REFLEX_DEBUG
inline void Reflex::GLX::Detail::LogEventStep(GLX::Object & src, Event & e, GLX::Object & receiver) {}
#endif

REFLEX_INLINE Reflex::GLX::Object::Object()
	: Object(kStandardLayout)
{
}

REFLEX_INLINE Reflex::GLX::Object::Object(Detail::LayoutModelCtr ctr)
	: Object(ctr(*this))
{
}

REFLEX_INLINE void Reflex::GLX::Core::Object::Focus()
{
	desktop->SetFocus(*Cast<GLX::Object>(this));
}

REFLEX_INLINE void Reflex::GLX::Object::SetMouseOverTrapMode(Trap trap)
{
	m_mouseover_trap = trap;
}

REFLEX_INLINE Reflex::GLX::Trap Reflex::GLX::Object::GetMouseOverTrapMode() const 
{
	return m_mouseover_trap;
}

REFLEX_INLINE void Reflex::GLX::Object::SetMouseClickTrapMode(Trap trap)
{
	m_mouseclick_trap = trap;
}

REFLEX_INLINE Reflex::GLX::Trap Reflex::GLX::Object::GetMouseClickTrapMode() const 
{
	return m_mouseclick_trap;
}

REFLEX_INLINE void Reflex::GLX::Object::SetLayoutFlags(UInt8 flags)
{
	m_flowflags = flags;

	RebuildLayout();
}

REFLEX_INLINE void Reflex::GLX::Object::SetPositioningFlags(UInt8 flags)
{
	m_positioningflags = flags;

	Realign();
}

REFLEX_INLINE bool Reflex::GLX::Object::ProcessEvent(Object & src, Event & e)
{
	Detail::LogEventStep(src, e, *this);

	return OnEvent(src, e);
}

inline bool Reflex::GLX::Object::Emit(Event & e)
{
	return True(EmitEx(e));
}

REFLEX_INLINE Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::Object::GetStyle() const
{
	return m_style;
}

REFLEX_INLINE Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::Object::GetCurrentStyle() const
{
	return m_current_state;
}

REFLEX_INLINE Reflex::ConstTRef <Reflex::GLX::Detail::ComputedStyle> Reflex::GLX::Object::GetComputedStyle() const
{
	return m_cstyle;
}

REFLEX_INLINE void Reflex::GLX::Object::ComputeLayout() const
{
	RemoveConst(this)->Refresh();
}

REFLEX_INLINE void Reflex::GLX::Object::Delegate::OnAttach(List & list)
{
	constexpr auto kOffset = REFLEX_OFFSETOF(GLX::Object, m_delegates);

	auto pobject = Reinterpret<GLX::Object>(Reinterpret<UInt8>(GetList()) - kOffset);

	RemoveConst(Delegate::object) = pobject;

	pobject->RebuildLayout();

	OnAttachObject();
}

REFLEX_INLINE void Reflex::GLX::Object::Delegate::OnDetach(List & list)
{
	GetObject()->RebuildLayout();

	OnDetachObject();

	RemoveConst(Delegate::object) = {};
}

REFLEX_NS(Reflex::GLX::Detail)

inline void AccommodateRenderer(Object & object, bool & isresponsive, Size & contentsize)
{
	auto renderer = object.GetRenderer();

	isresponsive = Or(isresponsive, renderer->responsive);

	renderer->OnAccommodate(contentsize);
}

inline void AlignRenderer(Object & object, Float & contenth)
{
	object.GetRenderer()->OnAlign(object.GetRect().size, contenth);
}

REFLEX_END
