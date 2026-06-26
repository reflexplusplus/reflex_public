#pragma once

#include "library.h"

REFLEX_NS(Reflex::System::Android)

struct Window : public System::Window {
	static inline Window * st_self = nullptr;

	Window(UInt32 styleFlags, bool ontop, void *systemParent);
	~Window() override;

	// Internal use
    int HandleNextInputEvent(AInputEvent* event);
	void HandleImeInput(UInt chars_to_erase, const WString& chars_to_insert);

	void ScheduleWindowRectChanged();
	void ProcessPendingTasks();
	void UpdateWindowOnCreation();

	void OnSetFocus();
	void OnLoseFocus();


protected:

	void SetClient(TRef <Client> client) override;

	TRef <Client> GetClient() override { return m_client; }

	void Attach(System::Window & parent) override;

	void Detach() override;


	void SetTitle(const WString & label) override;


	void SetRect(const iRect & rect) override;

	void SetDisplayMode(WindowDisplay state) override;

	void SendTop() override;


	void EnableInput(bool enable) override;

	void SetMouseCursor(MouseCursor mousecursor) override;

	void SetMousePosition(const iPoint & position) override;


	void BeginDragDropFiles(const ArrayView <WString> & filenames) override;

	TRef<ObjectOf<RawBitmap>> CreateExportBitmapBuffer(UInt8 flags) const override;

	void ExportBitmap(TRef<ObjectOf<RawBitmap>> buffer) const override;

	void* GetSystemHandle() override;

	Reference <Client> m_client;


private:
	int ShouldConsume(int keycode);
	void SignalMoveIfNeeded(fPoint newPosition);

	bool m_needSizeUpdate = true;
	iPoint m_lastSignaledMousePos;
	Float64 m_lastclicktime = 0;
	bool m_keyboardshown = false;
};

REFLEX_END
