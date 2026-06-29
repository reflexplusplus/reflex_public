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

	void EnablePointer(bool enabled, bool active = false);

	Pair <bool> PointerEnabled() const;


	void EnableMultiTouch(bool enable = true);

	bool MultiTouchEnabled() const;



	//delegates (experimental API)

	void SetDelegate(Key32 id, TRef <Delegate> dlg);

	void ClearDelegate(Key32 id);


	Delegate * QueryDelegate(Reflex::Detail::DynamicTypeRef object_t);
	
	template <class TYPE> TYPE * QueryDelegate();

	void EnumerateDelegates(const Function <void(Delegate&)> & visitor);

	Array < Reference <Delegate> > GetDelegates();



	//events

	bool ProcessEvent(Object & src, Event & e);

	bool Emit(Event & e);

	TRef <Object> EmitEx(Event & e);


	void Focus();



	//style

	void SetStyle(const Style & style);

	ConstTRef <Style> GetStyle() const;



	//state

	void UnsetState(Key32 state);

	void SetState(Key32 state);

	bool CheckState(Key32 state) const;

	ConstTRef <Style> GetCurrentStyle() const;			//the current applied style, after states are applied



	//mods

	void UnsetMod(Key32 id);

	void SetMod(Key32 id, TRef <Detail::ComputedStyle> cstyle);

	ConstTRef <Detail::ComputedStyle> GetMod(Key32 id) const;

	ConstTRef <Detail::ComputedStyle> GetComputedStyle() const;



	//used by standard layout/style, typically do not use directly

	void SetLayoutFlags(UInt8 flags);

	const UInt8 & GetLayoutFlags() const { return m_flowflags.GetWord(); }


	void SetPositioningFlags(UInt8 flags);

	UInt8 GetPositioningFlags() const { return m_positioningflags; }


	void ComputeLayout() const;



	//properties

	Key32 id;



	//deprecations

	[[deprecated("use UnsetState")]] void ClearState(Key32 state) { return UnsetState(state); }



protected:

	using Mods = ObjectOf < Map <Key32, ConstReference <Detail::ComputedStyle> > >;


	virtual bool OnEvent(Object & src, Event & e);	//forwards to delegates

	virtual void OnSetStyle(const Style & style);	//forwards to delegates


	void OnAttachWindow() override;					//forwards to delegates

	void OnDetachWindow() override;					//forwards to delegates

	void OnUpdate() override;						//forwards to delegates


	void OnBuildLayout(AccommodateFn & accommodate, AlignFn & align) override;


	void OnFocus() final;

	void OnLoseFocus() final;


	Core::Trap OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags) override;

	Core::Trap OnPointerDown(const Core::Pointer & pointer, UInt8 flags, Float64 timestamp) final;

	void OnPointerDrag(const Core::Pointer & pointer, Float64 timestamp, Point drag) final;

	void OnPointerUp(const Core::Pointer & pointer, Float64 timestamp) final;


	void OnMouseEnter(Object & previous) final;

	void OnMouseLeave() final;

	void OnMouseWheel(const Core::Pointer & pointer, Float64 timestamp, Point delta, bool inverted) final;


	bool OnDragDropTender(const Core::Pointer & pointer, Reflex::Object & data) final;

	void OnDragDropEnter(Reflex::Object & data) final;

	void OnDragDropLeave(Reflex::Object & data) final;

	void OnDragDropReceive(Reflex::Object & data) final;

	bool OnDragDropReceiveExternal(const Reflex::Object & data) override;


	bool OnKeyPress(System::KeyCode keycode, bool repeat) final;

	void OnKeyRelease(System::KeyCode keycode) final;

	bool OnCharacter(WChar character) final;



	//Reflex::Object callbacks

	void OnReleaseData() override;



	//advanced

	Object(TRef <Detail::LayoutModel> layout);



private:

	friend class Library;

	friend class Event;

	friend class Style;

	friend struct StateTransition;

	
	void ApplyState(const Sequence <Key32> & states);

	void RebuildRenderer();


	struct Delegates : public Reflex::Item<Delegate>::List { using List::Clear; };
	
	Delegates m_delegates;


	Reference <Detail::LayoutModel> m_layout;


	UInt8 m_pointer_tender_mask;

	Core::Trap m_pointer_tender_trap;

	
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

	
	virtual bool OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags, Core::Trap & trap) { return false; }	//!experimental


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
//impl

REFLEX_NS(Reflex::GLX::Detail)

inline Key32 MakePointerDownLinkID(UInt8 pointer_slot)
{
	return Reinterpret<Key32>(MakeTuple(pointer_slot, UInt8(1), kMaxInt16));
}

void LogEventStep(GLX::Object & src, Event & e, GLX::Object & receiver);

using LegacyWeakReferenceObject = ObjectOf <Core::WeakReference>;

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

template <class TYPE> inline TYPE * Reflex::GLX::Object::QueryDelegate() 
{ 
	REFLEX_STATIC_ASSERT_DYNAMIC_CASTABLE(TYPE); 
	
	return Cast<TYPE>(QueryDelegate(TYPE::kDynamicTypeInfo)); 
}

REFLEX_INLINE void Reflex::GLX::Object::Focus()
{
	Core::desktop->SetFocus(*this);
}

REFLEX_INLINE void Reflex::GLX::Object::EnablePointer(bool enable, bool active)
{
	constexpr Core::Trap kMap[2][2] = { { Core::kTrapThru, Core::kTrapReject }, { Core::kTrapPassive, Core::kTrapActive } };

	m_pointer_tender_trap = kMap[enable][active];
}

inline Reflex::Pair <bool> Reflex::GLX::Object::PointerEnabled() const
{
	switch (m_pointer_tender_trap)
	{
	case Core::kTrapThru:	//0
		return { false, false };

	case Core::kTrapPassive://1
		return { true, false };

	case Core::kTrapActive:	//2
		return { true, true };

	case Core::kTrapReject:	//3
		return { false, true };

	default:
		return {};
	}
}

REFLEX_INLINE void Reflex::GLX::Object::EnableMultiTouch(bool enable)
{
	m_pointer_tender_mask = BitSet(m_pointer_tender_mask, GetFirstBit(kPointerFlagMulti).value, enable);
}

REFLEX_INLINE bool Reflex::GLX::Object::MultiTouchEnabled() const
{
	return True(m_pointer_tender_mask & kPointerFlagMulti);
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
