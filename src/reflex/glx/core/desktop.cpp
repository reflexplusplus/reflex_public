#include "impl.h"




//
//desktop

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Core)

REFLEX_INLINE TRef <System::Renderer> CreateRenderer(System::Renderer::Config & config)
{
	struct Compare
	{
		static bool eq(const Pair <CString::View, System::Renderer::EngineCtr> & a, UInt32 b) { return MakeKey32(a.a) == b; }
	};

	auto engines = System::Renderer::GetEngines();

	Key32 default_engine_id = engines ? MakeKey32(engines.GetFirst().a) : kNullKey;

	auto engine_id = System::Common::GetValue(config, kEngine, default_engine_id.value);

	Idx start = Search<Compare>(engines, engine_id);

	UInt offset = start ? start.value : 0;

	TRef <System::Renderer> renderer;

	REFLEX_LOOP(idx, engines.GetSize())
	{
		auto & i = engines[(idx + offset) % engines.GetSize()];

		renderer = i.b(config);

		if (renderer->Status())
		{
			break;
		}
		else if (renderer)
		{
			AutoRelease(renderer);

			renderer = {};
		}
	}

	config.Set(kEngine, engine_id);

	for (auto & i : renderer->GetConfig()) config.Set(i.key, i.value);

	return renderer;
}

REFLEX_INLINE bool GetIncrementalMouse(const System::Renderer::Config & config)
{
	bool incremental = System::Common::GetValue(config, kIncrementalMouse, true);

	switch (System::kPlatform)
	{
	case System::kPlatformMacOS:
		return incremental && (System::GetScreens().GetSize() == 1);

	case System::kPlatformAndroid:
	case System::kPlatformIOS:
		return false;

	default:
		return incremental;
	}
}

void DesktopImpl::ResetClock()
{
	m_time_seconds_z = System::GetElapsedTime();
}

DesktopImpl::Statics DesktopImpl::st_statics;

DesktopImpl::DesktopImpl(File::ResourcePool & resourcepool, const System::Renderer::Config & config)
	: Desktop(resourcepool)
	, m_config(config)
	, m_renderer(CreateRenderer(m_config))
	, m_clock_listener(System::CreateListener(System::kNotificationClock, this, &DesktopImpl::OnClock))
	, m_monitor(*this)
{
	RemoveConst(g_renderer) = m_renderer;

	REFLEX_ASSERT(IsNull(g_renderer) || g_renderer->GetContextID() == GetContextID());

	bool incremental_mouse = GetIncrementalMouse(config);

	m_config.Set(kIncrementalMouse, incremental_mouse);

	st_statics.trap_to_capture[kTrapThru] = kTrapThru;
	st_statics.trap_to_capture[kTrapPassive] = kTrapPassive;
	st_statics.trap_to_capture[kTrapActive] = kTrapActive;
	st_statics.trap_to_capture[kTrapActiveIncremental] = incremental_mouse ? kTrapActiveIncremental : kTrapActive;

	auto & mouse = m_pointers.Push({&GLX::WindowClient::null});

	mouse.slot = 0;
	mouse.touch_id = ~UIntNative(0);
}

DesktopImpl::~DesktopImpl()
{
	Accessor::GetAttachFlags(GLX::Object::null).Clear();

	m_focus.Clear();

	m_pointers.Clear();

	m_keycapture.Clear();

	RemoveConst(desktop) = kNoValue;

	RemoveConst(g_renderer) = {};
}

TRef <Reflex::Object> DesktopImpl::CreateAnimationClock(const Function <void(Float32)> & listener)
{
	return m_animation_clock.CreateListener(listener);
}

TRef <Reflex::Object> DesktopImpl::CreateListener(Notification n, const Function <void()> & listener)
{
	return m_signals[n].CreateListener(listener);
}

void DesktopImpl::SetFocus(GLX::Object & object)
{
	if (m_focus != object)
	{
		WeakReference prev = m_focus;

		m_focus = object;

		Accessor::GetAttachFlags(*prev).Clear(kHasFocus);

		Accessor::CallLoseFocus(prev);

		Accessor::GetAttachFlags(m_focus).Set(kHasFocus);

		Accessor::CallFocus(m_focus);

		m_signals[kNotificationFocus].Notify();
	}
}

TRef <GLX::Object> DesktopImpl::GetFocus()
{
	return *m_focus;
}

TRef <GLX::Object> DesktopImpl::GetMouseOver()
{
	return *m_mouseover;
}

TRef <Reflex::Object> DesktopImpl::GetDragDropData()
{
	return m_dragdrop_data;
}

TRef <GLX::Object> DesktopImpl::GetDragDropTarget()
{
	return *m_dragdrop_target;
}

void DesktopImpl::EnumerateWindows(const Function <void(GLX::WindowClient&)> & callback)
{
	for (auto & i : m_windows) callback(i);
}

void DesktopImpl::EndPointerCapture(UInt8 slot_id)
{
	if (auto pointer = RemoveConst(QueryPointer(slot_id)))
	{
		if (pointer->pressed)
		{
			DesktopImpl::Context ctx(pointer->target->GetWindow());

			PointerUp(ctx.window, m_time_seconds_z, *pointer);
		}
	}
}

void DesktopImpl::StartDragDrop(UInt8 pointer_slot, TRef <Reflex::Object> data, System::MouseCursor over, System::MouseCursor block)
{
	if (auto pointer = RemoveConst(QueryPointer(pointer_slot)))
	{
		if (pointer->capture_mode != kTrapThru && !m_dragdrop_data)
		{
			Context ctx(pointer->window);

			EndPointerCapture(pointer->slot);

			//todo - call to system window to start drag and drop, this will release capture

			pointer->drag_and_drop = true;

			m_dragdrop_data = data;

			st_statics.drag_cursors = { block, over };

			m_signals[kNotificationDragDropBegin].Notify();	//!2026 change order, allow Notify clients to know where it started (as no more drag source)

			SetMouseOver(GLX::Object::null);

			SetDragOver(ctx.window, FindDragOverInWindow(ctx.window, *pointer));

			UpdateDragDropCursor(ctx.window);
		}
	}
}

void DesktopImpl::PointerDown(Context & ctx, Float64 timestamp, Pointer & pointer, Point position, UInt8 flags)
{
	REFLEX_ASSERT(pointer.window == ctx.window);
	REFLEX_ASSERT(pointer.capture_mode == kTrapThru);

	for (auto & i : m_pointers)
	{
		if (i.pressed)
		{
			flags |= kPointerFlagMulti;

			break;
		}
	}

	pointer.position = position;

	pointer.target = FindPointerTargetInWindow(ctx.window, kPointerActionPress, flags, pointer);

	if (pointer.capture_mode == kTrapThru)
	{
		pointer.pressed = true;

		SetFocus(pointer.target);

		pointer.capture_mode = kTrapPassive;
		pointer.capture_origin = position;
		pointer.drag_z = kMaxUInt64;

		Trap trap = CallPointerDown(pointer.target, pointer, flags, timestamp);

		if (pointer.pressed)
		{
			pointer.capture_mode = DesktopImpl::st_statics.trap_to_capture[trap];
		}
		else
		{
			pointer.capture_mode = kTrapThru;
		}
	}
}

void DesktopImpl::PointerUp(WindowClient & window, Float64 timestamp, Pointer & pointer)
{
	REFLEX_ASSERT(pointer.window == window || IsNull(Cast<GLX::WindowClient>(window)));

	auto & target = pointer.target;

	if (pointer.capture_mode >= kTrapActive)
	{
		auto capture_mode = pointer.capture_mode;

		pointer.capture_mode = kTrapThru;		//this will ensure that StartDragDrop cant be reentrant

		CallPointerUp(target, pointer, timestamp);

		if (capture_mode == kTrapActiveIncremental)
		{
			auto position = pointer.capture_origin;

			window.owner->SetMousePosition({ Truncate(position.x), Truncate(position.y) });
		}
	}
	else
	{
		CompleteDragDrop();
	}

	pointer.pressed = false;
	pointer.capture_mode = kTrapThru;
	pointer.target.Clear();
}

bool DesktopImpl::UpdatePointerPosition(Context & ctx, Float64 timestamp, Pointer & pointer, Point position)
{
	REFLEX_ASSERT(pointer.window == ctx.window);

	Notify();

	pointer.position = position;

	if (pointer.drag_and_drop)
	{
		SetDragOver(ctx.window, FindDragOverInWindow(ctx.window, pointer));

		return true;
	}
	else if (pointer.capture_mode >= kTrapActive)
	{
		auto delta = position - pointer.capture_origin;

		if (SetFiltered(pointer.drag_z, Reinterpret<UInt64>(delta)))
		{
			CallPointerDrag(pointer.target, pointer, timestamp, delta);
		}

		return true;
	}

	return false;
}

void DesktopImpl::CompleteDragDrop()
{
	if (auto pointer = QueryDragDropPointer())
	{
		pointer->drag_and_drop = false;

		CallDragDropReceive(m_dragdrop_target, m_dragdrop_data);

		m_signals[kNotificationDragDropEnd].Notify();

		auto window = m_dragdrop_target->GetWindow();

		SetDragOver(window, GLX::Object::null);

		SetMouseOver(FindPointerTargetInWindow(window, kPointerActionDragAndDrop, 0, *pointer));

		m_dragdrop_data.Clear();
	}
}

void DesktopImpl::CancelDragDrop()
{
	SetDragOver(m_dragdrop_target->GetWindow(), GLX::Object::null);

	CompleteDragDrop();
}

void DesktopImpl::OnSetProperty(Address adr, Reflex::Object & object)
{
	if (adr == MakeAddress<Data::BoolProperty>(K32("thrash")))
	{
		OnClock(this);
	}

	Reflex::Object::OnSetProperty(adr, object);
}

void DesktopImpl::OnClock(void * pdesktop)
{
	//get delta time

	auto self = Cast<DesktopImpl>(pdesktop);

	auto time = System::GetElapsedTime();

	auto delta = Min(Float(SetDelta(self->m_time_seconds_z, time)), 0.03333333333333f);



	//context

	Core::Context ctx;		//(true);

	g_renderer->BeginAccess();	//force context to be set, because clock could be called while dialog is showing


	
	//clock: TODO should be per-window for valid current window

	auto & clocklist = self->m_clocklist;

	for (UInt idx = 0; idx < clocklist.GetSize(); idx++)
	{
		auto pclocklist = clocklist.GetData();

		auto pobject = pclocklist[idx]->key;

		REFLEX_ASSERT(pobject->GetWindow());

		Accessor::CallOnClock(*pobject, delta);
	}

	self->m_animation_clock.Notify(delta);



	//update windows

	SetDelta(time, System::GetElapsedTime());	//clock time

	SafeReverseIterate(self->m_windows, [&ctx, &time](WindowClient & window)
	{
		window.Refresh(ctx);

		RemoveConst(*window.profiler).Write(SetDelta(time, System::GetElapsedTime()) * 1000.0);
	});



	//cursor

	if (self->m_monitor.Poll())
	{
		if (!self->QueryDragDropPointer())
		{
			self->UpdateMouseOverCursor(self->GetMousePointer()->window, GetMouseCursor(self->m_mouseover));
		}
	}
}

void DesktopImpl::SetDragOver(WindowClient & window, GLX::Object & object)
{
	auto prev = m_dragdrop_target.Adr();

	if (prev != &object)
	{
		m_dragdrop_target = object;

		CallDragDropLeave(*prev, m_dragdrop_data);

		CallDragDropEnter(m_dragdrop_target, m_dragdrop_data);

		m_signals[kNotificationDragDropTarget].Notify();

		UpdateDragDropCursor(window);
	}
}

REFLEX_END_INTERNAL

const Reflex::TRef <Reflex::System::Renderer> Reflex::GLX::Core::g_renderer = kNoValue;

Reflex::GLX::Core::Desktop::Desktop(File::ResourcePool & resourcepool)
	: resourcepool(resourcepool)
{
	RemoveConst(desktop) = this;
}
