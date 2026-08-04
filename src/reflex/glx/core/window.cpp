#include "impl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Core)

Float64 g_touch_time_z = 0.0;

extern UInt16 g_debug_counters[WindowClient::kNumDebugCounter];

REFLEX_END_INTERNAL

Reflex::GLX::Core::WindowClient::WindowClient()	//for NULL
	: profiler(Null<Output::Profiler>())
	, m_display_mode(System::kWindowDisplayMinimised)
	, m_mousecursor(System::kMouseCursorArrow)
{
	Retain(profiler);
}

Reflex::GLX::Core::WindowClient::WindowClient(bool profile)
	: profiler(profile ? New<Output::Profiler>(output, "Refresh") : Null<Output::Profiler>())
	, m_background_colour(kWhite)
	, m_display_mode(System::kWindowDisplayMinimised)
	, m_mousecursor(System::kMouseCursorArrow)
{
	Attach(Cast<DesktopImpl>(desktop)->m_windows);

	Retain(profiler);
}

Reflex::GLX::Core::WindowClient::~WindowClient()
{
	for (auto & pointer : Cast<DesktopImpl>(desktop)->m_pointers)
	{
		if (pointer.window.Adr() == this)
		{
			pointer.window = GLX::WindowClient::null;
		}
	}

	Detach();

	m_content->Detach();

	m_content.Clear();

	Release(profiler);

	Data::Detail::DestroyProperties(m_properties);
}

void Reflex::GLX::Core::WindowClient::SetTitle(const WString & title)
{
	owner->SetTitle(title);

	profiler.RemoveConst()->SetName(ToCString(title));
}

void Reflex::GLX::Core::WindowClient::SetRect(const Rect & rect)
{
	m_rect = rect;

	owner->SetRect({ { ToInt32(rect.origin.x), ToInt32(rect.origin.y) }, { ToInt32(rect.size.w), ToInt32(rect.size.h) } });
}

void Reflex::GLX::Core::WindowClient::BeginDragDrop(const ArrayView <WString> & files)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	desktop_impl->EndMouseCapture(ctx.window);

	desktop_impl->CancelDragDrop();

	owner->BeginDragDropFiles(files);

	m_mousecursor = System::kNumMouseCursor;
}

void Reflex::GLX::Core::WindowClient::Refresh(Context & ctx)
{
	if (m_canvas)
	{
		REFLEX_ASSERT(!RenderContext::st_current);

		MemClear(g_debug_counters, kNumDebugCounter * 2);
		
		auto & content = *m_content;
		
		content.FastRefresh();
		
		auto isize = m_canvas_size.a;
		
		RenderContext render_ctx = { .canvas = m_canvas.Adr(), .viewport = {{}, isize}, .clip_rect = SIMD::IntV4(0, 0, isize.w, isize.h)};
		
		RenderContext::st_current = &render_ctx;
		
		render_ctx.canvas->SetCurrent();
		
		g_renderer->Clear(m_background_colour);
		
		content.Draw(render_ctx);
		
		render_ctx.canvas->Flush();
		
		MemCopy(g_debug_counters, m_debug_counters, kNumDebugCounter * 2);
		
		RenderContext::st_current = nullptr;
	}
}

void Reflex::GLX::Core::WindowClient::OnSetOwner(System::Window * window)
{
	DesktopImpl::Context ctx(*this);

	if (window)
	{
		RemoveConst(owner) = window;

		//defer canvas creation until OnSetRect so we only create valid, ready to use canvas
	}
	else
	{
		RemoveConst(owner) = {};

		m_canvas.Clear();
	}
}

void Reflex::GLX::Core::WindowClient::OnDestruct()
{
	auto retain = AutoRelease(GetAbstractProperty(*this, K32("library")));

	REFLEX_ASSERT(IsValid(retain) || IsNull(this));

	DesktopImpl::Context ctx(*this);

	Object::OnDestruct();
}

void Reflex::GLX::Core::WindowClient::OnSetFocus()
{
	auto desktop_impl = Cast<DesktopImpl>(desktop);

	if (desktop_impl->m_focus->m_window.Adr() != this)
	{
		DesktopImpl::Context ctx(*this);

		desktop_impl->SetFocus(m_content);
	}
}

void Reflex::GLX::Core::WindowClient::OnLoseFocus()
{
	DesktopImpl::Context ctx(*this);

	desktop->CancelDragDrop();
}

void Reflex::GLX::Core::WindowClient::OnMouseEnter()
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	auto mouse = desktop_impl->GetMousePointer();

	mouse->window = Cast<GLX::WindowClient>(this);

	auto mousecursor = m_mousecursor;

	m_mousecursor = System::kNumMouseCursor;

	desktop_impl->UpdateMouseOverCursor(*this, mousecursor);
}

void Reflex::GLX::Core::WindowClient::OnMouseLeave()
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	auto mouse = desktop_impl->GetMousePointer();

	desktop_impl->SetMouseOver(GLX::Object::null);

	mouse->window = GLX::WindowClient::null;
}

void Reflex::GLX::Core::WindowClient::OnMouseMove(System::iPoint position)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	auto mouse = desktop_impl->GetMousePointer();

	Point fmousepos = { ToFloat32(position.x), ToFloat32(position.y) };

	switch (mouse->capture_mode)
	{
	case kTrapThru:
	case kTrapPassive:
	case kTrapActive:	//capture
		m_mousepos = fmousepos;
		break;

	case kTrapActiveIncremental:
		{
			auto capture_origin = mouse->capture_origin;

			m_mousepos += (fmousepos - capture_origin);

			mouse->capture_mode = kNumTrap;	//prevent feedback

			owner->SetMousePosition({ Truncate(capture_origin.x), Truncate(capture_origin.y) });

			mouse->capture_mode = kTrapActiveIncremental;
		}
		break;

	case kNumTrap:
		//feedback from incremental shift
		break;
	};

	auto timestamp = desktop_impl->m_time_seconds_z;

	if (!desktop_impl->UpdatePointerPosition(ctx, timestamp, mouse, m_mousepos))
	{
		desktop_impl->SetMouseOver(DesktopImpl::FindPointerTargetInWindow(*this, kPointerActionMouseMove, 0, mouse));
	}
}

void Reflex::GLX::Core::WindowClient::OnMouseDown(bool rmb, bool dbl)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	UInt8 flags = (rmb ? kPointerFlagRightMouseButton : 0) | (dbl ? kPointerFlagDouble : 0);

	desktop_impl->PointerDown(ctx, desktop_impl->m_time_seconds_z, desktop_impl->GetMousePointer(), m_mousepos, flags);

	desktop_impl->SetMouseOver(desktop_impl->GetMousePointer()->target);
}

void Reflex::GLX::Core::WindowClient::OnMouseUp(bool rmb)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	desktop_impl->PointerUp(ctx.window, desktop_impl->m_time_seconds_z, desktop_impl->GetMousePointer());
}

void Reflex::GLX::Core::WindowClient::OnMouseWheel(Point delta, bool high_res, bool inverted)
{
	static constexpr auto Post = [](WindowClient * self, const Point & delta, bool inverted)
	{
		auto desktop_impl = Cast<DesktopImpl>(desktop);

		auto mouse = desktop_impl->GetMousePointer();

		if (mouse->capture_mode == kTrapThru)
		{
			DesktopImpl::Context ctx(*self);

			desktop_impl->SetMouseOver(DesktopImpl::FindPointerTargetInWindow(*self, kPointerActionMouseWheel, 0, mouse));

			DOWN_CAST(*desktop_impl->GetMouseOver()).OnMouseWheel(*mouse, desktop_impl->m_time_seconds_z, delta, inverted);
		}
	};

	Post(this, delta, inverted);

//	static constexpr Float32 kTauInRcp = 1.0f / 0.06f;   // input smoothing ~60ms
//	static constexpr Float32 kTauTailRcp = 1.0f / 0.20f;   // decay when idle ~200ms
//	static constexpr Float32 kEpsStop = 0.05f;   // px/frame stop threshold
//	//static constexpr Float32 kVelMax = 12000.0f;
//	//static constexpr Float32 kPerTickMax = 240.0f;
//
//	if (high_res||true)
//	{
//		Post(this, delta);
//	}
//	else
//	{
//		m_wheel_accumulate += delta;
//
//		m_wheel_smoother = desktop->CreateAnimationClock([this](Float32 dt)
//		{
//			if (dt > 0.0f)
//			{
//				auto batch = Poll(m_wheel_accumulate);
//
//				Size dt_size = MakeSize(dt);
//
//				Point target = batch / dt_size;
//
//				auto a_in = 1.0f - Exp(-dt * kTauInRcp);
//
//				m_wheel_velocity += (target - m_wheel_velocity) * MakeSize(a_in);
//
//				if (Reinterpret<UInt64>(target) == 0)
//				{
//					m_wheel_velocity *= MakeSize(Exp(-dt * kTauTailRcp));
//				}
//
//				//m_wheel_velocity = Clip(m_wheel_velocity, MakePoint(-kVelMax, -kVelMax), MakePoint(kVelMax, kVelMax));
//
//				//Point out = Clip(m_wheel_velocity * dt_size, MakePoint(-kPerTickMax, -kPerTickMax), MakePoint(kPerTickMax, kPerTickMax));
//
//				Point out = m_wheel_velocity * dt_size;
//
//				if (Reinterpret<UInt64>(out))
//				{
//					Post(this, out);
//				}
//
//				if (Abs(m_wheel_velocity.x) < kEpsStop && Abs(m_wheel_velocity.y) < kEpsStop)
//				{
//					m_wheel_velocity = {};
//
//					m_wheel_smoother.Clear();
//				}
//			}
//		});
//	}
}

void Reflex::GLX::Core::WindowClient::OnTouchBegin(UIntNative touch_id, Float64 timestamp, System::fPoint position)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	REFLEX_ASSERT(!desktop_impl->QueryPointerByTouchID(touch_id));

	auto dbl = SetDelta(g_touch_time_z, timestamp) < 0.3;

	UInt8 slot = 1;

	while (slot < kMaxUInt8)
	{
		if (!desktop_impl->QueryPointer(slot))
		{
			auto & pointer = desktop_impl->m_pointers.Push({ Cast<GLX::WindowClient>(this) });

			pointer.touch_id = touch_id;
			pointer.slot = slot;

			desktop_impl->PointerDown(ctx, timestamp, pointer, position, dbl ? kPointerFlagDouble : 0);

			break;
		}

		++slot;
	}
}

void Reflex::GLX::Core::WindowClient::OnTouchMove(UIntNative touch_id, Float64 timestamp, System::fPoint position)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	if (auto pointer = desktop_impl->QueryPointerByTouchID(touch_id))
	{
		desktop_impl->UpdatePointerPosition(ctx, timestamp, *pointer, position);
	}
}

void Reflex::GLX::Core::WindowClient::OnTouchEnd(UIntNative touch_id, Float64 timestamp)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	if (auto pointer = desktop_impl->QueryPointerByTouchID(touch_id))
	{
		desktop_impl->PointerUp(ctx.window, timestamp, *pointer);

		auto & pointers = desktop_impl->m_pointers;

		pointers.Remove(UInt(pointer - pointers.GetData()));
	}
}

void Reflex::GLX::Core::WindowClient::OnTouchCancel(UIntNative touch_id, Float64 timestamp)
{
	OnTouchEnd(touch_id, timestamp);
}

bool Reflex::GLX::Core::WindowClient::OnKeyPress(System::KeyCode keycode, bool repeat)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	if (keycode == System::kKeyCodeEscape)
	{
		if (desktop_impl->m_dragdrop_data)
		{
			desktop_impl->CancelDragDrop();

			return true;
		}
	}

	auto & lock = desktop_impl->m_keycapture.Acquire(keycode);

	if (repeat)
	{
		return DOWN_CAST(*lock).OnKeyPress(keycode, repeat);
	}
	else
	{
		auto & focus = *desktop_impl->m_focus;

		auto & target = focus.GetWindow().Adr() == this ? focus : *m_content;

		lock = target;

		return DOWN_CAST(*lock).OnKeyPress(keycode, repeat);
	}
}

void Reflex::GLX::Core::WindowClient::OnKeyRelease(System::KeyCode keycode)
{
	DesktopImpl::Context ctx(*this);

	auto & keycapture = Cast<DesktopImpl>(desktop)->m_keycapture;

	if (Idx idx = keycapture.Search(keycode))
	{
		auto & lock = keycapture[idx.value].value;

		DOWN_CAST(*lock).OnKeyRelease(keycode);

		keycapture.Remove(idx.value);
	}
}

bool Reflex::GLX::Core::WindowClient::OnCharacter(WChar character)
{
	auto & keycapture = Cast<DesktopImpl>(desktop)->m_keycapture;

	auto & focus = *Cast<DesktopImpl>(desktop)->m_focus;

	for (auto & i : keycapture)		//only pass to objects that request them via OnKeyPress (can happen on focus change)
	{
		if (i.value == focus)
		{
			DesktopImpl::Context ctx(*this);

			return DOWN_CAST(focus).OnCharacter(character);
		}
	}

	return false;
}

void Reflex::GLX::Core::WindowClient::OnSetRect(System::WindowDisplay mode, const System::iRect & rect, const System::iRect & interactable, Int32 dpifactor)
{
	if (SetFiltered(m_display_mode, mode)) m_content->Update();

	if (mode != System::kWindowDisplayMinimised)
	{
		auto & [origin,size] = rect;

		m_rect = { { Float(origin.x), Float(origin.y) }, { Float(size.w), Float(size.h) } };

		if (SetFiltered(m_canvas_size, MakeTuple(rect.size, Int8(dpifactor), true)))
		{
			DesktopImpl::Context ctx(*this);

			g_renderer->BeginAccess();	//force, this can come outside of refresh loop

			if (!m_canvas && owner) m_canvas = g_renderer->CreateCanvas(owner->GetSystemHandle());

			if (!m_canvas->SetSize({ Max(size.w, 1), Max(size.h, 1) }, dpifactor))
			{
				m_canvas.Clear();	//abort using invalid canvas
			}

			m_content->SetSize(m_rect.size);

			Refresh(ctx);
		}
	}
}

void Reflex::GLX::Core::WindowClient::OnDrop(const Object & object)
{
	DesktopImpl::Context ctx(*this);

	auto desktop_impl = Cast<DesktopImpl>(desktop);

	WeakReference target(*DesktopImpl::FindPointerTargetInWindow(*this, kPointerActionDragAndDrop, 0, desktop_impl->GetMousePointer()));

	do
	{
		if (DOWN_CAST(target).OnDragDropReceiveExternal(object)) break;

		target = target->GetParent();
	}
	while (*target);

	desktop_impl->EndMouseCapture(ctx.window);
}

void Reflex::GLX::Core::WindowClient::OnRequestClose()
{
	DesktopImpl::Context ctx(*this);

	OnClose();
}

void Reflex::GLX::Core::WindowClient::OnUnsetProperty(Address address)
{
	Data::Detail::ClearObject(m_properties, address);
}

void Reflex::GLX::Core::WindowClient::OnSetProperty(Address address, Object & value)
{
	Data::Detail::SetObject(m_properties, address, value);
}

void Reflex::GLX::Core::WindowClient::OnQueryProperty(Address address, Object * & pointer) const
{
	Data::Detail::QueryObject(m_properties, address, pointer);
}
