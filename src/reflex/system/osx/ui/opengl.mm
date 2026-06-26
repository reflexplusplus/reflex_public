#define REFLEX_MACOS_TARGET REFLEX_TARGET_OPENGL

#include "graphics.h"

#include "../../common/graphics/opengl.cpp"
#include "../../common/graphics/opengl_resources.cpp"

#define REFLEX_BIND_OPENGL(name) OpenGL::name = &::name
#define REFLEX_BIND_OPENGL_ARB(name) OpenGL::name = &::name##EXT




//
//macos graphics impl

REFLEX_DISABLE_WARNINGS

@interface NS_OpenGLView : NSOpenGLView
{
@public
}

- (BOOL) acceptsFirstMouse:(NSEvent*)event;
- (void) rightMouseDown:(NSEvent*)event;

@end

@implementation NS_OpenGLView
{
}

- (BOOL) acceptsFirstMouse:(NSEvent*)event
{
	return YES;
}

- (void) rightMouseDown:(NSEvent*)event;
{
	[[self nextResponder] rightMouseDown:event];
}

@end

REFLEX_BEGIN_INTERNAL(Reflex::System::OSX)

struct OpenGLImpl : public Common::OpenGL
{
	struct ScreenCanvasImpl;

	OpenGLImpl(bool hd, bool aa, bool rb);

	~OpenGLImpl();

	bool Status() const override { return true; }

	bool SupportsImageFormat(ImageFormat format) const override;

	void BeginAccess() override;

	TRef <Canvas> CreateCanvas(void * system_window) override;

	void CorrectStrokes(WString & path) override {}


	ObjCRef <NSOpenGLPixelFormat*> m_pixelformat;

	ObjCRef <NSOpenGLContext*> m_context;

	bool m_enableretina;

	UInt32 m_aa_factor;
};

struct OpenGLImpl::ScreenCanvasImpl : public Common::OpenGL::AbstractCanvas
{
	ScreenCanvasImpl(OpenGLImpl & engine, NSView * nsview);

	~ScreenCanvasImpl();

	bool SetSize(const iSize & size, Int32 dpi) override;

	void SetCurrent() override;

	void Flush() override;


	OpenGLImpl & engine;

	ObjCRef <NS_OpenGLView*> m_nsopenglview;

	ObjCRef <NSOpenGLContext*> m_context;
};

OpenGLImpl::OpenGLImpl(bool hd, bool aa, bool rb)
	: m_enableretina(hd && globals->m_pixeldensity > 1)
{
	//extensions

	Common::OpenGL::glFlushRender = &::glFlushRenderAPPLE;


	//context

	typedef NSOpenGLPixelFormatAttribute Attribute;

	if (aa)
	{
		m_aa_factor = GetMaxPixelDensity() > 1 ? 2 : 4;

		Attribute attributes[] = { NSOpenGLPFAAccelerated, NSOpenGLPFAWindow, NSOpenGLPFASampleBuffers, Attribute(1), NSOpenGLPFASamples, Attribute(m_aa_factor), NSOpenGLPFAStencilSize, Attribute(8), Attribute(0) };

		m_pixelformat = MakeOwnedObjCRef([[NSOpenGLPixelFormat alloc] initWithAttributes:attributes]);
	}
	else
	{
		m_aa_factor = 1;

		Attribute attributes[] = { NSOpenGLPFAAccelerated, NSOpenGLPFAWindow, NSOpenGLPFAStencilSize, Attribute(8), Attribute(0) };

		m_pixelformat = MakeOwnedObjCRef([[NSOpenGLPixelFormat alloc] initWithAttributes:attributes]);
	}

	m_context = MakeOwnedObjCRef([[NSOpenGLContext alloc] initWithFormat:m_pixelformat shareContext:nil]);

	[m_context makeCurrentContext];



	//bind

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

	bool v3 = GetVersion() >= 3.0f;

    if (v3)
    {
        REFLEX_BIND_OPENGL(glGenFramebuffers);
        REFLEX_BIND_OPENGL(glDeleteFramebuffers);
        REFLEX_BIND_OPENGL(glBindFramebuffer);
        REFLEX_BIND_OPENGL(glFramebufferTexture2D);
        REFLEX_BIND_OPENGL(glCheckFramebufferStatus);
        REFLEX_BIND_OPENGL(glGenRenderbuffers);
        REFLEX_BIND_OPENGL(glDeleteRenderbuffers);
        REFLEX_BIND_OPENGL(glBindRenderbuffer);
        REFLEX_BIND_OPENGL(glRenderbufferStorage);
        REFLEX_BIND_OPENGL(glFramebufferRenderbuffer);
        REFLEX_BIND_OPENGL(glBlitFramebuffer);
        REFLEX_BIND_OPENGL(glGenerateMipmap);
    }
	else if (CheckExtension("GL_ARB_framebuffer_object"))
	{
        REFLEX_BIND_OPENGL_ARB(glGenFramebuffers);
        REFLEX_BIND_OPENGL_ARB(glDeleteFramebuffers);
        REFLEX_BIND_OPENGL_ARB(glBindFramebuffer);
        REFLEX_BIND_OPENGL_ARB(glFramebufferTexture2D);
        REFLEX_BIND_OPENGL_ARB(glCheckFramebufferStatus);
        REFLEX_BIND_OPENGL_ARB(glGenRenderbuffers);
        REFLEX_BIND_OPENGL_ARB(glDeleteRenderbuffers);
        REFLEX_BIND_OPENGL_ARB(glBindRenderbuffer);
        REFLEX_BIND_OPENGL_ARB(glRenderbufferStorage);
        REFLEX_BIND_OPENGL_ARB(glFramebufferRenderbuffer);
        REFLEX_BIND_OPENGL_ARB(glBlitFramebuffer);
        REFLEX_BIND_OPENGL_ARB(glGenerateMipmap);
    }
	else
	{
		rb = false;
	}


	//init

	Init(MakeBits(rb, m_aa_factor > 1, m_enableretina));
}

OpenGLImpl::~OpenGLImpl()
{
	DebugLog(false, "~OpenGLImpl");

	BeginAccess();

	Deinit();

	EndAccess();

	[NSOpenGLContext clearCurrentContext];
}

bool OpenGLImpl::SupportsImageFormat(ImageFormat format) const
{
	return format != kImageFormatLuminance;
}

void OpenGLImpl::BeginAccess()
{
	[m_context makeCurrentContext];

	OpenGL::BeginAccess();
}

TRef <Renderer::Canvas> OpenGLImpl::CreateCanvas(void * system_window)
{
	return REFLEX_CREATE(ScreenCanvasImpl, *this, (__bridge NSView*)system_window);
}

OpenGLImpl::ScreenCanvasImpl::ScreenCanvasImpl(OpenGLImpl & engine, NSView * nsview)
	: AbstractCanvas(engine, 1)
	, engine(engine)
	, m_nsopenglview(MakeOwnedObjCRef([[NS_OpenGLView alloc] initWithFrame:NSMakeRect(0,0,0,0) pixelFormat:engine.m_pixelformat]))
{
	if ([m_nsopenglview respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)])
	{
  		[m_nsopenglview setWantsBestResolutionOpenGLSurface:engine.m_enableretina];
	}

	[nsview addSubview:m_nsopenglview];

	m_context = MakeOwnedObjCRef([[NSOpenGLContext alloc] initWithFormat:engine.m_pixelformat shareContext:engine.m_context]);

	[m_nsopenglview setOpenGLContext:m_context];

	GLint param = 1;

	[m_context setValues:&param forParameter:NSOpenGLCPSwapInterval];

	[engine.m_context makeCurrentContext];
}

OpenGLImpl::ScreenCanvasImpl::~ScreenCanvasImpl()
{
	[m_nsopenglview clearGLContext];

	[engine.m_context makeCurrentContext];
}

bool OpenGLImpl::ScreenCanvasImpl::SetSize(const iSize & size, Int32 dpi)
{
	if (size.w * size.h)
	{
		[m_context makeCurrentContext];

		[m_nsopenglview setFrameSize:NSMakeSize(size.w, size.h)];

		[m_context clearDrawable];

		[m_context setView:m_nsopenglview];

		[m_context update];

		OpenGL::InitContext();
	}

	return AbstractCanvas::SetSize(size, dpi);
}

void OpenGLImpl::ScreenCanvasImpl::SetCurrent()
{
	[m_context makeCurrentContext];

	AbstractCanvas::SetCurrent();
}

void OpenGLImpl::ScreenCanvasImpl::Flush()
{
	AbstractCanvas::Flush();

	glSwapAPPLE();
}

const Common::RendererFactory g_opengl_factory(Common::OpenGL::kEngineName, &CreateRenderer<OpenGLImpl>);

REFLEX_END_INTERNAL

REFLEX_ENABLE_WARNINGS

bool Reflex::System::Detail::InstantiateDesktopOpenGL()
{
	return True(&OSX::g_opengl_factory);
}
