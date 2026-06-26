#define REFLEX_OPENGL_ES true
#define REFLEX_BIND_OPENGL(name) OpenGL::name = ::name

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>

#include "../../common/graphics/opengl.h"
#include "../../common/graphics/opengl_resources.cpp"
#include "../../common/graphics/functions.cpp"
#include "../../common/graphics/opengl.cpp"




REFLEX_BEGIN_INTERNAL(Reflex::System::Android)

EGLConfig FindMatchingEglConfig(EGLDisplay display, ArrayView <EGLConfig> configs)
{
	for (auto & config : configs)
	{
		EGLint red, green, blue, depth;

		if (eglGetConfigAttrib(display, config, EGL_RED_SIZE, &red)
			&& eglGetConfigAttrib(display, config, EGL_GREEN_SIZE, &green)
			&& eglGetConfigAttrib(display, config, EGL_BLUE_SIZE, &blue)
			&& eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth))
		{
			//DEV_LOG("Found config with R=", red, ", G=", green, ", B=", blue, ", depth=", depth);
			if (red == 8 && green == 8 && blue == 8)
			{
				return config;
			}
		}
	}

	return nullptr;
}

struct OpenGL : public Common::OpenGL
{
	OpenGL(const Config & config);
	~OpenGL();

	void CorrectStrokes(WString & path) override {}

	bool Status() const override { return m_status; }

	TRef <Canvas> CreateCanvas(void * systemwindow) override;

	bool SupportsImageFormat(ImageFormat format) const override
	{
		return format == kImageFormatRGB || format == kImageFormatRGBA;
	}

	static void glFlushRenderFallback() {}


	EGLDisplay m_display = EGL_NO_DISPLAY;
	EGLSurface m_surface = EGL_NO_SURFACE;
	EGLContext m_context = EGL_NO_CONTEXT;
	UInt32 m_aa_factor = 1;
	bool m_status = false;
};

struct WindowCanvas : public Common::OpenGL::AbstractCanvas
{
	WindowCanvas(OpenGL& engine)
		: AbstractCanvas(engine, 1)
	{
	}

	void Flush() override
	{
		auto engine = Cast<OpenGL>(AbstractCanvas::engine);
		auto swapResult = eglSwapBuffers(engine->m_display, engine->m_surface);
		REFLEX_ASSERT_EX(swapResult == EGL_TRUE, "Failed to swap OpenGL");
		AbstractCanvas::Flush();
	}
};

OpenGL::OpenGL(const Config & config)
{
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
	REFLEX_BIND_OPENGL(glUniform1f);
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


	constexpr EGLint attribs[] =
	{
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
		EGL_BLUE_SIZE, 8,
		EGL_GREEN_SIZE, 8,
		EGL_RED_SIZE, 8,
		EGL_DEPTH_SIZE, 24,
		EGL_NONE
	};

	// The default m_display is probably what you want on Android
	m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(m_display, nullptr, nullptr);

	// figure out how many configs there are
	EGLint num_config;
	eglChooseConfig(m_display, attribs, nullptr, 0, &num_config);

	// get the list of configurations
	Array <EGLConfig> supported_configs(num_config);
	eglChooseConfig(m_display, attribs, supported_configs.GetData(), num_config, &num_config);

	auto egl_config = FindMatchingEglConfig(m_display, { supported_configs.GetData(), UInt(num_config) });
	REFLEX_ASSERT_EX(egl_config, "No appropriate config, will crash");

	// create the proper window m_surface
	EGLint format;
	eglGetConfigAttrib(m_display, egl_config, EGL_NATIVE_VISUAL_ID, &format);
	m_surface = eglCreateWindowSurface(m_display, egl_config, Library::st_androidapp->window, nullptr);

	// Create a GLES 3 m_context
	EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
	m_context = eglCreateContext(m_display, egl_config, nullptr, context_attribs);

	// get some window metrics
	m_status = eglMakeCurrent(m_display, m_surface, m_surface, m_context);
	REFLEX_ASSERT_EX(m_status, "Could not make the OpenGL m_context current");

	//stop SwapBuffers from blocking, which leaves no time for other tasks on main thread
	eglSwapInterval(m_display, 0);

	bool renderToTexture = Common::GetValue(config, kTX, true); // true because GLES version >= 3.0
	bool nativeScreenDensity = Common::GetValue(config, kHD, true) && globals->CurrentScreenDensity() > 1;
	// MEMO: anti-aliasing not supported for now (and removed from Metal as performance was found to be lower than oversampling)
	//m_aa_factor = Common::GetValue(config, kAA, true) ? 1 : 1;

	Init(MakeBits(renderToTexture, false, nativeScreenDensity));
}

OpenGL::~OpenGL()
{
	// https://stackoverflow.com/questions/54183090/eglcreatewindowsurface-native-window-api-connect-failed
	if (m_display != EGL_NO_DISPLAY)
	{
		BeginAccess();
		Deinit();
		EndAccess();

		eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		if (m_context != EGL_NO_CONTEXT)
		{
			eglDestroyContext(m_display, m_context);
			m_context = EGL_NO_CONTEXT;
		}
		if (m_surface != EGL_NO_SURFACE)
		{
			eglDestroySurface(m_display, m_surface);
			m_surface = EGL_NO_SURFACE;
		}
		eglTerminate(m_display);
		m_display = EGL_NO_DISPLAY;
	}
}

// void UnsetGraphicsContext(); // when the window becomes unusable -- call in APP_CMD_TERM_WINDOW
// void TransferGraphicsContext(); // when a new window needs to be associated with the current context (if any) -- call in APP_CMD_INIT_WINDOW

//void OpenGL::UnsetGraphicsContext() {
//	// Destroy the context, but keep the resources
//	eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
//	if (m_surface) eglDestroySurface(m_display, m_surface);
//	m_surface = nullptr;
//}
//
//void OpenGL::TransferGraphicsContext() {
//	if (m_surface) return;
//	auto global = TheLibrary::Get();
//	// restore the context
//	m_surface = eglCreateWindowSurface(m_display, m_glConfigForContext, global->m_androidApp->window, nullptr);
//	auto madeCurrent = eglMakeCurrent(m_display, m_surface, m_surface, m_context);
//	REFLEX_ASSERT_EX(madeCurrent, "Could not make the OpenGL m_context current");
//}

TRef<OpenGL::Canvas> OpenGL::CreateCanvas(void *systemwindow)
{
	return REFLEX_CREATE(WindowCanvas, *this);
}

const Common::RendererFactory g_opengl_factory(Common::OpenGL::kEngineName, [](const Renderer::Config & config) -> TRef <Renderer>
{
	return REFLEX_CREATE(OpenGL, config);
});

REFLEX_END_INTERNAL

bool Reflex::System::Detail::InstantiateDesktopOpenGL()
{
	return True(&Android::g_opengl_factory);
}
