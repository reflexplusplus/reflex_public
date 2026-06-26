#include "window.h"

#pragma comment(lib, "Imm32.lib")




//
//window

#define CLEAR(flags, bits) flags &= ~bits
#define SET(flags, bits, tf) if (tf) {flags |= bits;} else { CLEAR(flags, bits); }
#define SET_CONST(flags, bits, tf) if constexpr (tf) {flags |= bits;} else { CLEAR(flags, bits); }

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct DragFile
{
	const WChar * name;
	const void * data;
	UInt32 size;
};

struct ExportBitmapBuffer : public ObjectOf<RawBitmap>
{
	~ExportBitmapBuffer()
	{
		DeleteObject(hbitmap);

		DeleteDC(hdc_mem);

		ReleaseDC(hwnd, hdc_screen);
	}

	HWND hwnd;

	bool redraw;

	HDC hdc_screen, hdc_mem;

	HBITMAP hbitmap;

	BITMAP bitmap;

	BITMAPINFOHEADER bitmapinfoheader;

	iSize size;
};

template <class INTERFACE>
struct ComObject : public INTERFACE
{
	REFLEX_NONCOPYABLE(ComObject);

	ComObject(const IID & iid)
		: m_iid({ iid, iid })
		, m_retaincount(1)
	{
	}

	ComObject(const IID & a, const IID & b)
		: m_iid({ a, b })
		, m_retaincount(1)
	{
	}

	virtual ~ComObject()
	{
	}

	virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObject);

	virtual STDMETHODIMP_(ULONG) AddRef();

	virtual STDMETHODIMP_(ULONG) Release();

	Pair <IID> m_iid;

	UInt32 m_retaincount;
};

struct DropSource : public ComObject <IDropSource>
{
	static IDataObject * CreateFileDataObject(const WChar * path);

	DropSource() : ComObject(IID_IDropSource) {}

	virtual STDMETHODIMP QueryContinueDrag(BOOL escapepressed, DWORD keystate);

	virtual STDMETHODIMP GiveFeedback(DWORD effect) { return DRAGDROP_S_USEDEFAULTCURSORS; }
};

template <bool>
struct DragFilesObject : public ComObject <IDataObject>
{
	DragFilesObject(const ArrayView <WString> & filenames);

	DragFilesObject(const DragFile * files, UInt32 n);

	virtual HRESULT STDMETHODCALLTYPE QueryGetData(__RPC__in_opt FORMATETC * pformatetc);

	virtual HRESULT STDMETHODCALLTYPE GetData(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);

	virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, __RPC__deref_out_opt IEnumFORMATETC ** ppenumFormatEtc);

	virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(__RPC__in_opt FORMATETC * pformatectIn, __RPC__out FORMATETC * pformatetcOut);

	static bool CheckFormat(const FORMATETC & a, const FORMATETC & b);

	static HRESULT CreateHGlobal(const void * data, UInt32 size, STGMEDIUM & pmedium);


	virtual HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC * pformatetc, STGMEDIUM * pmedium) { return E_NOTIMPL; }

	virtual HRESULT STDMETHODCALLTYPE SetData(FORMATETC * pformatetc_in, STGMEDIUM * pmedium_in, BOOL fRelease) { return E_NOTIMPL; }

	virtual HRESULT STDMETHODCALLTYPE DAdvise(__RPC__in FORMATETC * pformatetc, DWORD advf, __RPC__in_opt IAdviseSink * pAdvSink, __RPC__out DWORD * pdwConnection) { return OLE_E_ADVISENOTSUPPORTED; }

	virtual HRESULT STDMETHODCALLTYPE DUnadvise(DWORD dwConnection) { return OLE_E_ADVISENOTSUPPORTED; }

	virtual HRESULT STDMETHODCALLTYPE EnumDAdvise(__RPC__deref_out_opt IEnumSTATDATA ** ppenumAdvise) { return OLE_E_ADVISENOTSUPPORTED; }


	Array <DragFile> m_files;

	FORMATETC m_formats[2];
};

inline RECT MakeRect(Int32 x, Int32 y, Int32 w, Int32 h)
{
	RECT rect;

	rect.left = x;
	rect.right = x + w;
	rect.top = y;
	rect.bottom = y + h;

	return rect;
}

LRESULT CALLBACK NullWinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

REFLEX_END_INTERNAL

Reflex::UInt32 Reflex::System::Win::Window::st_currentmsg = 0;

const Reflex::System::WindowDisplay Reflex::System::Win::Window::kWindowStates[7] =
{
	kWindowDisplayMinimised,
	kWindowDisplayWindowed,
	kWindowDisplayMinimised,
	kWindowDisplayMinimised,
	kWindowDisplayMinimised,
	kWindowDisplayMinimised,
	kWindowDisplayMinimised,
};

Reflex::System::Win::Window::Window(HWND sysparent, UInt32 styleflags, bool ontop, bool isplugin)
	: m_client(Client::null)
	, m_refcount(0)

	, m_isplugin(isplugin)
	, m_styleflags(UInt8(styleflags))
	, m_topmost(ontop)

	, m_state(kWindowDisplayMinimised)
	, m_xywh({ {0,0}, {0,0} })

	, m_current_style(WS_CLIPSIBLINGS | WS_CLIPCHILDREN | (sysparent ? WS_CHILD : 0))
	, m_current_stylex(ontop ? WS_EX_TOPMOST : 0)

	, m_mousecursor(kMouseCursorArrow)
	, m_mousebutton(0)

	, m_swapchainhack(&D3dNullSwapChain::self)
{
	MemClear(m_keystate, sizeof(bool) * 256);


	m_hwnd = CreateWindowExW(m_current_stylex, Library::kVisibleWindowClass.data, L"", m_current_style, 0, 0, 0, 0, sysparent, 0, Library::st_hinstance, 0);

	if (!sysparent) ApplyStyleFlagsBinder::Bind(m_styleflags)(*this);


	m_trackmouseevent.cbSize = sizeof(TRACKMOUSEEVENT);

	m_trackmouseevent.dwFlags = TME_LEAVE;

	m_trackmouseevent.hwndTrack = m_hwnd;

	m_trackmouseevent.dwHoverTime = 0;

	m_mousepoint.x = 0;

	m_mousepoint.y = 0;

	m_msgfn[kDEFAULT] = &Window::WM_Default;
	m_msgfn[kNCLBUTTONDOWN] = &Window::WM_NclButtonDown;
	m_msgfn[kMOUSELEAVE] = &Window::WM_MouseLeave;
	m_msgfn[kMOUSEMOVE] = &Window::WM_MouseMove<false>;
	m_msgfn[kLBUTTONDOWN] = &Window::WM_MouseDown<false, false>;
	m_msgfn[kRBUTTONDOWN] = &Window::WM_MouseDown<true, false>;
	m_msgfn[kLBUTTONDBLCLK] = &Window::WM_MouseDown<false, true>;
	m_msgfn[kRBUTTONDBLCLK] = &Window::WM_MouseDown<true, true>;
	m_msgfn[kLBUTTONUP] = &Window::WM_MouseUp<false>;
	m_msgfn[kRBUTTONUP] = &Window::WM_MouseUp<true>;
	m_msgfn[kMOUSEWHEEL_X] = &Window::WM_MouseWheel<false>;
	m_msgfn[kMOUSEWHEEL_Y] = &Window::WM_MouseWheel<true>;

	m_msgfn[kSYSKEYDOWN] = &Window::WM_KeyDown;
	m_msgfn[kSYSKEYUP] = &Window::WM_KeyUp;
	m_msgfn[kKEYDOWN] = &Window::WM_KeyDown;
	m_msgfn[kKEYUP] = &Window::WM_KeyUp;
	m_msgfn[kCHAR] = &Window::WM_Char;

	m_msgfn[kSETFOCUS] = &Window::WM_SetFocus;
	m_msgfn[kKILLFOCUS] = &Window::WM_LoseFocus;
	m_msgfn[kGETMINMAXINFO] = &Window::WM_GetMinMaxInfo;
	//m_msgfn[kWINDOWPOSCHANGING] = &Window::WM_WindowPosChanging;
	m_msgfn[kWINDOWPOSCHANGED] = &Window::WM_WindowPosChanged;
	m_msgfn[kSIZING] = &Window::WM_Sizing;
	m_msgfn[kEXITSIZEMOVE] = &Window::WM_ExitSizing;
	m_msgfn[kPAINT] = &Window::WM_Paint;
	m_msgfn[kERASEBKGND] = &Window::WM_EraseBackground;
	m_msgfn[kCLOSE] = &Window::WM_Close;

	m_msgfn[kIME_STARTCOMPOSITION] = &Window::WM_IME_Start;
	m_msgfn[kIME_ENDCOMPOSITION] = &Window::WM_IME_End;
	m_msgfn[kIME_COMPOSITION] = &Window::WM_IME_Compose;
	m_msgfn[kIME_KEYLAST] = &Window::WM_IME_Compose;

	m_msgfn[kDESTROY] = &Window::WM_Destroy;

	m_msgfn[kSYSCOMMAND] = [](Window & self, UIntNative wparam, UIntNative lparam) -> UIntNative
	{
		if ((wparam & 0xFFF0) == SC_KEYMENU)
		{
			return 0;
		}
		else
		{
			return self.DefaultMessageHandler(wparam, lparam);
		}
	};

	RegisterDragDrop(m_hwnd, this);


	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, LONG_PTR(this));

	SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, LONG_PTR(&Window::WinProc));
}

Reflex::System::Win::Window::~Window()
{
	REFLEX_LOCAL(BOOL STDCALL, EnumProc)(HWND hwnd, LPARAM param)
	{
		//incase external (vst) windows are hosting subviews

		if (GetWindowLongPtr(hwnd, GWLP_WNDPROC) == LONG_PTR(&Window::WinProc))	//fixed Waldorf Nave
		{
			SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, 0);
		}

		return TRUE;
	}
	REFLEX_END

	m_client->OnSetOwner(nullptr);

	m_client.Clear();

	SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, LONG_PTR(&NullWinProc));

	SetWindowLongPtr(m_hwnd, GWLP_HWNDPARENT, 0);

	EnumChildWindows(m_hwnd, &EnumProc::Call, 0);

	auto & mouseover = globals->m_mouseover;

	if (mouseover == this) mouseover = 0;

	DestroyWindow(m_hwnd);
}

template <bool FRAME, bool MINABLE, bool RESIZEABLE> void Reflex::System::Win::Window::ApplyStyleFlags(Window & self)
{
	self.m_resizable.value = RESIZEABLE;

	auto hwnd = self.m_hwnd;

	auto style = DWORD(GetWindowLongPtr(hwnd, GWL_STYLE));

	auto stylex = DWORD(GetWindowLongPtr(hwnd, GWL_EXSTYLE));

	switch (self.m_state)
	{
	case kWindowDisplayMinimised:
	case kWindowDisplayWindowed:
	{
		SET_CONST(style, WS_BORDER | WS_DLGFRAME | WS_SYSMENU, FRAME | MINABLE | RESIZEABLE);

		SET_CONST(style, WS_MINIMIZEBOX, MINABLE);

		SET_CONST(style, WS_THICKFRAME, RESIZEABLE);

		SET_CONST(stylex, WS_EX_TOOLWINDOW, !MINABLE);

		CLEAR(style, WS_POPUP);

		SET(stylex, WS_EX_TOPMOST, self.m_topmost);
	}
	break;

	case kWindowDisplayFullScreen:
	{
		CLEAR(style, WS_BORDER | WS_DLGFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME);

		CLEAR(stylex, WS_EX_TOOLWINDOW);

		auto fs = globals->m_enable_truefullscreen;

		SET(style, WS_POPUP, fs);	//WS_POPUP would be better but it doesnt allow overlapwindows

		SET(style, WS_BORDER, !fs);

		SET_CONST(stylex, WS_EX_TOPMOST, true);
	}
	break;
	}

	SetWindowLongPtr(hwnd, GWL_STYLE, style);

	SetWindowLongPtr(hwnd, GWL_EXSTYLE, stylex);

	self.m_current_style = style;

	self.m_current_stylex = stylex;
}

void Reflex::System::Win::Window::Attach(System::Window & parent)
{
	if (auto pparent = DynamicCast<Window>(parent))
	{
		SetWindowLongPtr(m_hwnd, GWLP_HWNDPARENT, (UIntNative)pparent->m_hwnd);

		// !!! WORKAROUND !!!: windows sends erroneous WM_WINDOWPOSCHANGED to parent on attaching (despite SWP_NOSIZE flag), this caused recursive crash in client code

		auto & parent_poschangedcallback = pparent->m_msgfn[kWINDOWPOSCHANGED];

		parent_poschangedcallback = &Window::WM_ExitSizing;

		SetWindowPos(m_hwnd, pparent->m_hwnd, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

		parent_poschangedcallback = &Window::WM_WindowPosChanged;
	}
}

void Reflex::System::Win::Window::Detach()
{
	SetWindowLongPtr(m_hwnd, GWLP_HWNDPARENT, 0);

	SetWindowPos(m_hwnd, 0, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
}

void Reflex::System::Win::Window::SetStyleFlags(UInt32 flags)
{
	m_styleflags = UInt8(flags);

	ApplyStyleFlagsBinder::Bind(m_styleflags)(*this);
}

void Reflex::System::Win::Window::SetTitle(const WString & label)
{
	SetWindowText(m_hwnd, label.GetData());
}

void Reflex::System::Win::Window::SetRect(const iRect & rect)
{
	//DEV_LOG(this, "Window::SetRect", rect.origin.x, rect.origin.y, rect.size.w, rect.size.h);

	m_xywh = rect;

	if (m_state == kWindowDisplayWindowed) SetNativeWindowRect(m_xywh);
}

void Reflex::System::Win::Window::SetDisplayMode(WindowDisplay state)
{
	m_state = state;

	ApplyStyleFlagsBinder::Bind(m_styleflags)(*this);

	switch (m_state)
	{
	case kWindowDisplayMinimised:
		ShowWindow(m_hwnd, SW_SHOWMINIMIZED);
		break;

	case kWindowDisplayWindowed:
	{
		SetNativeWindowRect(m_xywh);
		ShowWindow(m_hwnd, SW_SHOWNORMAL);
		SetForegroundWindow(m_hwnd);
	}
	break;

	case kWindowDisplayFullScreen:
	{
		HMONITOR hmon = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);

		MONITORINFO mi = { sizeof(MONITORINFO) };

		if (GetMonitorInfo(hmon, &mi))
		{
			iRect rect = { { 0, 0 }, { mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top } };

			auto density = globals->m_maxpixeldensity;

			rect.size.w /= density;

			rect.size.h /= density;

			SetNativeWindowRect(rect);
		}
		ShowWindow(m_hwnd, SW_SHOW);
		SetForegroundWindow(m_hwnd);
		break;
	}
	}
}

void Reflex::System::Win::Window::SendTop()
{
	REFLEX_LOCAL(BOOL STDCALL, EnumProc)(HWND hwnd, LPARAM param)
	{
		SetForegroundWindow(hwnd);

		return TRUE;
	}
	REFLEX_END

	SetForegroundWindow(m_hwnd);

	EnumChildWindows(m_hwnd, &EnumProc::Call, 0);
}

void Reflex::System::Win::Window::EnableInput(bool enable)
{
	EnableWindow(m_hwnd, BOOL(enable));
}

void Reflex::System::Win::Window::SetMouseCursor(MouseCursor mousecursor)
{
	m_mousecursor = mousecursor;

	if (globals->m_mouseover == this) SetCursor(globals->m_hcursor[mousecursor]);
}

void Reflex::System::Win::Window::SetMousePosition(const iPoint & position)
{
	auto density = globals->m_maxpixeldensity;

	POINT point = { position.x * density, position.y * density };

	ClientToScreen(m_hwnd, &point);

	SetCursorPos(point.x, point.y);
}

void Reflex::System::Win::Window::BeginDragDropFiles(const ArrayView <WString> & filenames)
{
	if (filenames)
	{
		WM_MouseUp<false>(*this, 0, 0);

		WM_MouseUp<true>(*this, 0, 0);

		IDataObject * idataobject = ::new DragFilesObject<false>(filenames);

		IDropSource * idropsource = ::new DropSource;

		DWORD deffect = 0;

		DoDragDrop(idataobject, idropsource, DROPEFFECT_COPY | DROPEFFECT_LINK, &deffect);

		idropsource->Release();

		idataobject->Release();
	}
}

Reflex::TRef < Reflex::ObjectOf <Reflex::System::RawBitmap> > Reflex::System::Win::Window::CreateExportBitmapBuffer(UInt8 flags) const
{
	auto density = globals->m_maxpixeldensity;

	auto rect = GetRect();


	auto & rtn = *REFLEX_CREATE(ExportBitmapBuffer);

	rtn.hwnd = m_hwnd;

	rtn.redraw = flags;

	rtn.hdc_screen = GetDC(m_hwnd);

	rtn.hdc_mem = CreateCompatibleDC(rtn.hdc_screen);

	rtn.size = rect.size;

	rtn.size.w *= density;
	rtn.size.h *= density;

	rtn.hbitmap = CreateCompatibleBitmap(rtn.hdc_screen, rtn.size.w, rtn.size.h);

	SelectObject(rtn.hdc_mem, rtn.hbitmap);


	GetObjectW(rtn.hbitmap, sizeof(BITMAP), &rtn.bitmap);


	auto & bitmapinfoheader = rtn.bitmapinfoheader;

	MemClear(&bitmapinfoheader, sizeof(BITMAPINFOHEADER));

	bitmapinfoheader.biSize = sizeof(BITMAPINFOHEADER);
	bitmapinfoheader.biWidth = rtn.bitmap.bmWidth;
	bitmapinfoheader.biHeight = rtn.bitmap.bmHeight;
	bitmapinfoheader.biPlanes = 1;
	bitmapinfoheader.biBitCount = 24;
	bitmapinfoheader.biCompression = BI_RGB;


	auto & info = rtn.value.a;

	info.pixdensity = density;

	info.size = rect.size;

	info.format = System::kImageFormatRGB;


	auto & data = rtn.value.b;

	auto bytes = rtn.size.w * rtn.size.h * 3;

	data.Allocate(bytes + 4);

	data.SetSize(bytes);


	return rtn;
}

void Reflex::System::Win::Window::ExportBitmap(TRef < ObjectOf <RawBitmap> > buffer) const
{
	auto impl = Cast<ExportBitmapBuffer>(buffer);

	//force window to draw

	if (impl->redraw)
	{
		UpdateWindow(m_hwnd);

		InvalidateRect(m_hwnd, 0, TRUE);

		UpdateWindow(m_hwnd);

		RedrawWindow(m_hwnd, 0, 0, RDW_ERASE | RDW_INTERNALPAINT | RDW_INVALIDATE);	//RDW_ERASE | RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_INVALIDATE
	}

	BitBlt(impl->hdc_mem, 0, 0, impl->size.w, impl->size.h, impl->hdc_screen, 0, 0, SRCCOPY);



	//export the hbitmap

	auto ExportHBITMAP = [](HDC hdc_src, HBITMAP hbitmap, ExportBitmapBuffer & buffer)
	{
		auto & bitmapinfo = buffer.bitmapinfoheader;

		UInt8 * dest = buffer.value.b.GetData();

		UInt32 rowsize = bitmapinfo.biWidth * 3;

		REFLEX_RLOOP(idx, bitmapinfo.biHeight)
		{
			GetDIBits(hdc_src, hbitmap, idx, 1, dest, (BITMAPINFO *)&bitmapinfo, DIB_RGB_COLORS);

			auto prow = dest;

			REFLEX_LOOP(x, bitmapinfo.biWidth)
			{
				Swap(prow[0], prow[2]);

				prow += 3;
			}

			dest += rowsize;
		}
	};

	ExportHBITMAP(impl->hdc_screen, impl->hbitmap, impl);
}

void Reflex::System::Win::Window::OnSetProperty(Address address, Object & object)
{
	//if (auto call = CastCall<void, UInt32>(address, pvalue, K32("SetStyleFlags")))
	//{
	//	SetStyleFlags(call->a);

	//	return true;
	//}
	if (address == MakeAddress<ObjectOf<bool>>(K32("MouseClick_experimental")))
	{
		INPUT input[3];

		MemClear(input, sizeof(INPUT) * 3);

		input[0].type = INPUT_MOUSE;
		input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		SendInput(1, input + 0, sizeof(INPUT));

		input[1].type = INPUT_MOUSE;
		input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		System::SuspendThread(50);

		SendInput(1, input + 1, sizeof(INPUT));
	}

	Object::OnSetProperty(address, object);
}

LRESULT CALLBACK Reflex::System::Win::Window::Window::WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg < sizeof(Library::kMsgMap))
	{
		auto & implementation = *reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

		UInt8 fnidx = Library::kMsgMap[msg];

		st_currentmsg = msg;

		return (*implementation.m_msgfn[fnidx])(implementation, wparam, lparam);
	}
	else
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

STDMETHODIMP Reflex::System::Win::Window::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IUnknown || riid == IID_IDropTarget)
	{
		*ppv = static_cast<IUnknown*>(this);

		AddRef();

		return S_OK;
	}

	*ppv = NULL;

	return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) Reflex::System::Win::Window::AddRef()
{
	REFLEX_ATOMIC_INC(m_refcount, 1);

	return m_refcount;
}

STDMETHODIMP_(ULONG) Reflex::System::Win::Window::Release()
{
	REFLEX_ATOMIC_DEC(m_refcount, 1);

	UInt32 refcount = m_refcount;

	if (!refcount) Object::OnDestruct();

	return refcount;
}

HRESULT Reflex::System::Win::Window::DragEnter(IDataObject * idataobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	return S_OK;
}

HRESULT Reflex::System::Win::Window::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	return S_OK;
}

HRESULT Reflex::System::Win::Window::DragLeave(void)
{
	return S_OK;
}

HRESULT Reflex::System::Win::Window::Drop(IDataObject * idataobj, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	FORMATETC fmte = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

	STGMEDIUM stgm;

	constexpr CLIPFORMAT formats[] = { CF_HDROP, CF_TEXT };

	for (auto & i : formats)
	{
		fmte.cfFormat = i;

		if (SUCCEEDED(idataobj->GetData(&fmte, &stgm)))
		{
			POINT mousepos;

			GetCursorPos(&mousepos);

			ScreenToClient(m_hwnd, &mousepos);

			ProcessMouseMove(Reinterpret<iPoint>(mousepos));

			if (i == CF_TEXT)
			{
				ObjectOf < CString > text;	//bfr(nfile * MAX_PATH);

				HGLOBAL hmem = stgm.hGlobal;

				char * pmem = Reinterpret<char>(GlobalLock(hmem));

				UInt size = UInt(GlobalSize(hmem));

				if (size > 1)
				{
					auto & a = text.value;

					a.SetSize(size);

					MemCopy(pmem, a.GetData(), size);

					REFLEX_ASSERT(a.GetLast() == 0);

					m_client->OnDrop(text);
				}

				GlobalUnlock(hmem);
			}
			else
			{
				ObjectOf < Array <WString> > files;	//bfr(nfile * MAX_PATH);

				HDROP hdrop = reinterpret_cast<HDROP>(stgm.hGlobal);

				UINT nfile = DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);

				WChar buffer[MAX_PATH];

				WString t;

				files.value.Allocate(nfile);

				REFLEX_LOOP(idx, nfile)
				{
					DragQueryFile(hdrop, idx, buffer, MAX_PATH);

					WString path(buffer + 0);
					
					File::Detail::CorrectStrokes(path);
					
					files.value.Push<kAllocateNone>(std::move(path));
				}

				m_client->OnDrop(files);
			}

			ReleaseStgMedium(&stgm);
		}
	}

	return S_OK;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_Default(Window & self, UIntNative wparam, UIntNative lparam)
{
	return self.DefaultMessageHandler(wparam, lparam);
}

Reflex::UIntNative Reflex::System::Win::Window::WM_NclButtonDown(Window & self, UIntNative wparam, UIntNative lparam)
{
	//DEV_LOG(&self, "WM_NclButtonDown");

	self.m_client->OnSetFocus();

	return self.DefaultMessageHandler(wparam, lparam);
}

Reflex::UIntNative Reflex::System::Win::Window::WM_MouseLeave(Window & self, UIntNative, UIntNative)
{
	//DEV_LOG(&self, "Window::WM_MouseLeave");

	globals->m_mouseover = 0;

	self.m_mouseparam = 0;

	self.m_client->OnMouseLeave();

	return 1;
}

template <bool MB> Reflex::UIntNative Reflex::System::Win::Window::WM_MouseMove(Window & self, UIntNative wparam, UIntNative lparam)
{
	auto & client = *self.m_client;

	if constexpr (MB)
	{
		POINT point;

		GetCursorPos(&point);

		point.x -= self.m_xinit;

		point.y -= self.m_yinit;

		typedef Pair <Int32> Point;

		if (Reinterpret<Point>(point) != Reinterpret<Point>(self.m_mousepoint))
		{
			self.m_mousepoint = point;

			self.ProcessMouseMove(Reinterpret<iPoint>(point));
		}
	}
	else if (SetFiltered(self.m_mouseparam, Reinterpret<LPARAM>(lparam)))
	{
		if (SetFiltered(globals->m_mouseover, &self))
		{
			client.OnMouseEnter();

			self.SetMouseCursor(self.m_mousecursor);

			TrackMouseEvent(&self.m_trackmouseevent);
		}

		self.ProcessMouseMove({ Reinterpret<Int16>(LOWORD(lparam)), Reinterpret<Int16>(HIWORD(lparam)) });
	}

	return 1;
}

template <bool RMB, bool DBL> Reflex::UIntNative Reflex::System::Win::Window::WM_MouseDown(Window & self, UIntNative, UIntNative)
{
	self.ProcessMouseDown(RMB, DBL);

	return 0;
}

template <bool RMB> Reflex::UIntNative Reflex::System::Win::Window::WM_MouseUp(Window & self, UIntNative, UIntNative)
{
	self.ProcessMouseUp(RMB);

	return 0;
}

template <bool Y> Reflex::UIntNative Reflex::System::Win::Window::WM_MouseWheel(Window & self, UIntNative wparam, UIntNative)
{
	fPoint delta;
	
	(&delta.x)[Y] = GET_WHEEL_DELTA_WPARAM(wparam) * (32.0f / 120.0f);

	if (auto pmouseover = globals->m_mouseover)
	{
		pmouseover->m_client->OnMouseWheel(delta, false, false);
	}
	else
	{
		self.m_client->OnMouseWheel(delta, false, false);
	}

	return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_KeyDown(Window & self, UIntNative wparam, UIntNative lparam)
{
	//DEV_LOG(&self, "Window::WM_KeyDown");

	if (self.m_isplugin) UpdateModifierKeyFlags();

	if (self.ProcessKeyDown(UInt(wparam)))
	{
		return 0;
	}
	else
	{
		return self.DefaultMessageHandler(wparam, lparam);
	}

	return self.m_isplugin;

	//workaround for key + alt key causes sound.
	//correct windows solution is to not translate keys when used for keydown for shortcuts (?)
	//but this is opposite of implemented model, where trap true to keydown then forwards char to that object

	//auto rtn = PLUGIN ? 1 : global->m_modifierflags & 4;

	//return rtn;

	//return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_KeyUp(Window & self, UIntNative wparam, UIntNative lparam)
{
	if (self.ProcessKeyUp(UInt(wparam)))
	{
		return 0;
	}
	else
	{
		return self.DefaultMessageHandler(wparam, lparam);
	}
}

Reflex::UIntNative Reflex::System::Win::Window::WM_Char(Window & self, UIntNative wparam, UIntNative lparam)
{
	//DEV_LOG(&self, "Window::WM_Char");

	if (self.m_client->OnCharacter(WChar(wparam)))
	{
		return 0;
	}
	else
	{
		return self.DefaultMessageHandler(wparam, lparam);
	}
}

Reflex::UIntNative Reflex::System::Win::Window::WM_SetFocus(Window & self, UIntNative, UIntNative)
{
	//DEV_LOG(&self, "Window::WM_SetFocus", self.m_isplugin);

	UpdateModifierKeyFlags();

	self.SetMouseCursor(self.m_mousecursor);

	self.m_client->OnSetFocus();

	return 0;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_LoseFocus(Window & self, UIntNative, UIntNative)
{
	//DEV_LOG(&self, "Window::WM_LoseFocus", self.m_isplugin);

	Library::st_modifier_key_flags = 0;

	WM_MouseUp<false>(self, 0, 0);

	WM_MouseUp<true>(self, 0, 0);

	REFLEX_LOOP(idx, 256) WM_KeyUp(self, idx, 0);

	self.m_client->OnLoseFocus();

	return 0;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_GetMinMaxInfo(Window & self, UIntNative wparam, UIntNative lparam)
{
	REFLEX_STATIC_ASSERT(sizeof(LONG) == sizeof(Int32));

	auto dpifactor = globals->m_maxpixeldensity;

	auto pminmaxinfo = ToPointer<MINMAXINFO>(lparam);

	auto contentsize = self.m_client->OnGetContentSize();

	auto rect = MakeRect(0, 0, contentsize.w * dpifactor, contentsize.h * dpifactor);

	AdjustWindowRectEx(&rect, self.m_current_style, false, self.m_current_stylex);

	contentsize = { rect.right - rect.left, rect.bottom - rect.top };

	Reinterpret<iSize>(pminmaxinfo->ptMinTrackSize) = contentsize;

	Reinterpret<iSize>(pminmaxinfo->ptMaxTrackSize) = (self.m_styleflags & kWindowStyleResizable) ? iSize({ kMaxInt16, kMaxInt16 }) : contentsize;

	return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_WindowPosChanged(Window & self, UIntNative wparam, UIntNative lparam)
{
	Int32 px = globals->m_maxpixeldensity;

	auto hwnd = self.m_hwnd;


	self.m_swapchainhack->SetFullscreenState(false, 0);

	if (self.m_state != kWindowDisplayFullScreen)
	{
		WINDOWPLACEMENT windowplacement;

		GetWindowPlacement(hwnd, &windowplacement);

		self.m_state = kWindowStates[windowplacement.showCmd];
	}


	iRect temp;

	iRect * rects[] = { &temp, &self.m_xywh, &temp };

	iRect & rect = *rects[self.m_state];

	rect = self.GetRect();


	self.m_client->OnSetRect(self.m_state, rect, { {}, rect.size }, px);

	return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_Sizing(Window & self, UIntNative wparam, UIntNative lparam)
{
	auto density = globals->m_maxpixeldensity;

	auto & rect = *ToPointer<RECT>(lparam);

	rect.left = QuantiseDown<Int32>(rect.left, density);

	rect.right = QuantiseDown<Int32>(rect.right, density);

	Int32 w = QuantiseDown<Int32>(rect.right - rect.left, density);

	Int32 h = QuantiseDown<Int32>(rect.bottom - rect.top, density);

	rect.right = rect.left + w;

	rect.bottom = rect.top + h;

	return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_ExitSizing(Window & self, UIntNative wparam, UIntNative lparam)
{
	return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_Paint(Window & self, UIntNative wparam, UIntNative lparam)
{
	HWND hwnd = self.m_hwnd;

	BeginPaint(hwnd, 0);

	EndPaint(hwnd, 0);

	return 0;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_EraseBackground(Window & self, UIntNative wparam, UIntNative lparam)
{
	return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_Close(Window & self, UIntNative wparam, UIntNative lparam)
{
	//DEV_LOG(&self, "Window::WM_Close");

	self.m_client->OnRequestClose();

	return 1;
}

Reflex::UIntNative Reflex::System::Win::Window::WM_IME_Start(Window & self, UIntNative wparam, UIntNative lparam)
{
	return self.DefaultMessageHandler(wparam, lparam);
}

Reflex::UIntNative Reflex::System::Win::Window::WM_IME_End(Window & self, UIntNative wparam, UIntNative lparam)
{
	//DEV_LOG(&self, "Window::WM_IME_End");

	WChar buffer[512];

	UInt length = ImmGetCompositionStringW(ImmGetContext(self.m_hwnd), GCS_RESULTSTR, buffer, GetArraySize(buffer));

	WString::View string(buffer, length / sizeof(WChar));

	REFLEX_LOOP(idx, string.size)
	{
		WM_KeyDown(self, 0, 0);

		WM_Char(self, string[idx], 0);

		WM_KeyUp(self, 0, 0);
	}

	return self.DefaultMessageHandler(wparam, lparam);
}

Reflex::UIntNative Reflex::System::Win::Window::WM_IME_Compose(Window & self, UIntNative wparam, UIntNative lparam)
{
	return self.DefaultMessageHandler(wparam, lparam);
}

Reflex::UIntNative Reflex::System::Win::Window::WM_Destroy(Window & self, UIntNative wparam, UIntNative lparam)
{
	return self.DefaultMessageHandler(wparam, lparam);
}

REFLEX_INLINE void Reflex::System::Win::Window::ProcessMouseMove(const iPoint & position)
{
	auto density = globals->m_maxpixeldensity;

	m_client->OnMouseMove({ position.x / density, position.y / density });
}

void Reflex::System::Win::Window::SetNativeWindowRect(const iRect & rect)
{
	auto density = globals->m_maxpixeldensity;

	auto winrect = MakeRect(rect.origin.x * density, rect.origin.y * density, rect.size.w * density, rect.size.h * density);

	AdjustWindowRectEx(&winrect, m_current_style, false, m_current_stylex);

	MoveWindow(m_hwnd, winrect.left, winrect.top, winrect.right - winrect.left, winrect.bottom - winrect.top, 1);
}

Reflex::System::iRect Reflex::System::Win::Window::GetRect() const
{
	auto density = globals->m_maxpixeldensity;

	RECT rect = {};

	GetClientRect(m_hwnd, &rect);

	POINT point = { rect.left, rect.top };

	ClientToScreen(m_hwnd, &point);

	return { { point.x / density, point.y / density }, { (rect.right - rect.left) / density, (rect.bottom - rect.top) / density } };
}

REFLEX_NOINLINE void Reflex::System::Win::Window::ProcessMouseDown(bool rmb, bool dbl)
{
	if (m_isplugin) UpdateModifierKeyFlags();

	POINT point = { 0, 0 };

	ClientToScreen(m_hwnd, &point);

	m_xinit = point.x;

	m_yinit = point.y;

	SetCapture(m_hwnd);

	UInt8 bit = rmb;

	m_mousebutton = BitSet(m_mousebutton, bit);

	m_msgfn[kMOUSEMOVE] = &Window::WM_MouseMove<true>;

	SetMouseCursor(m_mousecursor);

	m_client->OnMouseDown(rmb, dbl);
}

REFLEX_NOINLINE void Reflex::System::Win::Window::ProcessMouseUp(bool rmb)
{
	UInt8 bit = rmb;

	if (BitCheck(m_mousebutton, bit))
	{
		m_mousebutton = BitClear(m_mousebutton, bit);

		m_client->OnMouseUp(rmb);
	}

	ReleaseCapture();

	SetMouseCursor(m_mousecursor);

	m_msgfn[kMOUSEMOVE] = m_mousebutton ? &Window::WM_MouseMove<true> : &Window::WM_MouseMove<false>;
}

REFLEX_NOINLINE bool Reflex::System::Win::Window::ProcessKeyDown(UInt vk)
{
	//DEV_LOG(&self, "Window::WM_KeyDown");

	if (vk < 256)
	{
		auto keycode_modifier = globals->m_vk_to_keycode_modifier[vk];

		Library::st_modifier_key_flags |= keycode_modifier.b;

		if (m_isplugin)
		{
			if (keycode_modifier.b) return false;
		}

		bool & keystate = m_keystate[keycode_modifier.a];

		bool repeat = keystate;

		keystate = true;

		return m_client->OnKeyPress(keycode_modifier.a, repeat);
	}

	return false;
}

REFLEX_NOINLINE bool Reflex::System::Win::Window::ProcessKeyUp(UInt vk)
{
	if (vk < 256)
	{
		auto keycode_modifier = globals->m_vk_to_keycode_modifier[vk];

		Library::st_modifier_key_flags &= ~keycode_modifier.b;

		if (m_isplugin && keycode_modifier.b) return false;

		bool & keystate = m_keystate[keycode_modifier.a];

		if (keystate)
		{
			keystate = false;

			m_client->OnKeyRelease(keycode_modifier.a);

			return true;
		}
	}

	return false;
}

void Reflex::System::Win::Window::UpdateModifierKeyFlags()
{
	constexpr Pair <UInt8,ModifierKeys> kModifierMap[] = { { VK_SHIFT, kModifierKeyShift }, { VK_CONTROL, kModifierKeyCtrl }, { VK_MENU, kModifierKeyAlt }, { VK_LWIN, kModifierKeySystem } };

	Library::st_modifier_key_flags = 0;

	for (auto & i : kModifierMap)
	{
		if (GetAsyncKeyState(i.a) & 0x8000) Library::st_modifier_key_flags |= i.b;
	}
}

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

template <class I> HRESULT ComObject<I>::QueryInterface(REFIID riid, void ** ppv)
{
	if (riid == IID_IUnknown || riid == m_iid.a || riid == m_iid.b)
	{
		*ppv = static_cast<IUnknown*>(this);

		AddRef();

		return S_OK;
	}

	*ppv = NULL;

	return E_NOINTERFACE;
}

template <class I> ULONG ComObject<I>::AddRef()
{
	return ++m_retaincount;
}

template <class I> ULONG ComObject<I>::Release()
{
	if (!--m_retaincount)
	{
		delete this;

		return 0;
	}

	return m_retaincount;
}

HRESULT DropSource::QueryContinueDrag(BOOL escapepressed, DWORD mbstate)
{
	if (escapepressed) return DRAGDROP_S_CANCEL;

	if (!(mbstate & (MK_LBUTTON | MK_RBUTTON)))
	{
		return DRAGDROP_S_DROP;
	}

	return S_OK;
}

IDataObject * DropSource::CreateFileDataObject(const WChar * path)
{
	IDataObject * ppv = 0;

	LPITEMIDLIST pidl = 0;

	SFGAOF sfgaof = 0;

	if (SUCCEEDED(SHParseDisplayName(path, 0, &pidl, 0, &sfgaof)))
	{
		IShellFolder * psf;

		LPCITEMIDLIST pidlchild;

		if (SUCCEEDED(SHBindToParent(pidl, IID_IShellFolder, (void**)&psf, &pidlchild)))
		{
			psf->GetUIObjectOf(GetDesktopWindow(), 1, &pidlchild, IID_IDataObject, 0, (void**)&ppv);	//was local hwnd

			psf->Release();

			//return ppv;
		}

		CoTaskMemFree(pidl);
	}

	return ppv;
}

template <bool VIRTUAL> DragFilesObject<VIRTUAL>::DragFilesObject(const DragFile * files, UInt32 n)
	: ComObject(IID_IDataObject)
{
	m_files.SetSize(n);

	REFLEX_LOOP(idx, n)
	{
		m_files[idx] = files[idx];
	}

	const WChar * id[2] = { CFSTR_FILEDESCRIPTOR, CFSTR_FILECONTENTS };

	REFLEX_LOOP(idx, 2)
	{
		FORMATETC & format = m_formats[idx];

		format.cfFormat = RegisterClipboardFormat(id[idx]);	//ignored for real file, see below
		format.tymed = TYMED_HGLOBAL;
		format.lindex = -1;
		format.dwAspect = DVASPECT_CONTENT;
		format.ptd = 0;
	}

	m_formats[1].lindex = 0;
}

template <bool VIRTUAL> DragFilesObject<VIRTUAL>::DragFilesObject(const ArrayView <WString> & filenames)
	: ComObject(IID_IDataObject)
{
	m_files.SetSize(filenames.size);

	REFLEX_LOOP(idx, m_files.GetSize())
	{
		m_files[idx].name = filenames[idx].GetData();
	}

	FORMATETC & format = m_formats[0];

	format.cfFormat = CF_HDROP;
	format.tymed = TYMED_HGLOBAL;
	format.lindex = -1;
	format.dwAspect = DVASPECT_CONTENT;
	format.ptd = 0;
}

template <bool VIRTUAL> HRESULT STDMETHODCALLTYPE DragFilesObject<VIRTUAL>::QueryGetData(__RPC__in_opt FORMATETC * pformat)
{
	FORMATETC * formats = m_formats;

	REFLEX_LOOP(idx, 1 + VIRTUAL)
	{
		FORMATETC & format = *formats++;

		if (CheckFormat(format, *pformat))
		{
			return S_OK;
		}
	}

	return S_FALSE;
}

template <bool VIRTUAL> HRESULT STDMETHODCALLTYPE DragFilesObject<VIRTUAL>::GetData(FORMATETC * format, STGMEDIUM * pmedium)
{
	if constexpr (VIRTUAL)
	{
		ZeroMemory(pmedium, sizeof(STGMEDIUM));

		if (CheckFormat(*format, m_formats[0]))		//filegroupdescriptor
		{
			Array <UInt8> data(sizeof(UINT) + (sizeof(FILEDESCRIPTOR) * m_files.GetSize()));

			FILEGROUPDESCRIPTOR & fgd = *Reinterpret<FILEGROUPDESCRIPTOR>(data.GetData());

			fgd.cItems = m_files.GetSize();

			REFLEX_LOOP(idx, m_files.GetSize())
			{
				auto & dragfile = m_files[idx];

				FILEDESCRIPTOR & fd = fgd.fgd[idx];

				MemClear(&fd, sizeof(FILEDESCRIPTOR));

				fd.dwFlags = FD_FILESIZE;

				Reflex::RawStringCopy(dragfile.name, fd.cFileName, MAX_PATH);

				fd.nFileSizeLow = dragfile.size;
			}

			pmedium->tymed = TYMED_HGLOBAL;

			return CreateHGlobal(&fgd, sizeof(FILEGROUPDESCRIPTOR), *pmedium);
		}
		else if (CheckFormat(*format, m_formats[1]))	//filecontents
		{
			pmedium->tymed = TYMED_HGLOBAL;

			return CreateHGlobal(m_files[0].data, m_files[0].size, *pmedium);
		}
	}
	else if (CheckFormat(*format, m_formats[0]))	//check it is HDROP
	{
		Array <UInt8> buffer;

		buffer.SetSize(sizeof(DROPFILES));

		MemClear(buffer.GetData(), sizeof(DROPFILES));

		for (auto & i : m_files)
		{
			WString temp = Replace(i.name, L'/', L'\\');

			UInt32 offset = buffer.GetSize();

			UInt32 num_bytes = (temp.GetSize() + 1) * sizeof(WChar);

			buffer.SetSize(buffer.GetSize() + num_bytes);

			MemCopy(temp.GetData(), buffer.GetData() + offset, num_bytes);
		}

		buffer.Push(0);
		buffer.Push(0);

		DROPFILES * dropfiles = Reinterpret<DROPFILES>(buffer.GetData());

		dropfiles->pFiles = sizeof(DROPFILES);
		dropfiles->fWide = 1;

		return CreateHGlobal(buffer.GetData(), buffer.GetSize(), *pmedium);
	}

	return DV_E_FORMATETC;
}

template <bool VIRTUAL> HRESULT STDMETHODCALLTYPE DragFilesObject<VIRTUAL>::EnumFormatEtc(DWORD direction, __RPC__deref_out_opt IEnumFORMATETC ** ppformat)
{
	if (direction == DATADIR_GET) return SHCreateStdEnumFmtEtc(1 + VIRTUAL, m_formats, ppformat);

	ppformat = 0;

	return E_NOTIMPL;
}

template <bool VIRTUAL> HRESULT STDMETHODCALLTYPE DragFilesObject<VIRTUAL>::GetCanonicalFormatEtc(__RPC__in_opt FORMATETC * formatin, __RPC__out FORMATETC * formatout)
{
	*formatout = *formatin;

	formatout->ptd = 0;

	return DATA_S_SAMEFORMATETC;
}

template <bool VIRTUAL> bool DragFilesObject<VIRTUAL>::CheckFormat(const FORMATETC & a, const FORMATETC & b)
{
	bool equal = true;

	equal = a.cfFormat == b.cfFormat;

	equal &= a.dwAspect == b.dwAspect;

	equal &= a.lindex == b.lindex;

	equal &= ((a.tymed & b.tymed) != 0);

	return equal;
}

template <bool VIRTUAL> HRESULT DragFilesObject<VIRTUAL>::CreateHGlobal(const void * data, UInt32 size, STGMEDIUM & pmedium)
{
	HGLOBAL & hglobal = pmedium.hGlobal;

	if (hglobal = GlobalAlloc(GMEM_MOVEABLE, size))
	{
		if (void * pvalloc = GlobalLock(hglobal))
		{
			CopyMemory(pvalloc, data, size);

			GlobalUnlock(hglobal);
		}
		else
		{
			GlobalFree(hglobal);

			hglobal = 0;
		}
	}

	pmedium.tymed = TYMED_HGLOBAL;
	pmedium.pUnkForRelease = 0;

	return True(hglobal) ? S_OK : E_OUTOFMEMORY;
}

REFLEX_END_INTERNAL
