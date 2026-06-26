#include "window.h"




// MARK: Window::Globals
Reflex::System::iOS::Window::Globals::Globals() {
	MemClear(keystate, kNumKeyCode * sizeof(bool));
	MemClear(keytrap, kNumKeyCode * sizeof(bool));

	auto Register = [](Sequence < WChar, Pair<KeyCode,bool> > & keycodes, WChar w, KeyCode keycode, bool printable) {
		keycodes.Insert(w, {keycode, printable});
	};
	//printable
	Register(unichar2keycodes, '0', kKeyCode0, true);
	UInt32 keycode = kKeyCode1;
	for (WChar idx = '1'; idx <= '9'; ++idx) Register(unichar2keycodes, idx, KeyCode(keycode++), true);
	keycode = kKeyCodeA;
	for (WChar idx = 'a'; idx <= 'z'; ++idx) Register(unichar2keycodes, idx, KeyCode(keycode++), true);
	keycode = kKeyCodeA;
	for (WChar idx = 'A'; idx <= 'Z'; ++idx) Register(unichar2keycodes, idx, KeyCode(keycode++), true);
	Register(unichar2keycodes, ' ', kKeyCodeSpace, true);
	Register(unichar2keycodes, '+', kKeyCodeNumericPlus, true);
	Register(unichar2keycodes, '-', kKeyCodeNumericMinus, true);
	Register(unichar2keycodes, '/', kKeyCodeNumericDivide, true);
	Register(unichar2keycodes, '*', kKeyCodeNumericMultiply, true);
	Register(unichar2keycodes, '=', kKeyCodePlus, true);
	Register(unichar2keycodes, '[', kKeyCodeBracketOpen, true);
	Register(unichar2keycodes, ']', kKeyCodeBracketClose, true);
	Register(unichar2keycodes, '{', kKeyCodeBracketOpen, true);
	Register(unichar2keycodes, '}', kKeyCodeBracketClose, true);
	//non printable
	REFLEX_LOOP_TYPE (WChar, idx, 32) Register(unichar2keycodes, idx, kKeyCodeNull, false);
	unichar2keycodes.Acquire(3) = {kKeyCodeEnter, false};	//numeric enter
	unichar2keycodes.Acquire(9) = {kKeyCodeTab, false};
	unichar2keycodes.Acquire(25) = {kKeyCodeTab, false};//shift+tab
	unichar2keycodes.Acquire(13) = {kKeyCodeEnter, false};
	unichar2keycodes.Acquire(27) = {kKeyCodeEscape, false};
	Register(unichar2keycodes, 127, kKeyCodeBackspace, false);
	REFLEX_LOOP (idx, 12) Register(unichar2keycodes, 63236 + idx, KeyCode(kKeyCodeF1 + idx), false);
	Register(unichar2keycodes, 63232, kKeyCodeUp, false);
	Register(unichar2keycodes, 63233, kKeyCodeDown, false);
	Register(unichar2keycodes, 63234, kKeyCodeLeft, false);
	Register(unichar2keycodes, 63235, kKeyCodeRight, false);
	Register(unichar2keycodes, 63273, kKeyCodeHome, false);
	Register(unichar2keycodes, 63275, kKeyCodeEnd, false);
	Register(unichar2keycodes, 63276, kKeyCodePageUp, false);
	Register(unichar2keycodes, 63277, kKeyCodePageDown, false);
	Register(unichar2keycodes, 63302, kKeyCodeInsert, false);
	Register(unichar2keycodes, 63272, kKeyCodeDelete, false);
	Register(unichar2keycodes, 63248, kKeyCodeNull, false);//print screen
	Register(unichar2keycodes, 63249, kKeyCodeNull, false);//num lock
}

Reflex::Pair<Reflex::System::KeyCode,bool> Reflex::System::iOS::Window::Globals::TranslateKey(unichar code) {
	Pair <KeyCode,bool> null = {kKeyCodeNull, true};
	return *unichar2keycodes.SearchValue(code, &null);
}

void Reflex::System::iOS::Window::Globals::UpdateModifierFlags(Client& client, UIKeyModifierFlags eventFlags) {
	UInt32 flags = (eventFlags >> 17) & 0xF;
	auto current = modifierkeys;

	if (SetFiltered(modifierkeys, UInt8(flags))) {
		// st_modifierkeys = flags;

		REFLEX_LOOP(idx, 4) {
			if (BitCheck(flags, idx) > BitCheck(current, idx)) {
				client.OnKeyPress(kKeyCodeNull, false);
			}
			else if (BitCheck(flags, idx) < BitCheck(current, idx)) {
				client.OnKeyRelease(kKeyCodeNull);
			}
		}
	}
}

// MARK: Window
Reflex::System::iOS::Window::Window(UInt32 styleFlags, bool ontop, void* systemparent)
	: m_lastSignaledMousePos({ -1, -1 })
	, m_isFingerDown(false)
{
	st_self = this;

	// When we support multiple windows, we'll need to do that only the first time
	st_globals.Init();
}

Reflex::System::iOS::Window::~Window() {
	m_client->OnSetOwner(nullptr);
	st_globals.Deinit();
	st_self = nullptr;
}

void Reflex::System::iOS::Window::Attach(System::Window& parent) {
	DEV_WARN("Unsupported Window::Attach()");
}

void Reflex::System::iOS::Window::Detach() {
	DEV_WARN("Unsupported Window::Detach()");
}

void Reflex::System::iOS::Window::UpdateViewSize() {
	auto screenSize = UIScreen.mainScreen.bounds.size;
	iRect wholeScreen = { {0, 0}, {int(screenSize.width), int(screenSize.height)} };
	auto insets = st_viewcontroller.viewInsets;

	iRect interactable = {
		{ int(insets.left), int(insets.top) },
		{ wholeScreen.size.w - int(insets.right), wholeScreen.size.h - int(insets.bottom) }
	};

	m_client->OnSetRect(kWindowDisplayFullScreen, wholeScreen, interactable, GetMaxPixelDensity());
}

void Reflex::System::iOS::Window::ProcessPendingTasks() {
	if (!m_client) return;
}

void Reflex::System::iOS::Window::SetClient(TRef <Client> client) {
	m_client = client;

	m_client->OnSetOwner(this);

	UpdateViewSize();
}

void Reflex::System::iOS::Window::SetTitle(const WString& label) {
}

void Reflex::System::iOS::Window::SetRect(const iRect& rect) {
}

void Reflex::System::iOS::Window::SetDisplayMode(WindowDisplay state) {
}

void Reflex::System::iOS::Window::SendTop() {
}

void Reflex::System::iOS::Window::EnableInput(bool enable) {
}

void Reflex::System::iOS::Window::SetMouseCursor(MouseCursor mousecursor) {
}

void Reflex::System::iOS::Window::SetMousePosition(const iPoint& position) {
}

void Reflex::System::iOS::Window::BeginDragDropFiles(const ArrayView <WString> & filenames) {
}

void* Reflex::System::iOS::Window::GetSystemHandle() {
	return nullptr;
}

void Reflex::System::iOS::Window::ExportBitmap(TRef<ObjectOf<RawBitmap>> buffer) const {
	DEV_WARN("Window::ExportBitmap unsupported");
}

Reflex::TRef<Reflex::ObjectOf<Reflex::System::RawBitmap>> Reflex::System::iOS::Window::CreateExportBitmapBuffer(UInt8 flags) const {
	return REFLEX_CREATE(ObjectOf<System::RawBitmap>, );
}

void Reflex::System::iOS::Window::Internal_HandleImeInput(UInt chars_to_erase, const WString& chars_to_insert) 
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
			// TODO: map like on iOS
			m_client->OnKeyPress(kKeyCodeEnter, false);
			m_client->OnKeyRelease(kKeyCodeEnter);
		}
		else if (auto trap = m_client->OnKeyPress(kNumKeyCode, false)) 
		{
			m_client->OnCharacter(c);
		}
	}
}

bool Reflex::System::iOS::Window::Internal_ProcessKeyDown(UIPressesEvent *event) {
	st_globals->UpdateModifierFlags(m_client, [event modifierFlags]);

	for (UIPress* press in event.allPresses) {
		NSString *keycodes = [[press key] charactersIgnoringModifiers];

		if ([keycodes length]) {
			auto keyinfo = st_globals->TranslateKey([keycodes characterAtIndex:0]);
			auto keycode = keyinfo.a;
			bool &trap = st_globals->keytrap[keycode];
			bool standalone = kEnvironmentType != kEnvironmentTypeAudioPlugin;
			bool cmd = True(st_globals->modifierkeys & kModifierKeySystem);
			st_globals->keystate[keycode] = true;
			// MEMO: No way to detect whether it's a repeat
			constexpr bool isARepeat = false;
			if ((trap = m_client->OnKeyPress(keycode, isARepeat))) {
				NSString* characters = [[press key] characters];
				if (characters.length > 0 && keyinfo.b && !cmd) {
					m_client->OnCharacter([characters characterAtIndex:0]);
				}
			}

			// MEMO: we are ignoring the following characters in the event in that case
			trap = trap || (standalone && !cmd);
			return trap;
		}
	}

	return false;
}

void Reflex::System::iOS::Window::Internal_ProcessKeyUp(UIPressesEvent *event) {
	st_globals->UpdateModifierFlags(m_client, [event modifierFlags]);

	for (UIPress* press in event.allPresses) {
		NSString *characters = [[press key] characters];

		if ([characters length]) {
			auto keyinfo = st_globals->TranslateKey([characters characterAtIndex:0]);
			auto keycode = keyinfo.a;

			if (st_globals->keystate[keycode]) {
				st_globals->keystate[keycode] = false;
				m_client->OnKeyRelease(keycode);
			}

			st_globals->keytrap[keycode] = false;
		}
	}
}

void Reflex::System::iOS::Window::SignalMoveIfNeeded(double currentX, double currentY) {
	iPoint pos = { int32_t(currentX), int32_t(currentY) };
	if (pos.x != m_lastSignaledMousePos.x || pos.y != m_lastSignaledMousePos.y) {
		m_lastSignaledMousePos = pos;
		m_client->OnMouseMove(pos);
	}
}

Reflex::Detail::Initialiser<Reflex::System::iOS::Window::Globals> Reflex::System::iOS::Window::st_globals;

Reflex::TRef<Reflex::System::Window> Reflex::System::Window::Create(UInt32 styleflags, bool topmost, void* systemparent)
{
	if (iOS::Window::st_self) {
		DEV_WARN("Only one Window supported on iOS");
		return {};
	}
	else {
		return REFLEX_CREATE(iOS::Window, styleflags, topmost, systemparent);
	}
}

UIViewController* Reflex::System::iOS::Window::CreateViewController()
{
	return [[ReflexViewController alloc] init];
}
