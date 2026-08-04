#include "reflex/glx/window.h"
#include "reflex/glx/functions/properties.h"

#include "style/detail/cstyle.h"

#include "library.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

#if REFLEX_DEBUG
struct BaseEvent : public Event
{
	BaseEvent(Key32 id)
		: Event(id)
	{
		REFLEX_ASSERT(Reflex::Detail::ContextScope::GetCurrent() == Core::desktop->GetContextID() || IsNull(Core::g_renderer));
	}
};
#else
typedef Event BaseEvent;
#endif

struct EventWithModifiers : public BaseEvent
{
	EventWithModifiers(Key32 id)
		: BaseEvent(id)
	{
		Data::SetUInt8(*this, kmodifiers, System::GetModifierKeys());
	}
};

struct PointerEvent : public EventWithModifiers
{
	PointerEvent(GLX::Object & src, Key32 id, const Core::Pointer & pointer, Float64 timestamp)
		: EventWithModifiers(id)
		, src(src)
	{
		Data::SetUInt8(*this, kpointer_slot, pointer.slot);

		SetPoint(*this, kposition, pointer.position);

		Data::SetFloat64(*this, ktimestamp, timestamp);
	}

	Core::WeakReference src;
};

struct DragDropEvent : public BaseEvent
{
	DragDropEvent(Key32 id, Reflex::Object & data)
		: BaseEvent(id)
	{
		SetAbstractProperty(*this, kdrag_data, data);
	}
};

template <auto METHOD, typename... VARGS> REFLEX_INLINE void ForwardToDelegates(Object::Delegate::List & list, VARGS &&... args)
{
	if (list)
	{
		SafeIterate(list, [](Object::Delegate & d, Reflex::Detail::ArgPassType <VARGS> ... args)
		{
			(d.*METHOD)(args...);
		},
		std::forward<VARGS>(args)...);
	}
}

inline Key32 MakeKeyEventId(System::KeyCode keycode)
{
	return Reinterpret<Key32>(MakeTuple(UInt16(keycode), kMaxUInt16));
}

TRef <GLX::Object> BeginLinkedEvent(const Core::WeakReference & self_ref, Key32 id, Event & e)
{
	if (auto receiver = self_ref->EmitEx(e))	//IsValid(recipient))
	{
		self_ref->SetProperty(id, New<Detail::LegacyWeakReferenceObject>(receiver));

		return receiver;
	}
	else
	{
		return {};
	}
}

REFLEX_INLINE bool PerformLinkedEvent(Object & self, Key32 id, Event & e)
{
	if (auto preceiver = self.QueryProperty<Detail::LegacyWeakReferenceObject>(id))
	{
		return preceiver->value->ProcessEvent(self, e);
	}

	return false;
}

bool EndLinkedEvent(Object & self, Key32 id, Event & e)
{
	if (auto preceiver = self.QueryProperty<Detail::LegacyWeakReferenceObject>(id))
	{
		Core::WeakReference self_ref(self);

		preceiver->value->ProcessEvent(self, e);

		self_ref->UnsetProperty<Detail::LegacyWeakReferenceObject>(id);

		return true;
	}

	return false;
}

const Style * GetState(const Style & style, const Sequence <Key32> & states)
{
	auto state = &style;

	for (auto id : Cast<StyleAccessor>(style)->m_states)
	{
		if (id == kNullKey) break;

		if (states.Search(id))
		{
			if (auto sub_style = style.QuerySubStyle(id))	//nullptr can occur if state was detached, havent found a way to clean up m_states
			{
				state = GetState(*sub_style, states);
			}
		}
	}

	return state;
}

REFLEX_END_INTERNAL

REFLEX_SET_TRAIT(Reflex::GLX::Object::Delegates, IsSingleThreadExclusive);




//
//

//0: axis
//1: invert
//3: wrap
//4: autofit_x
//5: autofit_y

//0: alignmode_a (inline, float, abs)
//1: alignmode_b (inline, float, abs)
//2: axis_orientation (near, center, far, stretch)
//3: axis_orientation (near, center, far, stretch)
//4: ortho_orientation (near, center, far, stretch)
//5: ortho_orientation (near, center, far, stretch)




//
//implementation

struct Reflex::GLX::Event::Scope
{
	REFLEX_INLINE Scope(Event & e)
		: e(e)
	{
		e.Attach(st_list);
	}

	REFLEX_INLINE ~Scope()
	{
		e.Detach();
	}

	Event & e;
};

#if REFLEX_DEBUG
inline Reflex::GLX::Object::Token::Token(GLX::Object & self)
	: self(self)
{
	Attach(st_tokens);
}
decltype(Reflex::GLX::Object::st_tokens) Reflex::GLX::Object::st_tokens;
#endif

Reflex::GLX::Object::Object(TRef <Detail::LayoutModel> layout)
	: m_layout(layout)
	, m_pointer_tender_mask(kPointerFlagRightMouseButton | kPointerFlagDouble | kPointerFlagEmulated)
	, m_pointer_tender_trap(Core::kTrapPassive)
	, m_flowflags(Bits<false,false,false,false,true,true>::value)	//fit x | fit y
	, m_positioningflags(48)	//inline, axis=near, ortho=fit
	, m_mods(&g_library->null_mods)
	, m_current_state(&Style::null)
	, m_transition_z(0.0f)
#if REFLEX_DEBUG
	, token(*this)
#endif
{
}

Reflex::GLX::Object::~Object()
{
#if REFLEX_DEBUG
	token.Detach();
#endif

	auto library = g_library;

	m_delegates.Clear();

	m_mods = &library->null_mods;

	PropertySet::Clear();
}

void Reflex::GLX::Object::SetLayoutModel(TRef <Detail::LayoutModel> layout)
{
	m_layout = layout;

	RebuildLayout();
}

void Reflex::GLX::Object::SetStyle(const Style & style)
{
	REFLEX_ASSERT(Core::Context::IsActive());

	if (m_style != style)
	{
		m_style = style;

		auto & states = QueryProperty<States>(kStates, &g_library->null_states)->value;

		auto state = GetState(m_style, states);

		if (SetFiltered(m_current_state, state))
		{
			UnsetMod(kMaxUInt32);

			OnSetStyle(*state);

			RebuildLayout();
		}
	}
}

void Reflex::GLX::Object::SetDelegate(Key32 id, TRef <Delegate> delegate)
{
	auto t = AutoRelease(delegate);

	ClearDelegate(id);

	delegate->m_id = id;

	//preserve order for legacy compatibility, TODO remove id

	for (auto & i : m_delegates)
	{
		if (id.value < i.m_id.value)
		{
			delegate->InsertBefore(i);

			return;
		}
	}

	delegate->Attach(m_delegates);
}

void Reflex::GLX::Object::ClearDelegate(Key32 id)
{
	for (auto & i : m_delegates)
	{
		if (i.m_id == id)
		{
			i.Detach();

			return;
		}
	}
}

Reflex::GLX::Object::Delegate * Reflex::GLX::Object::QueryDelegate(Reflex::Detail::DynamicTypeRef object_t)
{
	for (auto & i : m_delegates)
	{
		if (Reflex::Detail::CheckObjectType(i, object_t)) return &i;
	}

	return nullptr;
}

void Reflex::GLX::Object::EnumerateDelegates(const Function <void(Delegate&)> & visitor)
{
	Reflex::SafeIterate(m_delegates, [](Delegate & d, const Function <void(Delegate&)> & visitor)
	{
		visitor(d);
	},
	visitor);
}

Reflex::Array < Reflex::Reference <Reflex::GLX::Object::Delegate> > Reflex::GLX::Object::GetDelegates()
{
	Array < Reference <Delegate> > rtn;

	rtn.Allocate(m_delegates.GetNumItem());

	for (auto & i : m_delegates)
	{
		rtn.Push<kAllocateNone>(i);
	}

	return rtn;
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Object::EmitEx(Event & e)
{
	auto pnull = &Object::null;

	REFLEX_ASSERT(GetActualRetainCount());	//TODO make this callers responsilbility, owners should never trigger callbacks if not retained

	if (GetActualRetainCount())
	{
		Event::Scope scope(e);

		Core::WeakReference source(*this);

		Core::WeakReference dest(*this);

		do
		{
			if (dest->ProcessEvent(source, e)) return dest.Adr();	//NullObject returns true so will break eventually

			dest = dest->GetParent();
		} 
		while (source.Adr() != pnull);
	}
	else
	{
		output.Warn("Object::Emit suppressed as not retained");
	}

	return pnull;
}

void Reflex::GLX::Object::SetMod(Key32 id, TRef <Detail::ComputedStyle> cstyle)
{
	REFLEX_ASSERT(cstyle->GetAllocator());

	m_mods = Data::Detail::AcquireProperty<Mods>(*this, kMods).Adr();

	m_mods->value.Set(id, cstyle);

	RebuildLayout();
}

void Reflex::GLX::Object::UnsetMod(Key32 id)
{
	auto & mods = m_mods->value;

	if (mods.Unset(id))
	{
		if (mods.Empty())
		{
			m_mods = &g_library->null_mods;

			UnsetProperty<Mods>(kMods);
		}

		RebuildLayout();
	}
}

Reflex::ConstTRef <Reflex::GLX::Detail::ComputedStyle> Reflex::GLX::Object::GetMod(Key32 id) const
{
	return *m_mods->value.Search(id, &g_library->null_cstyle_ref);
}

void Reflex::GLX::Object::SetState(Key32 state)
{
	REFLEX_ASSERT(Core::Context::IsActive());

	auto & states = Data::Detail::AcquireProperty<States>(*this, kStates)->value;

	states.Set(state);

	ApplyState(states);
}

void Reflex::GLX::Object::ClearState(Key32 state)
{
	REFLEX_ASSERT(Core::Context::IsActive());

	auto & states = QueryProperty<States>(kStates, &g_library->null_states)->value;

	if (auto idx = states.Search(state))
	{
		states.Remove(idx.value);

		ApplyState(states);
	}
}

bool Reflex::GLX::Object::CheckState(Key32 state) const
{
	auto & states = QueryProperty<States>(kStates, &g_library->null_states)->value;

	return True(states.Search(state));
}

void Reflex::GLX::Object::OnAttachWindow()
{
	ForwardToDelegates<&Delegate::OnAttachWindow>(m_delegates);
}

void Reflex::GLX::Object::OnDetachWindow()
{
	ForwardToDelegates<&Delegate::OnDetachWindow>(m_delegates);
}

void Reflex::GLX::Object::OnUpdate()
{
	ForwardToDelegates<&Delegate::OnUpdate>(m_delegates);
}

bool Reflex::GLX::Object::OnEvent(Object & src, Event & e)
{
	if (m_delegates)
	{
		Core::WeakReference source(src);

		return SafeIterate(m_delegates, [](Delegate & dlg, Core::WeakReference & source, Event & e)
		{
			return dlg.OnEvent(source, e);
		},
		source, e);
	}

	return false;
}

void Reflex::GLX::Object::OnSetStyle(const Style & style)
{
	if (m_delegates)
	{
		ForwardToDelegates<&Delegate::OnSetStyle>(m_delegates, style);
	}
}

Reflex::GLX::Core::Trap Reflex::GLX::Object::OnPointerTender(Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags)
{
	if (flags != (flags & m_pointer_tender_mask))
	{
		return Core::kTrapThru;
	}

	auto trap = m_pointer_tender_trap;

	if (m_delegates)
	{
		SafeIterate(m_delegates, [&trap](Delegate & dlg, Core::PointerAction action, const Core::Pointer & pointer, UInt8 flags)
		{
			return dlg.OnPointerTender(action, pointer, flags, trap);
		},
		action, pointer, flags);
	}

	return trap;
}

void Reflex::GLX::Object::OnBuildLayout(AccommodateFn & accommodate, AlignFn & align)
{
	UInt8 layout_flags = ComputeStyle();
	
	auto cstyle = GetComputedStyle();
	
	SetRenderer(cstyle->CreateRenderer(*this), cstyle->GetZIndex());

	auto layout = m_layout->OnRebuild(*this, layout_flags);

	accommodate = layout.a;

	align = layout.b;
}

void Reflex::GLX::Object::OnFocus()
{
	SetState(kFocusedState);

	BaseEvent e(kFocus);

	BeginLinkedEvent(*this, kFocus, e);
}

void Reflex::GLX::Object::OnLoseFocus()
{
	ClearState(kFocusedState);

	BaseEvent e(kLoseFocus);

	EndLinkedEvent(*this, kFocus, e);
}

void Reflex::GLX::Object::OnMouseEnter(Object & previous)
{
	Core::WeakReference self(*this);

	BaseEvent e(kMouseEnter);

	BeginLinkedEvent(self, kMouseEnter, e);

	self->SetState(kHoverState);

	auto library = g_library;

	auto & mouseover = library->m_mouseover;

	auto & mouseover_z = library->m_mouseover_z;

	mouseover_z.Swap(mouseover);

	for (auto & i : ParentRange(*self))
	{
		mouseover.Push({ &i, i });

		i.SetState(kHoverState);
	}

	for (auto & i : mouseover_z)
	{
		if (!Search<KeyCompare>(mouseover, i.a))
		{
			i.b->ClearState(kHoverState);
		}
	}

	mouseover_z.Clear();
}

void Reflex::GLX::Object::OnMouseLeave()
{
	BaseEvent e(kMouseLeave);

	EndLinkedEvent(*this, kMouseEnter, e);
}

Reflex::GLX::Core::Trap Reflex::GLX::Object::OnPointerDown(const Core::Pointer & pointer, UInt8 flags, Float64 timestamp)
{
	PointerEvent e(*this, kPointerDown, pointer, timestamp);

	Data::SetUInt8(e, kflags, flags);

	BeginLinkedEvent(e.src, Detail::MakePointerDownLinkID(pointer.slot), e);

	return Core::Trap(Data::GetUInt8(e, kcapture, Core::kTrapActive));// .src->m_mouseclick_trap;	//!important must read after the event
}

void Reflex::GLX::Object::OnPointerDrag(const Core::Pointer & pointer, Float64 timestamp, Point drag)
{
	PointerEvent e(*this, kPointerDrag, pointer, timestamp);

	SetPoint(e, kdelta, drag);

	PerformLinkedEvent(e.src, Detail::MakePointerDownLinkID(pointer.slot), e);
}

void Reflex::GLX::Object::OnPointerUp(const Core::Pointer & pointer, Float64 timestamp)
{
	PointerEvent e(*this, kPointerUp, pointer, timestamp);

	EndLinkedEvent(e.src, Detail::MakePointerDownLinkID(pointer.slot), e);
}

void Reflex::GLX::Object::OnMouseWheel(const Core::Pointer & pointer, Float64 timestamp, Point delta, bool inverted)
{
	PointerEvent e(*this, kMouseWheel, pointer, timestamp);

	SetPoint(e, kdelta, delta);

	Data::SetBool(e, kinverted, inverted);
	
	Emit(e);
}

bool Reflex::GLX::Object::OnDragDropTender(const Core::Pointer & pointer, Reflex::Object & data)
{
	DragDropEvent e(kDragDropTender, data);

	Data::SetUInt8(e, kpointer_slot, pointer.slot);
	SetPoint(e, kposition, pointer.position);

	return ProcessEvent(*this, e) && Data::GetBool(e, MakeKey32("accept"), true);
}

void Reflex::GLX::Object::OnDragDropEnter(Reflex::Object & data)
{
	DragDropEvent e(kDragDropEnter, data);

	ProcessEvent(*this, e);
}

void Reflex::GLX::Object::OnDragDropLeave(Reflex::Object & data)
{
	DragDropEvent e(kDragDropLeave, data);

	ProcessEvent(*this, e);
}

void Reflex::GLX::Object::OnDragDropReceive(Reflex::Object & data)
{
	DragDropEvent e(kDragDropReceive, data);

	ProcessEvent(*this, e);
}

bool Reflex::GLX::Object::OnDragDropReceiveExternal(const Reflex::Object & data)
{
	DragDropEvent e(kDragDropReceiveExternal, RemoveConst(data));

	return ProcessEvent(*this, e);
}

bool Reflex::GLX::Object::OnKeyPress(System::KeyCode keycode, bool repeat)
{
	REFLEX_STATIC_ASSERT(System::kNumKeyCode < kMaxUInt8);

	EventWithModifiers e(kKeyDown);

	Data::SetUInt8(e, kkeycode, UInt8(keycode));

	Data::SetBool(e, MakeKey32("repeat"), repeat);

	return True(BeginLinkedEvent(*this, MakeKeyEventId(keycode), e));
}

void Reflex::GLX::Object::OnKeyRelease(System::KeyCode keycode)
{
	EventWithModifiers e(kKeyUp);

	Data::SetUInt8(e, kkeycode, UInt8(keycode));

	EndLinkedEvent(*this, MakeKeyEventId(keycode), e);
}

bool Reflex::GLX::Object::OnCharacter(WChar character)
{
	EventWithModifiers e(kCharacter);

	e.SetProperty(MakeKey32("character"), New<ObjectOf<WChar>>(character));

	return Emit(e);
}

REFLEX_INLINE void Reflex::GLX::Object::ApplyState(const States::ValueType & states)
{
	auto state = GetState(m_style, states);

	auto & from = *m_current_state;

	if (SetFiltered(m_current_state, state))
	{
		auto computed = Detail::Compile<Detail::ComputedStyle>(*state);

		auto time_z = m_transition_z;

		m_transition_z = computed->GetTransitionTime() * Float(AnimationScope::IsEnabled());

		if (auto transition_time = Reflex::Max(time_z, m_transition_z))
		{
			auto from_computed = Detail::Compile<Detail::ComputedStyle>(from);

			auto transition = New<StateTransition>(*this, from, *state, transition_time);

			auto cstyle_transition = New<Detail::ComputedStyleTransition>(from_computed, computed, transition);

			SetMod(kMaxUInt32, cstyle_transition);

			transition->Play();
		}
		else
		{
			UnsetMod(kMaxUInt32);
		}

		OnSetStyle(*m_current_state);

		RebuildLayout();
	}
}

Reflex::UInt8 Reflex::GLX::Object::ComputeStyle()
{
	auto & state = *m_current_state;

	m_cstyle = Detail::Compile<Detail::ComputedStyle>(state);

	REFLEX_ASSERT(m_cstyle->GetAllocator());

	if (auto & mods = m_mods->value)
	{
		//here ComputedStyleTranstion carries forward its ref to TransitionState

		for (auto & i : mods)
		{
			m_cstyle = i.value->Mutate(m_cstyle);
		}
	}

	return m_flowflags.GetWord() | m_cstyle->GetLayoutFlags();
}

void Reflex::GLX::Object::OnReleaseData()
{
	auto library = g_library;

	m_mods = &library->null_mods;

	Core::Object::OnReleaseData();
}
