#include "window.h"

//
//window

REFLEX_BEGIN_INTERNAL(Reflex::System::Android)

KeyCode translateToKeyCode(int32_t androidKeyCode) {
	// TODO: Florian --
	return KeyCode(androidKeyCode);
}

UIntNative GetPointerId(AInputEvent *event, UInt32 ptrindex) {
	return AMotionEvent_getPointerId(event, ptrindex);
}

fPoint GetEventPosition(AInputEvent *event, UInt32 ptrindex) {
	auto screenDensity = float(globals->CurrentScreenDensity());
	return {
		AMotionEvent_getX(event, ptrindex) / screenDensity,
		AMotionEvent_getY(event, ptrindex) / screenDensity
	};
}

fPoint GetHistoricalEventPosition(AInputEvent *event, UInt32 ptrindex, UInt32 history_index) {
	auto screenDensity = float(globals->CurrentScreenDensity());
	return {
		AMotionEvent_getHistoricalX(event, ptrindex, history_index) / screenDensity,
		AMotionEvent_getHistoricalY(event, ptrindex, history_index) / screenDensity
	};
}

Float64 GetEventTimestamp(AInputEvent *event) {
	return Float64(AMotionEvent_getEventTime(event)) / 1000000000.0;
}

Float64 GetHistoricalEventTimestamp(AInputEvent *event, UInt32 history_index) {
	return Float64(AMotionEvent_getHistoricalEventTime(event, history_index)) / 1000000000.0;
}

REFLEX_END_INTERNAL

Reflex::System::Android::Window::Window(UInt32 styleFlags, bool ontop, void *systemparent)
{
	st_self = this;
}

Reflex::System::Android::Window::~Window() {
	m_client->OnSetOwner(nullptr);

	st_self = nullptr;
}

void Reflex::System::Android::Window::SetClient(TRef<Client> client) {
	m_client = client;
	m_lastSignaledMousePos = { -1, -1 };

	client->OnSetOwner(this);
}

void Reflex::System::Android::Window::ScheduleWindowRectChanged() {
	m_needSizeUpdate = true;
}

void Reflex::System::Android::Window::ProcessPendingTasks() {
	if (m_client && m_needSizeUpdate) {
		auto windowInsets = globals->CurrentScreenInsets();
		REFLEX_ASSERT(windowInsets.b.size.w > 0)

		m_client->OnSetRect(kWindowDisplayFullScreen, windowInsets.a, windowInsets.b, globals->CurrentScreenDensity());
		m_needSizeUpdate = false;
	}
}

void Reflex::System::Android::Window::OnSetFocus() {
	if (m_client) m_client->OnSetFocus();
}

void Reflex::System::Android::Window::OnLoseFocus() {
	if (m_client) m_client->OnLoseFocus();
}

void Reflex::System::Android::Window::UpdateWindowOnCreation() {
	REFLEX_USE(Jni)

	AttachedJavaEnv env;
	int orientation = /* ActivityInfo.SCREEN_ORIENTATION_UNSET */ -2;

	switch (m_client->OnGetScreenOrientation()) {
		case kScreenOrientationDefault:
			orientation = /* ActivityInfo.SCREEN_ORIENTATION_FULL_USER */ 13;
			break;

		case kScreenOrientationPortrait:
			orientation = /* ActivityInfo.SCREEN_ORIENTATION_USER_PORTRAIT */ 12;
			break;

		case kScreenOrientationLandscape:
			orientation = /* ActivityInfo.SCREEN_ORIENTATION_USER_LANDSCAPE */ 11;
			break;
	}

	g_reflexActivityInstance->SetRequestedOrientation(env, orientation);
}

int Reflex::System::Android::Window::HandleNextInputEvent(AInputEvent* event) {
	switch (AInputEvent_getType(event)) {
		case AINPUT_EVENT_TYPE_MOTION: {
			auto action = AMotionEvent_getAction(event);
			auto event_ptrindex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
			auto source = AInputEvent_getSource(event);

			switch (action & AMOTION_EVENT_ACTION_MASK) {
				case AMOTION_EVENT_ACTION_DOWN:
				case AMOTION_EVENT_ACTION_POINTER_DOWN: {
					if (source & AINPUT_SOURCE_TOUCHSCREEN) {
						auto timestamp = GetEventTimestamp(event);
						m_client->OnTouchBegin(GetPointerId(event, event_ptrindex), timestamp, GetEventPosition(event, event_ptrindex));
						return 1; // consumed
					}
					else if (source & AINPUT_SOURCE_MOUSE) {
						bool doubleclick = SetDelta(m_lastclicktime, GetElapsedTime()) < 0.3;
						SignalMoveIfNeeded(GetEventPosition(event, event_ptrindex));
						m_client->OnMouseDown(false, doubleclick);
						return 1; // consumed
					}
					break;
				}

				case AMOTION_EVENT_ACTION_CANCEL:
					if (source & AINPUT_SOURCE_TOUCHSCREEN) {
						auto timestamp = GetEventTimestamp(event);
						REFLEX_LOOP(idx, AMotionEvent_getPointerCount(event)) {
							m_client->OnTouchCancel(GetPointerId(event, idx), timestamp);
						}
						return 1; // consumed
					}
					break;

				case AMOTION_EVENT_ACTION_UP:
				case AMOTION_EVENT_ACTION_POINTER_UP: {
					if (source & AINPUT_SOURCE_TOUCHSCREEN) {
						auto timestamp = GetEventTimestamp(event);
						m_client->OnTouchEnd(GetPointerId(event, event_ptrindex), timestamp);
						return 1; // consumed
					}
					else if (source & AINPUT_SOURCE_MOUSE) {
						SignalMoveIfNeeded(GetEventPosition(event, event_ptrindex));
						m_client->OnMouseUp(false);
						return 1; // consumed
					}
					break;
				}

				case AMOTION_EVENT_ACTION_MOVE:
					if (source & AINPUT_SOURCE_TOUCHSCREEN) {
						// There is no pointer index for ACTION_MOVE, only a snapshot of all active pointers; app needs to cache previous active pointers to figure out which ones are actually moved.
						REFLEX_LOOP(history_index, AMotionEvent_getHistorySize(event)) {
							auto timestamp = GetHistoricalEventTimestamp(event, history_index);
							REFLEX_LOOP(idx, AMotionEvent_getPointerCount(event)) {
								m_client->OnTouchMove(GetPointerId(event, idx), timestamp, GetHistoricalEventPosition(event, idx, history_index));
							}
						}

						auto timestamp = GetEventTimestamp(event);
						REFLEX_LOOP(idx, AMotionEvent_getPointerCount(event)) {
							m_client->OnTouchMove(GetPointerId(event, idx), timestamp, GetEventPosition(event, idx));
						}
						return 1; // consumed
					}
					else if (source & AINPUT_SOURCE_MOUSE) {
						SignalMoveIfNeeded(GetEventPosition(event, event_ptrindex));
						return 1; // consumed
					}
					break;
			}
			break;
		}

		case AINPUT_EVENT_TYPE_KEY: {
			if (m_keyboardshown) break;

			switch (AKeyEvent_getAction(event)) {
				case AMOTION_EVENT_ACTION_DOWN: {
					auto keycode = AKeyEvent_getKeyCode(event);
					auto repeatcount = AKeyEvent_getRepeatCount(event);
					m_client->OnKeyPress(translateToKeyCode(keycode), repeatcount > 0);
					return ShouldConsume(keycode);
				}
				case AMOTION_EVENT_ACTION_UP: {
					auto keycode = AKeyEvent_getKeyCode(event);
					m_client->OnKeyRelease(translateToKeyCode(keycode));
					return ShouldConsume(keycode);
				}
			}
		}
	}

	return 0; // not consumed
}

void Reflex::System::Android::Window::HandleImeInput(UInt chars_to_erase, const WString& chars_to_insert)
{
	REFLEX_LOOP(i, chars_to_erase)
	{
		m_client->OnKeyPress(kKeyCodeBackspace, false);
		m_client->OnKeyRelease(kKeyCodeBackspace);
	}

	for (auto c : chars_to_insert)
	{
		if (c == '\n')
		{
			// TODO: [Florian] map like on iOS
			m_client->OnKeyPress(kKeyCodeEnter, false);
			m_client->OnKeyRelease(kKeyCodeEnter);
		}
		else if (auto trap = m_client->OnKeyPress(kNumKeyCode, false))
		{
			m_client->OnCharacter(c);
		}
	}
}

int Reflex::System::Android::Window::ShouldConsume(int keycode)
{
	// MEMO: for now we just don't consume any key event. We process them but we leave them to process to the system (otherwise the volume keys etc. won't bring up the ringer).
	return 0;
//	return keycode != AKEYCODE_VOLUME_UP && keycode != AKEYCODE_VOLUME_DOWN && keycode != AKEYCODE_VOLUME_MUTE;
}

void Reflex::System::Android::Window::SignalMoveIfNeeded(fPoint current) {
	iPoint pos = { Truncate(current.x), Truncate(current.y) };
	if (SetFiltered(m_lastSignaledMousePos, pos)) {
		m_client->OnMouseMove(pos);
	}
}

void Reflex::System::Android::Window::Attach(System::Window& parent) {
	// Unsupported on mobile
}

void Reflex::System::Android::Window::Detach() {
	// Unsupported on mobile
}

void Reflex::System::Android::Window::SetTitle(const WString& label) {
	// Unsupported on mobile
}

void Reflex::System::Android::Window::SetRect(const iRect& rect) {
	// Unsupported on mobile
}

void Reflex::System::Android::Window::SetDisplayMode(WindowDisplay state) {
	if (state != kWindowDisplayFullScreen) {
		DEV_WARN("Only full screen supported on mobile");
	}
}

void Reflex::System::Android::Window::SendTop() {
	// Unsupported on mobile
}

void Reflex::System::Android::Window::EnableInput(bool enable) {
}

void Reflex::System::Android::Window::SetMouseCursor(MouseCursor mousecursor) {
	// Unsupported on mobile
}

void Reflex::System::Android::Window::SetMousePosition(const iPoint& position) {
	// Unsupported on mobile
}

void Reflex::System::Android::Window::BeginDragDropFiles(const ArrayView <WString> & filenames) {
	// Unsupported on mobile
}

void* Reflex::System::Android::Window::GetSystemHandle() {
	// Not used on mobile
	return nullptr;
}

void Reflex::System::Android::Window::ExportBitmap(TRef<ObjectOf<RawBitmap>> buffer) const {
	// TODO: Florian --
	DEV_WARN("Window::ExportBitmap unsupported");
}

Reflex::TRef<Reflex::ObjectOf<Reflex::System::RawBitmap>> Reflex::System::Android::Window::CreateExportBitmapBuffer(UInt8 flags) const
{
	return REFLEX_CREATE(ObjectOf<System::RawBitmap>);
}

Reflex::TRef <Reflex::System::Window> Reflex::System::Window::Create(UInt32 styleflags, bool topmost, void * systemparent)
{
	if (Android::Window::st_self)
	{
		DEV_WARN("Only one window supported on mobile");
		return System::Window::null;
	}

	return REFLEX_CREATE(Android::Window, styleflags, topmost, systemparent);
}
