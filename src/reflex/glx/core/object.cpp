#include "reflex/glx/window.h"
#include "reflex/glx/functions/geometry.h"
#include "reflex/glx/detail/functions/render.h"

#include "impl.h"




//
//

#define UPCAST(obj) (static_cast<GLX::Object&>(obj))

REFLEX_NS(Reflex::GLX::Core)

void RealignBranch(Object & object)
{
	if (object.GetFirst())
	{
		for (auto & i : UPCAST(object)) RealignBranch(i);
	}
	else
	{
		object.Realign();
	}
}

REFLEX_END

struct Reflex::GLX::Core::Object::Internal
{
	struct AttachScope;

	struct ScopedWeakref;


	typedef ObjectOf < Sequence <Object*> > ZList;


	static void AttachToWindow(GLX::WindowClient & window, Object & object);

	static void UpdateUpwards(Object * object, UInt8 thisflags, UInt8 parentflags);


	template <bool ON_ATTACHDETACH, bool ON_CLOCK, bool HAS_ZINDEX, bool HAS_WINDOW, bool WILLHAVE_WINDOW> static void AttachRecursive(GLX::WindowClient & window, Object & object);

	REFLEX_TBINDER_5P(AttachRecursive);


	template <bool ON_ATTACHDETACH, bool ON_CLOCK, bool HAS_ZINDEX, bool HAS_WINDOW, bool HAS_MOUSEOVER, bool HAS_FOCUS, bool DESTROY> static void DetachRecursive(bool root, Object * parent, Object * object);

	REFLEX_TBINDER_7P(DetachRecursive);


	template <bool CHILDREN, bool UPDATE> static void ProcessUpdate(Object & self);

	REFLEX_TBINDER_2P(ProcessUpdate);


	template <bool CHILDREN, bool UPDATE, bool REBIND, bool ACCOMMODATE, bool REALIGN, bool REDRAW> static void ProcessRefresh(Object & self);

	REFLEX_TBINDER_6P(ProcessRefresh);


	template <Trap TRAP> static bool FindPointerTargetImpl(PointerAction action, const Pointer & pointer, UInt8 flags, Point position, const System::Renderer::Transform & transform, Object & object, Object * & result);


	REFLEX_INLINE static void EnableZIndex(GLX::Object & parent, Object * child)
	{
		auto zlist = Data::Detail::AcquireProperty<ZList>(parent, kNullKey);

		zlist->value.Set(child);

		parent.m_draw_with_zindex = true;
	}

	REFLEX_INLINE static void DisableZIndex(GLX::Object & parent, Object * child)
	{
		if (auto zlist = parent.QueryProperty<ZList>(kNullKey))
		{
			RemoveKey(zlist->value, child);

			parent.m_draw_with_zindex = True(zlist->value);
		}
		else
		{
			REFLEX_ASSERT(!parent.m_draw_with_zindex);
			
			parent.m_draw_with_zindex = false;
		}
	}


	static const UInt8 kAttachRecursiveMask = (1 << kRequestOnAttachDetach) | (1 << kRequestOnClock) | (1 << kHasZIndex) | (1 << kHasWindow);

	static constexpr decltype(&FindPointerTargetImpl<kTrapThru>) kFindPointerTargetFns[kNumTrap] =
	{
		&Internal::FindPointerTargetImpl<kTrapThru>,
		&Internal::FindPointerTargetImpl<kTrapPassive>,
		&Internal::FindPointerTargetImpl<kTrapActive>,
		&Internal::FindPointerTargetImpl<kTrapReject>,
	};
};

struct Reflex::GLX::Core::Object::Internal::AttachScope
{
	REFLEX_INLINE AttachScope(Object & child, GLX::Object & parent)
		: child(child),
		parent(parent),
		m_upward_flags(child.m_upward_flags)
	{
		REFLEX_ASSERT(IsValid(parent));

		child.m_upward_flags = kMaxUInt8;

		child.Accommodate();

		child.m_parent = &parent;
	}

	REFLEX_INLINE ~AttachScope()
	{
		child.Accommodate();

		Internal::AttachToWindow(*parent.m_window, child);

		child.m_upward_flags = m_upward_flags;
	}

	Object & child;

	GLX::Object & parent;

	Flags8 m_upward_flags;
};

struct Reflex::GLX::Core::Object::Internal::ScopedWeakref
{
	template <class TYPE> REFLEX_INLINE ScopedWeakref(TYPE * & ptr)
		: m_ptr(reinterpret_cast<GLX::Object*&>(ptr)),
		m_weakref(*m_ptr)
	{
	}

	REFLEX_INLINE ~ScopedWeakref()
	{
		m_ptr = m_weakref.Adr();
	}

	bool Alive()
	{
		GLX::Object * test = m_ptr;

		m_ptr = m_weakref.Adr();

		return m_ptr == test;
	}

	GLX::Object * & m_ptr;

	WeakReference m_weakref;
};

Reflex::GLX::Core::Object::Object()
	: m_upward_flags(kMaxUInt8),
	m_mouse_cursor(System::kMouseCursorArrow),
	m_zindex(0),
	m_draw_with_zindex(false),
	m_redraw(false),
	m_scale(MakeSize(1.0f))
{
	m_refresh_flags.Set(kRefreshSetLayout);
	//m_refresh_flags.Set(kRefreshAccommodate);
}

Reflex::GLX::Core::Object::~Object()
{
	REFLEX_ASSERT(!Cast<DesktopImpl>(desktop)->m_clocklist.Search(this));

	REFLEX_ASSERT(m_attach_flags.Check(kHasWindow) == (m_window.Adr() != &GLX::WindowClient::null));

	m_parent->Accommodate();

	UInt8 flags = m_attach_flags.GetWord() /*| (1 << kDetachRoot)*/ | (1 << kDetachDestroy);

	Internal::DetachRecursiveBinder::Bind(flags)(true, m_parent.Adr(), this);
}

void Reflex::GLX::Core::Object::EnableOnClock(bool enable)
{
	if (m_attach_flags.Check(kHasWindow))
	{
		auto & clocklist = Cast<DesktopImpl>(desktop)->m_clocklist;

		if (enable)
		{
			clocklist.Set(this);
		}
		else 
		{
			clocklist.Unset(this);
		}
	}

	m_attach_flags.Set(kRequestOnClock, enable);
}

void Reflex::GLX::Core::Object::EnableOnAttachDetachWindow(bool enable)
{
	m_attach_flags.Set(kRequestOnAttachDetach, enable);
}

Reflex::Int8 Reflex::GLX::Core::Object::GetZIndex() const
{
	return m_zindex;
}

void Reflex::GLX::Core::Object::SetParent(GLX::Object & parent)
{
	Internal::AttachScope scope(*this, parent);

	Attach(parent);
}

void Reflex::GLX::Core::Object::InsertBefore(GLX::Object & object)
{
	Internal::AttachScope scope(*this, *object.m_parent);

	Node::InsertBefore(object);
}

void Reflex::GLX::Core::Object::InsertAfter(GLX::Object & object)
{
	Internal::AttachScope scope(*this, *object.m_parent);

	Node::InsertAfter(object);
}

void Reflex::GLX::Core::Object::Detach()
{
#if (REFLEX_DEBUG)
	bool haswindow = (m_window != GLX::WindowClient::null);

	REFLEX_ASSERT(m_attach_flags.Check(kHasWindow) == haswindow);
#endif
	
	m_parent->Accommodate();

	UInt8 flags = m_attach_flags.GetWord() /*| (1 << kDetachRoot)*/;

	Internal::DisableZIndex(m_parent, this);

	//TODO m_parent needs to be cleared here, before possiblity to destruct, callbacks should occur in Node::OnDetach call

	Internal::DetachRecursiveBinder::Bind(flags)(true, m_parent.Adr(), this);

	m_parent = GLX::Object::null;	//TODO unsafe here

	Node::Detach();					//TODO unsafe here
}

void Reflex::GLX::Core::Object::Clear()
{
	for (auto itr = GetLast(); itr;) Reflex::Detail::Traverse<true>(itr)->Detach();
}

void Reflex::GLX::Core::Object::SendBottom()
{
	if (GetPrev())
	{
		Node::InsertBefore(*Node::GetParent()->GetFirst());

		Realign();
	}
}

void Reflex::GLX::Core::Object::SendTop()
{
	if (GetNext())
	{
		Node::Attach(*Node::GetParent());

		Realign();
	}
}

void Reflex::GLX::Core::Object::SetRect(const Rect & rect)
{
	if (SetFiltered(m_rect, rect)) Realign();
}

void Reflex::GLX::Core::Object::SetPosition(const Point & position)
{
	auto & rectpoint = RemoveConst(m_rect.origin);

	if (rectpoint != position)
	{
		rectpoint = position;

		auto redraw = UInt8(1 << kRefreshRedraw);

		Internal::UpdateUpwards(m_parent.Adr(), redraw, redraw);
	}
}

void Reflex::GLX::Core::Object::SetSize(const Size & size)
{
	auto & rectsize = RemoveConst(m_rect.size);

	if (rectsize != size)
	{
		rectsize = size;

		Realign();
	}
}

void Reflex::GLX::Core::Object::SetScale(const Scale & scale)
{
	if (SetFiltered(m_scale, scale)) Realign();
}

void Reflex::GLX::Core::Object::SetMouseCursor(System::MouseCursor mousecursor)
{
	auto desktop_impl = Cast<DesktopImpl>(desktop);

	m_mouse_cursor = mousecursor;

	Cast<DesktopImpl>(desktop)->Notify();
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Core::Object::FindPointerTarget(PointerAction action, const Pointer & pointer, UInt8 flags, Point offset)
{
	Object * result = this;

	(*Internal::kFindPointerTargetFns[kTrapThru])(action, pointer, flags, offset, {}, *this, result);

	return UPCAST(*result);
}

void Reflex::GLX::Core::Object::RebuildLayout()
{
	Internal::UpdateUpwards(this, (1 << kRefreshSetLayout), (1 << kRefreshAccommodate));
}

void Reflex::GLX::Core::Object::Accommodate()
{
	UInt8 flags = (1 << kRefreshAccommodate);	// | (1 << kRefreshAlign) | (1 << kRefreshRedraw);

	Internal::UpdateUpwards(this, flags, flags);
}

void Reflex::GLX::Core::Object::Realign()
{
	//TODO does Realign need to realign the parent branches?
	//an example is if centered, adjusting its content size will need realign on the parent
	//cant think of other cases..
	//so this should work, but need to ensure all drawing refresh are done on Redraw and not on Refresh

	//if (REFLEX_DEBUG)
	{
		Internal::UpdateUpwards(this, (1 << kRefreshAlign), (1 << kRefreshRedraw));

		m_parent->m_refresh_flags |= UInt8(1 << kRefreshAlign);
	}
	//else
	//{
	//	UInt8 flags = (1 << kRefreshAlign);	// | (1 << kRefreshRedraw);

	//	Internal::UpdateUpwards(this, flags, flags);
	//}
}

void Reflex::GLX::Core::Object::Redraw()
{
	UInt8 flags = (1 << kRefreshRedraw);	// | (1 << kRefreshRedraw);

	Internal::UpdateUpwards(this, flags, flags);
}

void Reflex::GLX::Core::Object::Update()
{
	Internal::UpdateUpwards(this, 1 << kRefreshUpdate, 0);
}

void Reflex::GLX::Core::Object::SetRenderer(Renderer & renderer, Int8 zindex)
{
	m_renderer = renderer;

	if (SetFiltered(m_zindex, zindex))
	{
		auto & parent = *m_parent;

		m_attach_flags.Set(kHasZIndex, True(zindex));

		if (zindex)
		{
			Internal::EnableZIndex(parent, this);
		}
		else
		{
			Internal::DisableZIndex(parent, this);
		}
	}
}

void Reflex::GLX::Core::Object::OnReleaseData()
{
	while (auto pchild = GetFirst())
	{
		Reference <Object> child = pchild;

		child->ReleaseData();

		child->Detach();
	}

	Data::PropertySet::OnReleaseData();
}

const Reflex::GLX::Core::Object::Internal::ProcessRefreshBinder::Type * Reflex::GLX::Core::Object::st_updatefns = Reflex::Detail::TBind<Internal::ProcessUpdateBinder>();

const Reflex::GLX::Core::Object::Internal::ProcessRefreshBinder::Type * Reflex::GLX::Core::Object::st_refreshfns = Reflex::Detail::TBind<Internal::ProcessRefreshBinder>();

REFLEX_INLINE void Reflex::GLX::Core::Object::Internal::AttachToWindow(GLX::WindowClient & window, Object & object)
{
	UInt8 flags = (object.m_attach_flags.GetWord() & kAttachRecursiveMask) | (&window != &GLX::WindowClient::null ? (1 << kWillHaveNewWindow) : 0);

	auto attachrecursive = AttachRecursiveBinder::Bind(flags);

	attachrecursive(window, object);
}

REFLEX_INLINE void Reflex::GLX::Core::Object::Internal::UpdateUpwards(Object * object, UInt8 flags, UInt8 parentflags)
{
	object->m_refresh_flags |= flags;

	auto parent = object->m_parent.Adr();

	parentflags = (1 << kRefreshChildren) | (parentflags & object->m_upward_flags.GetWord());

	while ((parent->m_refresh_flags.GetWord() & parentflags) != parentflags)	//
	{
		parent->m_refresh_flags |= parentflags;

		parentflags &= parent->m_upward_flags.GetWord();

		parent = parent->m_parent.Adr();
	}

	Cast<DesktopImpl>(desktop)->Notify();
}

template <bool ENABLE_ATTACHDETACH, bool ENABLE_CLOCK, bool HAS_ZINDEX, bool HAS_WINDOW, bool WILLHAVE_WINDOW> void Reflex::GLX::Core::Object::Internal::AttachRecursive(GLX::WindowClient & window, Object & object)
{
	//OBJECT COULD DIE DURING CALLBACKS, unsafe for Script level
	//solution: for stack objects, use weakref checker
	//for heap, use strongref
	//do retain/check at root level of this function, not in the child iterator
	
	auto desktopimpl = Cast<DesktopImpl>(desktop);

	object.m_window = &window;

	if constexpr (HAS_ZINDEX) EnableZIndex(*object.m_parent, &object);

	if constexpr (WILLHAVE_WINDOW > HAS_WINDOW) //attaching to window
	{
		REFLEX_ASSERT(&window != &GLX::WindowClient::null);

		object.m_attach_flags.Set(kHasWindow);

		if constexpr (ENABLE_ATTACHDETACH)
		{
			object.OnAttachWindow();
		}

		if constexpr (ENABLE_CLOCK)	//doestn currently have window and clock requested
		{
			desktopimpl->m_clocklist.Set(&object);	//.Set(Cast<GLX::Object>(&object));
		}
	}
	else if constexpr (HAS_WINDOW > WILLHAVE_WINDOW)	//detaching
	{
		if constexpr (ENABLE_ATTACHDETACH) object.OnDetachWindow();

		object.m_attach_flags.Clear(kHasWindow);

		if constexpr (ENABLE_CLOCK) desktopimpl->m_clocklist.Unset(&object);
	}

	//for (auto itr = object.GetFirst(); itr;)
	//{
	//	auto object = Reflex::Detail::Traverse<false>(itr);

	//	Reference <GLX::Object> retainer(object);

	//	//auto test = (1 << kWillHaveNewWindow);

	//	UInt8 flags = (object->m_attach_flags.GetWord() & kAttachRecursiveMask) | (WILLHAVE_WINDOW ? (1 << kWillHaveNewWindow) : 0);

	//	AttachRecursiveBinder::Bind(flags)(window, object);
	//}

	if (object.GetNumItem())
	{
		SafeIterate(object, [](Object & child, GLX::WindowClient & window)
		{
			UInt8 flags = (child.m_attach_flags.GetWord() & kAttachRecursiveMask) | (WILLHAVE_WINDOW ? (1 << kWillHaveNewWindow) : 0);

			AttachRecursiveBinder::Bind(flags)(window, child);
		}, window);
	}
}

template <bool ENABLE_ATTACHDETACH, bool ENABLE_CLOCK, bool HAS_ZINDEX, bool HAS_WINDOW, bool HAS_MOUSEOVER, bool HAS_FOCUS, bool DESTROY> void Reflex::GLX::Core::Object::Internal::DetachRecursive(bool root, Object * parent, Object * object)
{
	auto desktopimpl = Cast<DesktopImpl>(desktop);

	if constexpr (HAS_ZINDEX)
	{
		if (root) DisableZIndex(UPCAST(*parent), object);
	}

	if constexpr (HAS_WINDOW & ENABLE_CLOCK)
	{
		desktopimpl->m_clocklist.Unset(object);
	}

	if constexpr (HAS_WINDOW & !DESTROY)
	{
		if constexpr (ENABLE_ATTACHDETACH)
		{
			ScopedWeakref a(parent);

			ScopedWeakref b(object);

			object->OnDetachWindow();	//could kill self here
		}

		object->m_window = GLX::WindowClient::null;

		object->m_attach_flags.Clear(kHasWindow);
	}

	if constexpr (HAS_FOCUS & DESTROY)	//changed may 2021!!		//change to & DESTROY aug 2019
	{
		desktopimpl->m_mouseover.Clear();	//prevent callback to dying object, causes problems

		if constexpr (HAS_WINDOW)
		{
			//output("HAS_FOCUS & DESTROY");

			ScopedWeakref a(parent);

			ScopedWeakref b(object);

			desktopimpl->m_mouseover.Clear();	//prevent callback to dying object, causes problems

			desktopimpl->SetFocus(object->GetWindow()->GetContent());
		}
	}

	if constexpr (HAS_MOUSEOVER & DESTROY)
	{
		//output("HAS_MOUSEOVER & DESTROY");

		ScopedWeakref a(parent);

		ScopedWeakref b(object);

		desktopimpl->m_mouseover.Clear();	//prevent callback to dying object, causes problems

		desktopimpl->EndMouseCapture(object->m_window);		//could kill self here

		desktopimpl->SetMouseOver(GLX::Object::null);		//could kill self here
	}

	//if constexpr (ROOT)
	//{
	//	object->m_parent = GLX::Object::null;
	//}

	if constexpr (HAS_WINDOW | DESTROY)
	{
		//for (auto itr = object->GetLast(); itr;)
		//{
		//	auto child = Reflex::Detail::Traverse<true>(itr);

		//	if constexpr (DESTROY) child->m_parent = GLX::Object::null;

		//	if constexpr (HAS_WINDOW)
		//	{
		//		DetachRecursiveBinder::Bind(child->m_attach_flags.GetWord())(parent, child.Adr());	//TODO pass ref to pparent, because may die recursively
		//	}
		//}

		if constexpr (DESTROY)
		{
			for (auto itr = object->GetLast(); itr;)
			{
				auto child = Reflex::Detail::Traverse<true>(itr);

				child->m_parent = GLX::Object::null;
			}
		}

		if constexpr (HAS_WINDOW)
		{
			if (object->GetNumItem())
			{
				SafeReverseIterate(*object, [](Object & child, Object * root)
				{
					DetachRecursiveBinder::Bind(child.m_attach_flags.GetWord())(false, root, &child);	//TODO pass ref to pparent, because may die recursively
				},
				parent);
			}
		}
	}

	//if constexpr (ROOT & !DESTROY) object->Node::Detach();
}

#define SET(flags, x) flags.Set(x)
#define CLEAR(flags, x) flags.Clear(x)

template <bool CHILDREN, bool UPDATE> void Reflex::GLX::Core::Object::Internal::ProcessUpdate(Object & object)
{
	Flags8 & flags = object.m_refresh_flags;

	Flags8 & guard = object.m_refresh_flags_guard;

	if constexpr (UPDATE)
	{
		SET(guard, kRefreshUpdate);

		if constexpr (REFLEX_DEBUG) CLEAR(flags, kRefreshUpdate);

		object.OnUpdate();

		g_debug_counters[WindowClient::kDebugCounterUpdate]++;
	}

	if constexpr (CHILDREN)
	{
		auto itr = UPCAST(object).GetFirst();

		while (itr)
		{
			auto & child = *itr;

			//auto next = child.GetNext();

			ProcessUpdateBinder::Bind(child.m_refresh_flags.GetWord())(child);

			itr = child.GetNext();

			//REFLEX_ASSERT(next == itr);
		}
	}
	else if (UPDATE & flags.Check(kRefreshChildren))	//refresh children could be requested during update
	{
		auto itr = UPCAST(object).GetFirst();

		while (itr)
		{
			auto & child = *itr;

			//auto next = child.GetNext();

			ProcessUpdateBinder::Bind(child.m_refresh_flags.GetWord())(child);

			itr = child.GetNext();

			//REFLEX_ASSERT(next == itr);
		}
	}

	if constexpr (UPDATE)
	{
		CLEAR(guard, kRefreshUpdate);

		CLEAR(flags, kRefreshUpdate);	//clear reentrant guard
	}
}

template <bool CHILDREN, bool UPDATE, bool REQUESTLAYOUT, bool ACCOMMODATE, bool REALIGN, bool REDRAW> void Reflex::GLX::Core::Object::Internal::ProcessRefresh(Object & core_object)
{
	REFLEX_INLINE_LOCAL(void,ProcessAccommodate)(Object & object, Flags8 & guard)
	{
		SET(guard, kRefreshAccommodate);

		RemoveConst(object.isresponsive) = false;

		RemoveConst(object.contentsize) = {};

		object.m_accommodate(UPCAST(object), RemoveConst(object.isresponsive), RemoveConst(object.contentsize));

		g_debug_counters[WindowClient::kDebugCounterAccommodate]++;

		CLEAR(guard, kRefreshAccommodate);
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(void,ProcessRefresh)(Object & object, Flags8 & guard)
	{
		SET(guard, kRefreshAlign);

		auto & content_h = RemoveConst(object.contentsize).h;

		auto align = object.m_align;

		bool isresponsive = object.isresponsive;

		align(UPCAST(object), isresponsive, content_h);

		if (object.isresponsive)
		{
			align(UPCAST(object), isresponsive, content_h);

			g_debug_counters[WindowClient::kDebugCounterAlignResponsive]++;
		}

		g_debug_counters[WindowClient::kDebugCounterAlign]++;

		CLEAR(guard, kRefreshAlign);
	}
	REFLEX_END

	auto & object = UPCAST(core_object);

	Flags8 & flags = core_object.m_refresh_flags;

	Flags8 & guard = core_object.m_refresh_flags_guard;

	if constexpr (REQUESTLAYOUT)
	{
		SET(guard, kRefreshSetLayout);

		core_object.OnBuildLayout(core_object.m_accommodate, core_object.m_align);

		g_debug_counters[WindowClient::kDebugCounterRebuild]++;

		CLEAR(guard, kRefreshSetLayout);

		ProcessAccommodate::Call(object, guard);

		ProcessRefresh::Call(object, guard);
	}
	else if constexpr (ACCOMMODATE)
	{
		ProcessAccommodate::Call(object, guard);

		ProcessRefresh::Call(object, guard);
	}
	else if constexpr (REALIGN)
	{
		ProcessRefresh::Call(object, guard);
	}

	if constexpr (CHILDREN)
	{
		auto itr = object.GetFirst();

		while (itr)
		{
			itr->FastRefresh();

			itr = itr->GetNext();
		}

		object.m_redraw = true;
	}
	else if ((UPDATE|REQUESTLAYOUT|ACCOMMODATE|REALIGN) & flags.Check(kRefreshChildren))
	{
		auto itr = UPCAST(object).GetFirst();

		while (itr)
		{
			itr->FastRefresh();

			itr = itr->GetNext();
		}

		object.m_redraw = true;
	}
	else if constexpr ((REQUESTLAYOUT | ACCOMMODATE | REALIGN | REDRAW))
	{
		object.m_redraw = true;
	}

	flags.Clear();

	//REFLEX_ASSERT(refresh_flags.Empty());
}

#undef UPCAST

template <Reflex::GLX::Core::Trap TRAP> bool Reflex::GLX::Core::Object::Internal::FindPointerTargetImpl(PointerAction action, const Pointer & pointer, UInt8 flags, Point position, const System::Renderer::Transform & transform, Object & object, Object * & result)
{
	if constexpr (TRAP == kTrapReject) return false;

	auto t = GLX::Detail::TransformMatrix(transform, object.GetRect().origin, object.GetScale());

	if constexpr (TRAP != kTrapThru) result = &object;

	if constexpr (TRAP != kTrapActive)
	{
		REFLEX_RFOREACH(child, object)
		{
			Point pos = position;

			pos -= child.GetRect().origin;

			pos /= child.GetScale();

			if (GLX::Contains(child.m_rect.size, pos))
			{
				auto trap = Cast<Object>(child)->OnPointerTender(action, pointer, flags);

				if ((*kFindPointerTargetFns[trap])(action, pointer, flags, pos, t, child, result)) return true;
			}
		}
	}

	return TRAP != kTrapThru;
}

void Reflex::GLX::Core::WindowClient::SetContent(GLX::Object & object)
{
	REFLEX_ASSERT(Or(object.GetAllocator(), IsNull(object)));

	m_content->Detach();

	m_content = object;

	m_content->SetSize(m_rect.size);

	Core::Object::Internal::AttachToWindow(*Cast<GLX::WindowClient>(this), object);
}
