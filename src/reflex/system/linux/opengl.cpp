#define REFLEX_OPENGL_ES true
#define REFLEX_BIND_OPENGL(name) WaylandOpenGL::name = ::name

#include "window.h"

#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>

#include "../common/graphics/opengl.h"
#include "../common/graphics/opengl_resources.cpp"
#include "../common/graphics/opengl.cpp"

REFLEX_NS(Reflex::System::Linux)

struct WaylandOpenGL : public Common::OpenGL
{
	struct WindowCanvas;

	WaylandOpenGL(const Config & config);

	~WaylandOpenGL();

	bool Status() const override { return m_status; }

	bool SupportsImageFormat(ImageFormat format) const override
	{
		return format == kImageFormatRGB || format == kImageFormatRGBA;
	}

	void CorrectStrokes(WString & path) override {}

	TRef <Canvas> CreateCanvas(void * systemwindow) override;

	void BeginAccess() override;

	bool MakeCurrent(EGLSurface surface);

	static void glFlushRenderFallback() {}


	EGLDisplay m_display = EGL_NO_DISPLAY;
	EGLConfig m_config = nullptr;
	EGLContext m_context = EGL_NO_CONTEXT;
	EGLSurface m_bootstrap_surface = EGL_NO_SURFACE;
	EGLSurface m_current_surface = EGL_NO_SURFACE;
	bool m_status = false;
};

struct WaylandOpenGL::WindowCanvas : public Common::OpenGL::AbstractCanvas
{
	WindowCanvas(WaylandOpenGL & engine, Window & window);

	~WindowCanvas();

	bool SetSize(const iSize & size, Int32 pixdensity) override;

	void SetCurrent() override;

	void Flush() override;


	WaylandOpenGL & engine;

	Reference <Window> m_window;

	EGLSurface m_surface = EGL_NO_SURFACE;
};

WaylandOpenGL::WaylandOpenGL(const Config & config)
{
	if (!globals->m_display)
	{
		return;
	}

	m_display = eglGetDisplay(reinterpret_cast<EGLNativeDisplayType>(globals->m_display));

	if (m_display == EGL_NO_DISPLAY || !eglInitialize(m_display, nullptr, nullptr))
	{
		return;
	}

	eglBindAPI(EGL_OPENGL_ES_API);

	constexpr EGLint k_config_attributes[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
		EGL_RED_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_BLUE_SIZE, 8,
		EGL_ALPHA_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_STENCIL_SIZE, 8,
		EGL_NONE
	};

	EGLint num_configs = 0;

	if (!eglChooseConfig(m_display, k_config_attributes, &m_config, 1, &num_configs) || num_configs == 0)
	{
		return;
	}

	constexpr EGLint k_context_attributes[] =
	{
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};

	m_context = eglCreateContext(m_display, m_config, EGL_NO_CONTEXT, k_context_attributes);

	if (m_context == EGL_NO_CONTEXT)
	{
		return;
	}

	constexpr EGLint k_pbuffer_attributes[] =
	{
		EGL_WIDTH, 1,
		EGL_HEIGHT, 1,
		EGL_NONE
	};

	m_bootstrap_surface = eglCreatePbufferSurface(m_display, m_config, k_pbuffer_attributes);

	if (m_bootstrap_surface == EGL_NO_SURFACE || !MakeCurrent(m_bootstrap_surface))
	{
		return;
	}

	OpenGL::glFlushRender = &glFlushRenderFallback;

	REFLEX_BIND_OPENGL(glGenBuffers);
	REFLEX_BIND_OPENGL(glDeleteBuffers);
	REFLEX_BIND_OPENGL(glBindBuffer);
	REFLEX_BIND_OPENGL(glBufferData);
	REFLEX_BIND_OPENGL(glBufferSubData);
	REFLEX_BIND_OPENGL(glCreateProgram);
	REFLEX_BIND_OPENGL(glDeleteProgram);
	REFLEX_BIND_OPENGL(glCreateShader);
	REFLEX_BIND_OPENGL(glDeleteShader);
	REFLEX_BIND_OPENGL(glShaderSource);
	REFLEX_BIND_OPENGL(glCompileShader);
	REFLEX_BIND_OPENGL(glAttachShader);
	REFLEX_BIND_OPENGL(glLinkProgram);
	REFLEX_BIND_OPENGL(glGetProgramiv);
	REFLEX_BIND_OPENGL(glGetProgramInfoLog);
	REFLEX_BIND_OPENGL(glGetShaderiv);
	REFLEX_BIND_OPENGL(glGetShaderInfoLog);
	REFLEX_BIND_OPENGL(glUseProgram);
	REFLEX_BIND_OPENGL(glGetUniformLocation);
	REFLEX_BIND_OPENGL(glUniform1f);
	REFLEX_BIND_OPENGL(glUniform1i);
	REFLEX_BIND_OPENGL(glUniform2fv);
	REFLEX_BIND_OPENGL(glUniform4fv);
	REFLEX_BIND_OPENGL(glUniformMatrix4fv);
	REFLEX_BIND_OPENGL(glEnableVertexAttribArray);
	REFLEX_BIND_OPENGL(glDisableVertexAttribArray);
	REFLEX_BIND_OPENGL(glVertexAttrib4fv);
	REFLEX_BIND_OPENGL(glVertexAttribPointer);
	REFLEX_BIND_OPENGL(glBindAttribLocation);
	REFLEX_BIND_OPENGL(glGenerateMipmap);
	REFLEX_BIND_OPENGL(glGenFramebuffers);
	REFLEX_BIND_OPENGL(glDeleteFramebuffers);
	REFLEX_BIND_OPENGL(glBindFramebuffer);
	REFLEX_BIND_OPENGL(glFramebufferTexture2D);
	REFLEX_BIND_OPENGL(glBlitFramebuffer);
	REFLEX_BIND_OPENGL(glCheckFramebufferStatus);
	REFLEX_BIND_OPENGL(glGenRenderbuffers);
	REFLEX_BIND_OPENGL(glDeleteRenderbuffers);
	REFLEX_BIND_OPENGL(glBindRenderbuffer);
	REFLEX_BIND_OPENGL(glRenderbufferStorage);
	REFLEX_BIND_OPENGL(glFramebufferRenderbuffer);

	eglSwapInterval(m_display, 0);

	bool render_to_texture = Common::GetValue(config, kTX, true);
	bool hd = Common::GetValue(config, kHD, true);

	Init(MakeBits(render_to_texture, false, hd));

	m_status = true;
}

WaylandOpenGL::~WaylandOpenGL()
{
	if (m_display != EGL_NO_DISPLAY)
	{
		BeginAccess();
		Deinit();

		eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

		if (m_bootstrap_surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(m_display, m_bootstrap_surface);
			m_bootstrap_surface = EGL_NO_SURFACE;
		}

		if (m_context != EGL_NO_CONTEXT)
		{
			eglDestroyContext(m_display, m_context);
			m_context = EGL_NO_CONTEXT;
		}

		eglTerminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

TRef <Renderer::Canvas> WaylandOpenGL::CreateCanvas(void * systemwindow)
{
	auto window = Reinterpret<Window>(systemwindow);

	if (!window)
	{
		return {};
	}

	return REFLEX_CREATE(WindowCanvas, *this, *window);
}

void WaylandOpenGL::BeginAccess()
{
	if (m_current_surface == EGL_NO_SURFACE && m_bootstrap_surface != EGL_NO_SURFACE)
	{
		MakeCurrent(m_bootstrap_surface);
	}

	OpenGL::BeginAccess();
}

bool WaylandOpenGL::MakeCurrent(EGLSurface surface)
{
	m_current_surface = surface;

	return eglMakeCurrent(m_display, surface, surface, m_context) == EGL_TRUE;
}

WaylandOpenGL::WindowCanvas::WindowCanvas(WaylandOpenGL & engine, Window & window)
	: AbstractCanvas(engine, 1)
	, engine(engine)
	, m_window(&window)
{
	auto egl_window = window.AcquireEGLWindow();

	if (egl_window)
	{
		m_surface = eglCreateWindowSurface(engine.m_display, engine.m_config, reinterpret_cast<EGLNativeWindowType>(egl_window), nullptr);
	}
}

WaylandOpenGL::WindowCanvas::~WindowCanvas()
{
	if (m_surface != EGL_NO_SURFACE)
	{
		eglDestroySurface(engine.m_display, m_surface);
		m_surface = EGL_NO_SURFACE;
	}
}

bool WaylandOpenGL::WindowCanvas::SetSize(const iSize & size, Int32 pixdensity)
{
	m_window->ResizeEGLWindow(size);

	return Common::CanvasBase::SetSize(size, pixdensity);
}

void WaylandOpenGL::WindowCanvas::SetCurrent()
{
	if (m_surface != EGL_NO_SURFACE)
	{
		engine.MakeCurrent(m_surface);
		AbstractCanvas::SetCurrent();
	}
}

void WaylandOpenGL::WindowCanvas::Flush()
{
	if (m_surface != EGL_NO_SURFACE)
	{
		eglSwapBuffers(engine.m_display, m_surface);
	}

	AbstractCanvas::Flush();
}

REFLEX_INLINE TRef <Renderer> CreateOpenGL(const Renderer::Config & config)
{
	return REFLEX_CREATE(WaylandOpenGL, config);
}

const Common::RendererFactory g_opengl_factory(Common::OpenGL::kEngineName, &CreateOpenGL);

REFLEX_END

bool Reflex::System::Detail::InstantiateDesktopOpenGL()
{
	return true;
}
