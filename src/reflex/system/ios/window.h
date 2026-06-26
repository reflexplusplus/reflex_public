#pragma once

#include "sdk.h"
#import "window/ReflexViewController.h"




//
//declarations

REFLEX_NS(Reflex::System::iOS)

struct Touch;

struct Window;

REFLEX_END




//
//Touch

struct Reflex::System::iOS::Touch {
	CGPoint pos;
	UIKeyModifierFlags modifierFlags;
};




//
//Window

struct Reflex::System::iOS::Window : public Reflex::System::Window {
	static inline Window* st_self = nullptr;
	static inline ReflexViewController* st_viewcontroller = nil;

	Window(UInt32 styleFlags, bool ontop, void *systemParent);
	~Window() override;

	TRef<Client> GetClient() override { return m_client; }

	static UIViewController* CreateViewController();

	void UpdateViewSize();

	void ProcessPendingTasks();
	// Returns whether the key has been trapped
	bool Internal_ProcessKeyDown(UIPressesEvent* event);
	void Internal_ProcessKeyUp(UIPressesEvent* event);

	void Internal_HandleImeInput(UInt chars_to_erase, const WString& chars_to_insert);


private:

	struct Globals {
		Globals();

		Pair<KeyCode,bool> TranslateKey(unichar keycode);
		void UpdateModifierFlags(Client& client, UIKeyModifierFlags eventFlags);

		// Keyboard-related
		bool keystate[kNumKeyCode];
		bool keytrap[kNumKeyCode];
		Sequence<WChar, Pair<KeyCode,bool>> unichar2keycodes;
		UInt8 modifierkeys = 0;
	};

	// MEMO: OnMouseEnter and OnMouseLeave not supported on iOS (we have only one window)
	void SetClient(TRef<Client> client) override;

	void Attach(System::Window& parent) override;
	void Detach() override;

	void SetTitle(const WString& label) override;
	void SetRect(const iRect& rect) override;
	void SetDisplayMode(WindowDisplay state) override;
	void SendTop() override;

	void EnableInput(bool enable) override;

	void SetMouseCursor(MouseCursor mousecursor) override;
	void SetMousePosition(const iPoint& position) override;

	void BeginDragDropFiles(const ArrayView <WString> & filenames) override;

	TRef<ObjectOf<RawBitmap>> CreateExportBitmapBuffer(UInt8 flags) const override;
	void ExportBitmap(TRef<ObjectOf<RawBitmap>> buffer) const override;

	void* GetSystemHandle() override;

	bool HandleDoubleClickDown(double currentX, double currentY);
	void HandleDoubleClickUp(double currentX, double currentY);
	void SignalMoveIfNeeded(double currentX, double currentY);
	void PostScroll(fPoint delta);


	Reference<Client> m_client;

	iPoint m_lastSignaledMousePos;
	bool m_isFingerDown;
	bool m_keyboardshown = false;

	static Reflex::Detail::Initialiser<Globals> st_globals;

	static constexpr Float32 kDampeningFactorWhenHeld = 200, kDampeningFactorWhenReleased = 5;
};
