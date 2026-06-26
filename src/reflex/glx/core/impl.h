#pragma once

#include "../../../../include/reflex/glx/core/desktop.h"
#include "../../../../include/reflex/glx/window.h"




//
//

#define DOWN_CAST(object) (static_cast<Core::Object&>(object))

REFLEX_NS(Reflex::System::Common)

extern UInt GetValue(const Renderer::Config & config, Key32 key, UInt32 fallback);

REFLEX_END

struct Reflex::GLX::Core::Accessor
{
	REFLEX_INLINE static System::MouseCursor & GetMouseCursor(WindowClient & window) { return window.m_mousecursor; }

	REFLEX_INLINE static System::MouseCursor GetMouseCursor(Object & object) { return object.m_mouse_cursor; }

	REFLEX_INLINE static void CallOnClock(Object & object, Float32 delta) { object.OnClock(delta); }

	REFLEX_INLINE static void CallMouseEnter(Object & object, GLX::Object & arg) { object.OnMouseEnter(arg); }
	REFLEX_INLINE static void CallMouseLeave(Object & object) { object.OnMouseLeave(); }

	REFLEX_INLINE static Trap CallPointerDown(Object & object, const Pointer & pointer, UInt8 flags, Float64 timestamp) { return object.OnPointerDown(pointer, flags, timestamp); }
	REFLEX_INLINE static void CallPointerDrag(Object & object, const Pointer & pointer, Float64 timestamp, System::fPoint delta) { object.OnPointerDrag(pointer, timestamp, delta); }
	REFLEX_INLINE static void CallPointerUp(Object & object, const Pointer & pointer, Float64 timestamp) { object.OnPointerUp(pointer, timestamp); }

	REFLEX_INLINE static void CallFocus(Object & object) { object.OnFocus(); }
	REFLEX_INLINE static void CallLoseFocus(Object & object) { object.OnLoseFocus(); }

	REFLEX_INLINE static bool CallDragDropTender(Object & object, const Pointer & pointer, Reflex::Object & arg) { return object.OnDragDropTender(pointer, arg); }
	REFLEX_INLINE static void CallDragDropEnter(Object & object, Reflex::Object & arg) { object.OnDragDropEnter(arg); }
	REFLEX_INLINE static void CallDragDropLeave(Object & object, Reflex::Object & arg) { object.OnDragDropLeave(arg); }
	REFLEX_INLINE static void CallDragDropReceive(Object & object, Reflex::Object & arg) { object.OnDragDropReceive(arg); }

	REFLEX_INLINE static Flags8 & GetAttachFlags(Object & object) { return object.m_attach_flags; }

	REFLEX_INLINE static UInt8 CallComputeStyle(GLX::Object & object) { return object.ComputeStyle(); }

	REFLEX_INLINE static decltype(auto) CallSetRenderer(Object & object, Renderer & renderer, Int8 zindex) { object.SetRenderer(renderer, zindex); }
};

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Core)

UInt16 g_debug_counters[WindowClient::kNumDebugCounter];

enum AttachDetachFlags
{
	kRequestOnAttachDetach,
	kRequestOnClock,
	kHasZIndex,

	kHasWindow,
	kWillHaveNewWindow,
	kHasMouseOver = kWillHaveNewWindow,
	kHasFocus,

	kDetachDestroy,
};

enum RefreshFlags
{
	kRefreshChildren,
	kRefreshUpdate,
	kRefreshSetLayout,
	kRefreshAccommodate,
	kRefreshAlign,
	kRefreshRedraw,
};

enum DrawFlags
{
	kDrawUnused0,//kDrawEnable
	kDrawUnused1,//kDrawClipX
	kDrawUnused2,//kDrawClipY

	kDrawUnused3,//kDrawBG,
	kDrawUnused4,//kDrawChildren,
	kDrawUnused5,//kDrawFG,

	kDrawRedraw,
	kDrawSortChildren,
};

struct DesktopImpl : 
	public Desktop,
	public Accessor
{
	struct Context;


	DesktopImpl(File::ResourcePool & resourcepool, const System::Renderer::Config & config);

	~DesktopImpl();


	const System::Renderer::Config & GetGraphicsConfig() override { return m_config; }

	TRef <Reflex::Object> CreateAnimationClock(const Function <void(Float32)> & listener) override;

	TRef <Reflex::Object> CreateListener(Notification n, const Function <void()> & listener) override;

	void EnumerateWindows(const Function <void(GLX::WindowClient&)> & callback) override;

	const Pointer * QueryPointer(UInt8 slot) const override 
	{
		return SearchValue<FieldCompare<&Pointer::slot>>(m_pointers, slot);
	}

	ArrayView <Pointer> GetPointers() const override { return m_pointers; }

	void SetFocus(GLX::Object & object) override;

	TRef <GLX::Object> GetFocus() override;

	TRef <GLX::Object> GetMouseOver() override;

	void ResetClock() override;

	void EndPointerCapture(UInt8 slot) override;


	void StartDragDrop(UInt8 pointer_slot, TRef <Reflex::Object> data, System::MouseCursor over, System::MouseCursor leave) override;

	void CompleteDragDrop() override;

	void CancelDragDrop() override;

	TRef <GLX::Object> GetDragDropTarget() override;

	TRef <Reflex::Object> GetDragDropData() override;


	void OnSetProperty(Address adr, Reflex::Object & object) override;


	void PointerDown(Context & ctx, Float64 timestamp, Pointer & pointer, Point position, UInt8 flags);

	void PointerUp(WindowClient & window, Float64 timestamp, Pointer & pointer);

	bool UpdatePointerPosition(Context & ctx, Float64 timestamp, Pointer & pointer, Point position);


	TRef <Pointer> GetMousePointer()
	{
		return m_pointers.GetFirst();
	}

	void EndMouseCapture(WindowClient & window)
	{
		auto pointer = GetMousePointer();

		if (pointer->pressed)
		{
			PointerUp(window, m_time_seconds_z, RemoveConst(*pointer));
		}
	}

	REFLEX_NOINLINE Pointer * QueryPointerByTouchID(UIntNative touch_id)
	{
		return SearchValue<FieldCompare<&Pointer::touch_id>>(m_pointers, touch_id);
	}

	void UpdateMouseCursor(WindowClient & window, System::MouseCursor cursor)
	{
		if (SetFiltered(GetMouseCursor(window), cursor))
		{
			window.owner->SetMouseCursor(cursor);
		}
	}

	void UpdateMouseOverCursor(WindowClient & window, System::MouseCursor cursor)
	{
		auto mouse = GetMousePointer();

		if (mouse->capture_mode >= kTrapActiveIncremental) cursor = System::kMouseCursorInvisible;

		UpdateMouseCursor(window, cursor);
	}

	void SetMouseOver(GLX::Object & object)
	{
		auto & mouseover = m_mouseover;

		if (mouseover.Adr() != &object)
		{
			WeakReference prev = mouseover;

			mouseover = object;

			GetAttachFlags(prev).Clear(kHasMouseOver);

			CallMouseLeave(*prev);

			GetAttachFlags(mouseover).Set(kHasMouseOver);

			CallMouseEnter(*mouseover, prev);

			m_signals[Desktop::kNotificationMouseOver].Notify();

			UpdateMouseOverCursor(mouseover->GetWindow(), mouseover->GetMouseCursor());
		}
	}

	static TRef <GLX::Object> FindPointerTargetInWindow(WindowClient & window, PointerAction action, UInt8 flags, const Pointer & pointer)
	{
		return window.GetContent()->FindPointerTarget(action, pointer, flags, pointer.position);
	}

	TRef <GLX::Object> FindDragOverInWindow(WindowClient & window, const Pointer & pointer)
	{
		WeakReference over(FindPointerTargetInWindow(window, kPointerActionDragAndDrop, 0, pointer));

		WeakReference itr(over);

		while (itr)
		{
			if (CallDragDropTender(itr, pointer, m_dragdrop_data)) return *itr;

			itr = itr->GetParent();
		}

		return GLX::Object::null;
	}

	Pointer * QueryDragDropPointer()
	{
		return SearchValue<FieldCompare<&Pointer::drag_and_drop>>(m_pointers, true);
	}

	void UpdateDragDropCursor(WindowClient & window)
	{
		auto cursor = (&st_statics.drag_cursors.a)[True(m_dragdrop_target)];

		UpdateMouseCursor(window, cursor);
	}

	void SetDragOver(WindowClient & window, GLX::Object & object);


	static void OnClock(void * self);

	using State::Notify;



	System::Renderer::Config m_config;	//! must be first, modified in constructor

	Reference <System::Renderer> m_renderer;

	Reference <Reflex::Object> m_clock_listener;

	Monitor m_monitor;

	Float64 m_time_seconds_z;


	SignalComponent <Float> m_animation_clock;

	SignalComponent <> m_signals[kNumNotification];


	Array <Pointer> m_pointers;


	Reference <Reflex::Object> m_dragdrop_data;

	WeakReference m_mouseover, m_focus, m_dragdrop_target;

	Sequence <System::KeyCode, WeakReference> m_keycapture;


	Map <GLX::Core::Object*> m_clocklist;


	WindowClient::List m_windows;


	struct Statics
	{
		Pair <System::MouseCursor> drag_cursors;

		Trap trap_to_capture[kNumTrap];
	};

	static Statics st_statics;

};

struct DesktopImpl::Context : public Core::Context
{
	Context(WindowClient & window)
		: window(window)
	{
	}

	~Context()
	{
		Cast<DesktopImpl>(desktop)->Notify();
	}

	const TRef <WindowClient> window;
};

REFLEX_END_INTERNAL
