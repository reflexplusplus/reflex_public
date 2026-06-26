#pragma once

#include "d3d.h"




//
//declarations

REFLEX_NS(Reflex::System::Win)

class Window :
	public System::Window,
	public IDropTarget
{
public:

	REFLEX_OBJECT(System::Win::Window, System::Window);

	Window(HWND sysparent, UInt32 flags, bool ontop, bool isplugin);

	virtual ~Window();



protected:

	friend PluginWindow;	//for vst hooks


	template <bool FRAME, bool MINIMISEABLE, bool RESIZEABLE> static void ApplyStyleFlags(Window & self);

	REFLEX_TBINDER_3P(ApplyStyleFlags);


	void SetClient(TRef <Client> client) override { m_client = client; client->OnSetOwner(this); }

	TRef <Client> GetClient() override { return m_client; }


	void Attach(System::Window & parent) override;

	void Detach() override;


	virtual void SetStyleFlags(UInt32 flags) /*not override*/;

	void SetTitle(const WString & label) override;


	void SetRect(const iRect & rect) override;

	void SetDisplayMode(WindowDisplay state) override;

	void SendTop() override;


	void EnableInput(bool enable) override;

	void SetMouseCursor(MouseCursor mousecursor) override;

	void SetMousePosition(const iPoint & position) override;


	void BeginDragDropFiles(const ArrayView <WString> & filenames) override;


	TRef < ObjectOf <RawBitmap> > CreateExportBitmapBuffer(UInt8 flags) const override;

	void ExportBitmap(TRef < ObjectOf <RawBitmap> > buffer) const override;


	void * GetSystemHandle() override { return m_hwnd; }


	void OnSetProperty(Address address, Object & object) override;

	void OnQueryProperty(Address address, Object * & pobject) const override
	{
		if (address == MakeAddress<ObjectOf<bool>>(K32("resizable")))
		{
			pobject = &m_resizable;
		}
		else
		{
			Object::OnQueryProperty(address, pobject);
		}
	}


	void SetNativeWindowRect(const iRect & rect);

	iRect GetRect() const;

	void ProcessMouseMove(const iPoint & position);

	void ProcessMouseDown(bool rmb, bool dbl);

	void ProcessMouseUp(bool rmb);

	bool ProcessKeyDown(UInt VK);

	bool ProcessKeyUp(UInt VK);

	UIntNative DefaultMessageHandler(WPARAM wparam, LPARAM lparam) { return DefWindowProc(m_hwnd, st_currentmsg, wparam, lparam); }

	static void UpdateModifierKeyFlags();


	static LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);


	virtual STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

	virtual STDMETHODIMP_(ULONG) AddRef();

	virtual STDMETHODIMP_(ULONG) Release();

	virtual HRESULT STDMETHODCALLTYPE DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

	virtual HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

	virtual HRESULT STDMETHODCALLTYPE DragLeave(void);

	virtual HRESULT STDMETHODCALLTYPE Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);


	static UIntNative WM_Default(Window & window, UIntNative, UIntNative);

	static UIntNative WM_NclButtonDown(Window & window, UIntNative, UIntNative);

	static UIntNative WM_MouseLeave(Window & window, UIntNative, UIntNative);

	template <bool MB> static UIntNative WM_MouseMove(Window & window, UIntNative, UIntNative);

	template <bool RMB, bool DBL> static UIntNative WM_MouseDown(Window & window, UIntNative, UIntNative);

	template <bool RMB> UIntNative static WM_MouseUp(Window & window, UIntNative, UIntNative);

	template <bool Y> static UIntNative WM_MouseWheel(Window & window, UIntNative, UIntNative);

	static UIntNative WM_KeyDown(Window & window, UIntNative vk, UIntNative lparam);

	static UIntNative WM_KeyUp(Window & window, UIntNative vk, UIntNative lparam);

	static UIntNative WM_Char(Window & window, UIntNative character, UIntNative lparam);

	static UIntNative WM_SetFocus(Window & window, UIntNative, UIntNative);

	static UIntNative WM_LoseFocus(Window & window, UIntNative, UIntNative);

	static UIntNative WM_GetMinMaxInfo(Window & window, UIntNative, UIntNative);

	static UIntNative WM_WindowPosChanged(Window & window, UIntNative, UIntNative);

	static UIntNative WM_Sizing(Window & window, UIntNative, UIntNative);

	static UIntNative WM_ExitSizing(Window & window, UIntNative, UIntNative);

	static UIntNative WM_Paint(Window & window, UIntNative, UIntNative);

	static UIntNative WM_EraseBackground(Window & window, UIntNative, UIntNative);

	static UIntNative WM_Close(Window & window, UIntNative, UIntNative);

	static UIntNative WM_IME_Start(Window & window, UIntNative, UIntNative);

	static UIntNative WM_IME_End(Window & window, UIntNative, UIntNative);

	static UIntNative WM_IME_Compose(Window & window, UIntNative, UIntNative);

	static UIntNative WM_Destroy(Window & window, UIntNative, UIntNative);



	Reference <Client> m_client;

	
	const bool m_isplugin;

	UInt8 m_styleflags;

	bool m_topmost;

	WindowDisplay m_state;

	mutable ObjectOf <bool> m_resizable;


	iRect m_xywh;

	MouseCursor m_mousecursor;


	UInt8 m_mousebutton;

	bool m_keystate[256];


	HWND m_hwnd;

	DWORD m_current_style, m_current_stylex;


	LPARAM m_mouseparam;

	POINT m_mousepoint;

	Int32 m_xinit, m_yinit;

	TRACKMOUSEEVENT m_trackmouseevent;

	AtomicUInt32 m_refcount;


	FunctionPointer <UIntNative(Window&,UIntNative,UIntNative)> m_msgfn[Win::kNumWindowMsg];


	public:
		IDXGISwapChain * m_swapchainhack;
	private:


	static UInt32 st_currentmsg;

	static const WindowDisplay kWindowStates[7];

};

REFLEX_END