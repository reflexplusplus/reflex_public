#include "dpi_awareness.h"
#include "../library.h"

#include <GL\gl.h>
//#include "ext/opengl/wglext.h"

#define GL_TEXTURE_SWIZZLE_R                             0x8E42
#define GL_TEXTURE_SWIZZLE_G                             0x8E43
#define GL_TEXTURE_SWIZZLE_B                             0x8E44
#define GL_TEXTURE_SWIZZLE_A                             0x8E45
#define GL_MULTISAMPLE                    0x809D
#define GL_MAX_RENDERBUFFER_SIZE          0x84E8
#define GL_FRAMEBUFFER                    0x8D40
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_RENDERBUFFER                   0x8D41
#define GL_DEPTH24_STENCIL8               0x88F0
#define GL_COLOR_ATTACHMENT0              0x8CE0
#define GL_DEPTH_ATTACHMENT               0x8D00
#define GL_STENCIL_ATTACHMENT             0x8D20
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STATIC_DRAW                    0x88E4
#define GL_CLAMP_TO_EDGE                  0x812F
#define GL_INVALID_FRAMEBUFFER_OPERATION  0x0506
#define GL_COMPILE_STATUS                 0x8B81
#define GL_LINK_STATUS                    0x8B82
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_INFO_LOG_LENGTH                0x8B84
typedef char GLchar;
typedef ptrdiff_t GLintptr;
typedef ptrdiff_t GLsizeiptr;

#define WGL_SUPPORT_OPENGL_ARB         0x2010
#define WGL_DRAW_TO_WINDOW_ARB         0x2001
#define WGL_ACCELERATION_ARB           0x2003
#define WGL_DOUBLE_BUFFER_ARB          0x2011
#define WGL_FULL_ACCELERATION_ARB      0x2027
#define WGL_COLOR_BITS_ARB             0x2014
#define WGL_ALPHA_BITS_ARB             0x201B
#define WGL_ACCUM_BITS_ARB             0x201D
#define WGL_DEPTH_BITS_ARB             0x2022
#define WGL_STENCIL_BITS_ARB           0x2023
#define WGL_SAMPLE_BUFFERS_ARB         0x2041
#define WGL_SAMPLES_ARB                0x2042
typedef BOOL(WINAPI * PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
typedef BOOL(WINAPI * PFNWGLSWAPINTERVALEXTPROC) (int interval);

#include "../../common/graphics/opengl.cpp"
#include "../../common/graphics/opengl_resources.cpp"

#pragma comment(lib, "opengl32.lib")	//opengl




#define GET_EXTENSION(fn, check) GetExtension(REFLEX_STRINGIFY(fn), Reflex::System::Common::OpenGL::fn, check)

#define GET_EXTENSION_ARB(fn, check) GetExtension(REFLEX_STRINGIFY(fn##EXT), Reflex::System::Common::OpenGL::fn, check)




//
//common

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct WGL : public Common::OpenGL
{
	struct WindowCanvas;


	WGL(bool hd, const Config & config);

	~WGL();


	bool Status() const override { return m_status; }

	bool SupportsImageFormat(ImageFormat format) const 
	{
		//TODO reenable luminance on desktop and ideally android
		//was disabled to get opengl premult shader working

		return format != kImageFormatLuminance;
	}

	void BeginAccess() override;

	TRef <Canvas> CreateCanvas(void * systemwindow) override;;

	void CorrectStrokes(WString & wstring) override;;



	//internal

	REFLEX_INLINE BOOL MakeCurrent(HDC hdc)
	{
		m_current_hdc = hdc;

		return wglMakeCurrent(hdc, m_hglrc);
	}

	template <class FN> bool GetExtension(const char * name, FN * & fn, const FN & fallback);

	template <class FN> void GetExtension(const char * name, FN & fn, bool & status);


	static LRESULT CALLBACK NullWinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	static BOOL __stdcall wglChoosePixelFormatFallback(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats) {return false;}
	static BOOL __stdcall wglSwapIntervalFallback(int interval) {return 1;}

	static void STDCALL glFlushRenderFallback() {}

	static void __stdcall glGenerateMipmapFallback(GLenum target) {}

	static void __stdcall glBindFramebufferFallback(GLenum target, GLuint framebuffer) {}
	static void __stdcall glGenFramebuffersFallback(GLsizei n, GLuint *framebuffers){}
	static void __stdcall glDeleteFramebuffersFallback(GLsizei n, const GLuint *framebuffers){}


	bool m_status;


	PIXELFORMATDESCRIPTOR m_pfd;

	Int32 m_pixelformat;


	HWND m_hwnd;

	HDC m_hdc;

	HGLRC m_hglrc;

	HDC m_current_hdc;


	decltype (glGenBuffers) glGenBuffers_internal;

	decltype (glDeleteBuffers) glDeleteBuffers_internal;
};

struct WGL::WindowCanvas : public Common::OpenGL::AbstractCanvas
{
	WindowCanvas(WGL & engine, HWND hwnd);

	~WindowCanvas();


	void SetCurrent();

	void Flush();


	WGL & engine;

	HWND m_parent;

	HDC m_hdc;
};

WGL::WGL(bool hd, const Config & config)
	: m_status(false)
	, m_hglrc(0)
{
	globals->m_enable_truefullscreen = false;	//GetValue(Common::CheckConfig(config, K32("FS"), false);



	//get pixelformat

	m_hwnd = CreateWindowEx(WS_EX_TOPMOST, Library::kVisibleWindowClass.data, L"", 0, 0, 0, 16, 16, 0, 0, Library::st_hinstance, 0);

	SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, LONG_PTR(&NullWinProc));

	m_hdc = GetDC(m_hwnd);

	MemClear(&m_pfd, sizeof(PIXELFORMATDESCRIPTOR));
	m_pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	m_pfd.nVersion = 1;
	m_pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	m_pfd.iPixelType = PFD_TYPE_RGBA;
	m_pfd.iLayerType = PFD_MAIN_PLANE;
	m_pfd.cColorBits = 24;
	m_pfd.cAlphaBits = 8;
	m_pfd.cStencilBits = 8;

	m_pixelformat = ChoosePixelFormat(m_hdc, &m_pfd);

	m_status = True(SetPixelFormat(m_hdc, m_pixelformat, &m_pfd));

	m_hglrc = wglCreateContext(m_hdc);

	m_status = m_status && MakeCurrent(m_hdc);



	//get extended pixelformat

	//UInt aa_factor = 1;

	//if (m_status && Common::GetValue(config, kAA, true))
	//{
	//	aa_factor = 4 / global->m_maxpixeldensity;

	//	//OpenGL::Check check(*this, "Request Extended Pixelformat");

	//	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormat;

	//	GetExtension("wglChoosePixelFormatARB", wglChoosePixelFormat, wglChoosePixelFormatFallback);

	//	while (aa_factor > 1)
	//	{
	//		Int32 attributes[] =
	//		{
	//			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
	//			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
	//			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
	//			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
	//			WGL_COLOR_BITS_ARB, 24,
	//			WGL_ALPHA_BITS_ARB, 8,
	//			WGL_STENCIL_BITS_ARB, 8,
	//			WGL_DEPTH_BITS_ARB, 0,
	//			WGL_ACCUM_BITS_ARB, 0,
	//			WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
	//			WGL_SAMPLES_ARB, Int32(aa_factor),
	//			//WGL_SWAP_COPY_ARB, GL_TRUE,	//for freestyle, remove for LIBV3
	//			0
	//		};

	//		Float32 fattributes[] = { 0,0 };

	//		Int32 result = 0;

	//		UINT numformats = 0;

	//		bool ok = wglChoosePixelFormat(m_hdc, attributes, fattributes, 1, &result, &numformats);

	//		if (ok && numformats)
	//		{
	//			m_pixelformat = result;

	//			wglDeleteContext(m_hglrc);

	//			DestroyWindow(m_hwnd);

	//			m_hwnd = CreateWindowEx(WS_EX_TOPMOST, Library::kVisibleWindowClass.data, L"", 0, 0, 0, 16, 16, 0, 0, global->m_hinstance, 0);

	//			SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, LONG_PTR(&NullWinProc));

	//			m_hdc = GetDC(m_hwnd);

	//			SetPixelFormat(m_hdc, m_pixelformat, &m_pfd);

	//			m_hglrc = wglCreateContext(m_hdc);

	//			MakeCurrent(m_hdc);

	//			break;
	//		}

	//		aa_factor /= 2;
	//	}
	//}



	//get extensions

	glFlushRender = &glFlushRenderFallback;


	PFNWGLSWAPINTERVALEXTPROC wglSwapInterval;

	GetExtension("wglSwapIntervalEXT", wglSwapInterval, wglSwapIntervalFallback);

	(*wglSwapInterval)(0);


	GET_EXTENSION(glGenBuffers, m_status);
	GET_EXTENSION(glDeleteBuffers, m_status);

	GET_EXTENSION(glBindBuffer, m_status);
	GET_EXTENSION(glBufferData, m_status);
	GET_EXTENSION(glBufferSubData, m_status);

	GET_EXTENSION(glCreateProgram, m_status);
	GET_EXTENSION(glDeleteProgram, m_status);
	GET_EXTENSION(glCreateShader, m_status);
	GET_EXTENSION(glDeleteShader, m_status);
	GET_EXTENSION(glShaderSource, m_status);
	GET_EXTENSION(glCompileShader, m_status);
	GET_EXTENSION(glAttachShader, m_status);
	GET_EXTENSION(glLinkProgram, m_status);;
	GET_EXTENSION(glGetProgramiv, m_status);
	GET_EXTENSION(glGetProgramInfoLog, m_status);
	GET_EXTENSION(glGetShaderiv, m_status);
	GET_EXTENSION(glGetShaderInfoLog, m_status);
	GET_EXTENSION(glUseProgram, m_status);
	GET_EXTENSION(glGetUniformLocation, m_status);
	GET_EXTENSION(glUniform1i, m_status);
	GET_EXTENSION(glUniform1f, m_status);
	GET_EXTENSION(glUniform2fv, m_status);
	GET_EXTENSION(glUniform4fv, m_status);
	GET_EXTENSION(glUniformMatrix4fv, m_status);
	GET_EXTENSION(glVertexAttrib4fv, m_status);
	GET_EXTENSION(glVertexAttribPointer, m_status);
	GET_EXTENSION(glEnableVertexAttribArray, m_status);
	GET_EXTENSION(glDisableVertexAttribArray, m_status);
	GET_EXTENSION(glBindAttribLocation, m_status);

	bool rb = Common::GetValue(config, kTX, true);

	if (GetVersion() >= 3.0f)
	{
		GET_EXTENSION(glGenFramebuffers, rb);
		GET_EXTENSION(glDeleteFramebuffers, rb);
		GET_EXTENSION(glBindFramebuffer, rb);
		GET_EXTENSION(glFramebufferTexture2D, rb);
		GET_EXTENSION(glBlitFramebuffer, rb);
		GET_EXTENSION(glCheckFramebufferStatus, rb);
		GET_EXTENSION(glGenRenderbuffers, rb);
		GET_EXTENSION(glDeleteRenderbuffers, rb);
		GET_EXTENSION(glBindRenderbuffer, rb);
		GET_EXTENSION(glRenderbufferStorage, rb);
		GET_EXTENSION(glFramebufferRenderbuffer, rb);
		GET_EXTENSION(glGenerateMipmap, rb);
	}
	else if (CheckExtension("GL_ARB_framebuffer_object"))
	{
		GET_EXTENSION_ARB(glGenFramebuffers, rb);
		GET_EXTENSION_ARB(glDeleteFramebuffers, rb);
		GET_EXTENSION_ARB(glBindFramebuffer, rb);
		GET_EXTENSION_ARB(glFramebufferTexture2D, rb);
		GET_EXTENSION_ARB(glBlitFramebuffer, rb);
		GET_EXTENSION_ARB(glCheckFramebufferStatus, rb);
		GET_EXTENSION_ARB(glGenRenderbuffers, rb);
		GET_EXTENSION_ARB(glDeleteRenderbuffers, rb);
		GET_EXTENSION_ARB(glBindRenderbuffer, rb);
		GET_EXTENSION_ARB(glRenderbufferStorage, rb);
		GET_EXTENSION_ARB(glFramebufferRenderbuffer, rb);
		GET_EXTENSION_ARB(glGenerateMipmap, rb);
	}
	else
	{
		rb = false;
	}

	if (m_status) Init(MakeBits(rb, false, hd));
}

#undef GET_EXTENSION

WGL::~WGL()
{
	BeginAccess();

	Deinit();

	EndAccess();

	wglMakeCurrent(0, 0);

	wglDeleteContext(m_hglrc);

	DestroyWindow(m_hwnd);
}

void WGL::BeginAccess()
{
	MakeCurrent(m_current_hdc);

	Common::OpenGL::BeginAccess();
}

TRef <Renderer::Canvas> WGL::CreateCanvas(void * syswindow)
{
	return REFLEX_CREATE(WindowCanvas, *this, HWND(syswindow));
}

void WGL::CorrectStrokes(WString & wstring)
{
	CorrectStrokes(wstring);
}

template <class FN> void WGL::GetExtension(const char * name, FN & fn, bool & status)
{
	fn = (FN)wglGetProcAddress(name);

	bool ok = True(fn);

	if (!ok) Common::output.Warn("WGL::GetExtension", name, "FAIL");

	status = status && ok;
}

template <class FN> bool WGL::GetExtension(const char * name, FN * & fn, const FN & fallback)
{
	fn = (FN*)wglGetProcAddress(name);

	bool ok = True(fn);

	if (!ok) Common::output.Warn("WGL::GetExtension", name, "FAIL");

	fn = ok ? fn : fallback;

	return ok;
}

LRESULT CALLBACK WGL::NullWinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_ERASEBKGND) return 1;

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

WGL::WindowCanvas::WindowCanvas(WGL & engine, HWND hwnd)
	: Common::OpenGL::AbstractCanvas(engine, 1)
	, engine(engine)
	, m_parent(hwnd)
	, m_hdc(GetDC(hwnd))
{
	SetPixelFormat(m_hdc, engine.m_pixelformat, &engine.m_pfd);

	wglMakeCurrent(engine.m_hdc, engine.m_hglrc);
}

WGL::WindowCanvas::~WindowCanvas()
{
	if (engine.m_current_hdc == m_hdc) engine.m_current_hdc = engine.m_hdc;
}

void WGL::WindowCanvas::SetCurrent()
{
	engine.MakeCurrent(m_hdc);

	Common::OpenGL::AbstractCanvas::SetCurrent();
}

void WGL::WindowCanvas::Flush()
{
	::SwapBuffers(m_hdc);

	Common::OpenGL::AbstractCanvas::Flush();
}

TRef <Renderer> CreateOpenGL(const Renderer::Config & config)
{
	return REFLEX_CREATE(WGL, InstantiateDpiAwareness(config), config);
}

const Common::RendererFactory g_opengl_factory(Common::OpenGL::kEngineName, &CreateOpenGL);

REFLEX_END_INTERNAL

#undef GET_EXTENSION

bool Reflex::System::Detail::InstantiateDesktopOpenGL()
{
	return True(&Win::g_opengl_factory);
}