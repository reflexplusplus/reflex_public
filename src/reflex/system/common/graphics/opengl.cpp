#include "opengl.h"
#include "opengl_resources.h"
#include "matrix.h"




//
//macros

#if (REFLEX_DEBUG | 1)
#define GL_CHECK(engine, label) Check check(engine, label)
#else
#define GL_CHECK(engine, label)
#endif




//
//

REFLEX_NS(Reflex::System::Common)

template <bool RENDER_TARGET, bool MIPMAP> Renderer::Canvas * OpenGL::CreateBitmapImpl(OpenGL & engine, bool alphachannel)
{
	typedef Bitmap <RENDER_TARGET,MIPMAP> Type;

	return REFLEX_CREATE(Type, engine, alphachannel);
}

void STDCALL glBindFramebuffer_NULL(GLenum target, GLuint framebuffer) {}

Float32 OpenGL::st_matrixcache[16] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

#if REFLEX_OPENGL_ES
#define glTexEnvi(...)
constexpr GLenum GL_MULTISAMPLE = 0;
const Reflex::UInt32 OpenGL::kImageFormats[] = { GL_RGB, GL_RGB, GL_RGBA, GL_RGBA, GL_RED };
#else
const Reflex::UInt32 OpenGL::kImageFormats[] = { GL_RGB, GL_BGR_EXT, GL_RGBA, GL_BGRA_EXT, GL_RED };
#endif

const GLenum OpenGL::kPrimitiveTypes[] = { GL_LINES, GL_LINE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_POINTS };

class OpenGL::Check
{
public:

	Check(const OpenGL & engine, const char * block);

	~Check();

	bool Try(const char * msg);



private:

	void ShowError(GLenum error);


	const OpenGL & engine;

	const char * m_block;

};

struct OpenGL::AbstractBitmap : public AbstractCanvas
{
	AbstractBitmap(OpenGL & engine);

	~AbstractBitmap() override;

	const GLuint m_textureid;

	iSize m_size_pow2;
};

template <bool RENDER_TARGET, bool MIPMAP>
struct OpenGL::Bitmap : public AbstractBitmap
{
	Bitmap(OpenGL & engine, bool antialias);

	~Bitmap() override;

	bool SetSize(const iSize & size, Int32 dpifactor) override;

	void Write(ImageFormat format, const ArrayView <UInt8> & bytes) override;

	void SetCurrent() override;

	void Flush() override;

	TRef <Graphic> CreateTextures(const ArrayView < Pair <fRect> > & rects) const override;

	TRef <Graphic> CreateTexturesWithFilter(const ArrayView < Pair <fRect> >& rects, FilterMode mode, const ArrayView <Float>& parameters) const override;


	GLint m_internalformat;

	GLuint m_fboid;
};

template <class POINT>
struct OpenGL::Primitives : public Graphic
{
	Primitives(OpenGL & engine, GLenum mode, const ArrayView <POINT> & points);

	~Primitives() override;

	void Render(const Transform & transform, const Colour & colour) const override;


	OpenGL & engine;

	GLuint m_vboid;

	UInt32 m_nvertex;

	GLenum m_mode;


	static const Colour kWhite;
};

struct OpenGL::Textures : public Graphic
{
	Textures(const AbstractBitmap & bitmap, const ArrayView < Pair <fRect> > & rects);

	~Textures() override;

	void Render(const Transform & transform, const Colour & colour) const override;


	const AbstractBitmap & bitmap;

	GLuint m_vboid;

	UInt32 m_nvertex;
};

struct OpenGL::TexturesWithFilter : public OpenGL::Textures {

	TexturesWithFilter(const AbstractBitmap & bitmap, const ArrayView < Pair <fRect> > & rects, FilterMode mode, const ArrayView <Float>& in_params);

	void Render(const Transform & transform, const Colour & colour) const override;

	FilterMode m_filtermode;

	Float m_filterparams[5 * 4];
};

Reflex::Float32 OpenGL::GetVersion()
{
	auto version = ToView(Reinterpret<char>(glGetString(GL_VERSION)));

	if (auto pos = Search(version, ' ')) return Quantise(ToFloat32(Splice(version, pos.value).a), 0.1f);

	return 0.0f;
}

bool OpenGL::CheckExtension(const CString::View & extension)
{
	CString::View extensions(Reinterpret<char>(glGetString(GL_EXTENSIONS)));

	return True(Search<CaseInsensitive>(extensions, extension));
}

OpenGL::OpenGL()
	: m_capabilities(0)
	, m_texturesize(0)
	, m_program(m_programs)
	, m_fbo_output(0)
	, m_active(false)
	, m_current_canvas(nullptr)
	, m_dither_amount(0.0f)
{
	ResetMatrix(st_matrixcache);

	MemCopy(kDefaultColourTransform, m_colour_transform, sizeof(kDefaultColourTransform));
}

void OpenGL::Init(UInt8 flags)
{
	REFLEX_ASSERT(glFlushRender);

	REFLEX_ASSERT(glGenBuffers);
	REFLEX_ASSERT(glDeleteBuffers);
	REFLEX_ASSERT(glBindBuffer);
	REFLEX_ASSERT(glBufferData);
	REFLEX_ASSERT(glBufferSubData);

	REFLEX_ASSERT(glCreateProgram);
	REFLEX_ASSERT(glDeleteProgram);
	REFLEX_ASSERT(glCreateShader);
	REFLEX_ASSERT(glDeleteShader);
	REFLEX_ASSERT(glShaderSource);
	REFLEX_ASSERT(glCompileShader);
	REFLEX_ASSERT(glAttachShader);
	REFLEX_ASSERT(glLinkProgram);
	REFLEX_ASSERT(glGetProgramiv);
	REFLEX_ASSERT(glGetProgramInfoLog);
	REFLEX_ASSERT(glGetShaderiv);
	REFLEX_ASSERT(glGetShaderInfoLog);
	REFLEX_ASSERT(glUseProgram);
	REFLEX_ASSERT(glGetUniformLocation);
	REFLEX_ASSERT(glUniform1f);
	REFLEX_ASSERT(glUniform1i);
	REFLEX_ASSERT(glUniform2fv);
	REFLEX_ASSERT(glUniform4fv);
	REFLEX_ASSERT(glUniformMatrix4fv);
	REFLEX_ASSERT(glEnableVertexAttribArray);
	REFLEX_ASSERT(glDisableVertexAttribArray);
	REFLEX_ASSERT(glVertexAttrib4fv);
	REFLEX_ASSERT(glVertexAttribPointer);
	REFLEX_ASSERT(glBindAttribLocation);

	REFLEX_ASSERT(glGenerateMipmap);
	REFLEX_ASSERT(glGenFramebuffers);
	REFLEX_ASSERT(glDeleteFramebuffers);
	REFLEX_ASSERT(glBindFramebuffer);
	REFLEX_ASSERT(glFramebufferTexture2D);
	REFLEX_ASSERT(glBlitFramebuffer);
	REFLEX_ASSERT(glCheckFramebufferStatus);
	REFLEX_ASSERT(glGenRenderbuffers);
	REFLEX_ASSERT(glDeleteRenderbuffers);
	REFLEX_ASSERT(glBindRenderbuffer);
	REFLEX_ASSERT(glRenderbufferStorage);
	REFLEX_ASSERT(glFramebufferRenderbuffer);

	GL_CHECK(*this, "OpenGL::Init");

	output.LogEx(kLogNormal, {}, "OpenGL Vendor: ", (const char *)glGetString(GL_VENDOR));
	output.LogEx(kLogNormal, {}, "OpenGL Renderer: ", (const char *)glGetString(GL_RENDERER));
	output.LogEx(kLogNormal, {}, "OpenGL Version: ", (const char *)glGetString(GL_VERSION));
	auto exts = Split((const char*)glGetString(GL_EXTENSIONS), ' ');
	output.Log(exts);


	m_capabilities = flags;


	InitContext();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	glColorMask(true, true, true, true);


	Int32 texturesize = kMaxInt32;

	constexpr GLenum params[] = { GL_MAX_TEXTURE_SIZE, GL_MAX_RENDERBUFFER_SIZE };

	Int32 iinfo = 0;

	for (auto & i : params)
	{
		glGetIntegerv(i, &iinfo);

		texturesize = Min(texturesize, iinfo);
	}

	if (texturesize < 4096) m_capabilities &= ~kCapabilityRenderToTexture;

	m_texturesize = texturesize;



	//allocate fbos

	m_vbos.Init(glGenBuffers, glDeleteBuffers);

	m_textures.Init(glGenTextures, glDeleteTextures);

	if (True(m_capabilities & kCapabilityRenderToTexture))
	{
		glGenFramebuffers(1, &m_fbo_output);

		m_renderbuffers.Allocate(8);
	}
	else
	{
		glBindFramebuffer = &Common::glBindFramebuffer_NULL;	//this is because called in SetCurrent
	}


	if (InitShaders(kVertexShader, kFragmentShader)) return;

	DeinitShaders();

	m_active = true;
}

void OpenGL::Deinit()
{
	GL_CHECK(*this, "OpenGL::Deinit");

	REFLEX_ASSERT(m_active);

	for (auto & i : m_renderbuffers)
	{
		REFLEX_ASSERT(!i.used);

		glDeleteRenderbuffers(1, &i.stencilbuffer);

		glDeleteRenderbuffers(1, &i.colorbuffer);

		glDeleteFramebuffers(1, &i.framebuffer);
	}

	if (m_fbo_output) glDeleteFramebuffers(1, &m_fbo_output);

	m_textures.Flush();

	m_vbos.Flush();

	DeinitShaders();
}

void OpenGL::InitContext()
{
	//if constexpr (kDesktopOpenGL) glEnableClientState(GL_VERTEX_ARRAY);		//was only on mac

	glDisable(GL_DEPTH_TEST);

	glEnable(GL_SCISSOR_TEST);

	glEnable(GL_BLEND);

	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);	//for premult alpha

	//if constexpr (kDesktopOpenGL) glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);	//TODO probably can remove, now we only use shaders
}

void OpenGL::BeginAccess()
{
	m_active = true;
}

void OpenGL::EndAccess()
{
	REFLEX_ASSERT(m_active);

	REFLEX_ASSERT(Not(m_current_canvas));

	m_active = false;

	m_current_canvas = 0;
}

TRef <Renderer::Canvas> OpenGL::CreateBitmap(bool alphachannel, bool antialias)
{
	REFLEX_ASSERT(m_active);

	GL_CHECK(*this, __FUNCTION__);

	UInt8 flags = (m_capabilities & kCapabilityRenderToTexture) | (antialias ? 2 : 0);

	auto fn = CreateBitmapImplBinder::Bind(flags);

	return *fn(*this, alphachannel);
}

TRef <Renderer::Graphic> OpenGL::CreatePrimitives(PrimitiveType type, const ArrayView <fPoint> & points)
{
	GL_CHECK(*this, __FUNCTION__);

	REFLEX_ASSERT(m_active);

	typedef Primitives <fPoint> PointPrimitives;

	return REFLEX_CREATE(PointPrimitives, *this, kPrimitiveTypes[type], points);
}

TRef <Renderer::Graphic> OpenGL::CreatePrimitives(PrimitiveType type, const ArrayView <ColourPoint> & points)
{
	GL_CHECK(*this, __FUNCTION__);

	REFLEX_ASSERT(m_active);

	typedef Primitives <ColourPoint> ColourPointPrimitives;

	return REFLEX_CREATE(ColourPointPrimitives, *this, kPrimitiveTypes[type], points);
}

REFLEX_INLINE Reflex::UInt32 OpenGL::AquireRenderBuffer(UInt32 wp2, UInt32 hp2)
{
	REFLEX_ASSERT(wp2 <= 16384);

	REFLEX_ASSERT(m_active);

	GL_CHECK(*this, __FUNCTION__);


	//reuse

	for (auto & i : m_renderbuffers)
	{
		if (!i.used && wp2 <= i.w && hp2 <= i.h)
		{
			i.used = true;

			glBindFramebuffer(GL_FRAMEBUFFER, i.framebuffer);

			return i.framebuffer;
		}
	}


	//create

	auto & renderbuffer = m_renderbuffers.Push();	// (wp2 * hp2);


	renderbuffer.used = true;

	renderbuffer.w = wp2;

	renderbuffer.h = hp2;


	glGenFramebuffers(1, &renderbuffer.framebuffer);

	glGenRenderbuffers(1, &renderbuffer.colorbuffer);

	glGenRenderbuffers(1, &renderbuffer.stencilbuffer);


	glBindFramebuffer(GL_FRAMEBUFFER, renderbuffer.framebuffer);


	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer.colorbuffer);

	//	if (m_antialias > 1)
	//	{
	//		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_RGBA8, wp2, hp2);
	//	}

	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, wp2, hp2);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer.colorbuffer);


	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer.stencilbuffer);

	//	if (m_antialias > 1)
	//	{
	//		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 2, GL_DEPTH24_STENCIL8, wp2, hp2);
	//	}

	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, wp2, hp2);


	//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.stencilbuffer);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.stencilbuffer);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer.stencilbuffer);


	glBindRenderbuffer(GL_RENDERBUFFER, 0); //!pixel android 15 fix (this one isnt actually needed, but doesnt hurt as this is done rarely)


	return renderbuffer.framebuffer;
}

REFLEX_INLINE void OpenGL::ReleaseRenderBuffer(UInt32 id)
{
	for (auto & i : m_renderbuffers)
	{
		if (i.framebuffer == id)
		{
			i.used = false;

			return;
		}
	}

	REFLEX_ASSERT(0);
}

void OpenGL::EnableBlend(bool enable)
{
	REFLEX_ASSERT(m_active);

	enable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
}

void OpenGL::SetDitheringAmount(Float amount)
{
	REFLEX_ASSERT(m_active);

	m_dither_amount = amount / 255.0f;

	glUniform1f(m_program->g_dither_amount, m_dither_amount);
}

void OpenGL::SetColourTransform(const ArrayView <Float> & matrix_16)
{
	MemCopy(matrix_16.data, m_colour_transform, sizeof(kDefaultColourTransform));

	glUniformMatrix4fv(m_program->g_colour_transform, 1, GL_FALSE, matrix_16.data);
}

void OpenGL::Clear(const Colour & colour)
{
	REFLEX_ASSERT(m_active);

	glClearColor(colour.r, colour.g, colour.b, colour.a);

	glClear(GL_COLOR_BUFFER_BIT);
}

void OpenGL::SetClip(const iRect & rect)
{
	REFLEX_ASSERT(m_active && m_current_canvas);

	auto & canvas = *m_current_canvas;

	auto pixeldens = canvas.m_pixeldens;

	auto x = rect.origin.x * pixeldens;
	auto y = rect.origin.y * pixeldens;
	auto w = Max(rect.size.w * pixeldens, 0);
	auto h = Max(rect.size.h * pixeldens, 0);

	Int32 ys[2] = { y, (canvas.m_size.h * pixeldens) - y - h };

	glScissor(x, ys[canvas.m_internaltype], w, h);
}

void OpenGL::SetMask(const Graphic & graphic, const Transform & transform, bool invert)
{
	GL_CHECK(*this, __FUNCTION__);

	REFLEX_ASSERT(m_active);

	UInt32 all = kMaxUInt32;

	Colour colour;

	glEnable(GL_STENCIL_TEST);

	glColorMask(false, false, false, false);

	glStencilFunc(GL_ALWAYS, 1, all);  // to ensure everything you draw passes

	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	graphic.Render(transform, colour);

	glColorMask(true, true, true, true);

	glStencilFunc(GL_EQUAL, GLint(!invert), all);
}

void OpenGL::ClearMask()
{
	REFLEX_ASSERT(m_active);

	glDisable(GL_STENCIL_TEST);

	glClear(GL_STENCIL_BUFFER_BIT);
}

Renderer::Canvas * OpenGL::GetCurrent()
{
	return m_current_canvas;
}

OpenGL::Config OpenGL::GetConfig() const
{
	Config::Item hd = { kHD, UInt32(True(m_capabilities & kCapabilityHD)) };
	Config::Item tx = { kTX, UInt32(True(m_capabilities & kCapabilityRenderToTexture)) };

	Config config = { hd, tx };

	//if constexpr (kDesktopOpenGL) config.Set(kAA, True(m_capabilities & kCapabilityAA));

	return config;
}

OpenGL::AbstractCanvas::AbstractCanvas(OpenGL & engine, UInt internaltype)
	: engine(engine)
{
	m_internaltype = internaltype;

	Retain(engine);
}

OpenGL::AbstractCanvas::~AbstractCanvas()
{
	REFLEX_ASSERT(engine.m_active);

	REFLEX_ASSERT(engine.m_current_canvas != this);

	if (engine.m_current_canvas == this) engine.m_current_canvas = 0;

	Release(engine);
}

void OpenGL::AbstractCanvas::SetCurrent()
{
	GL_CHECK(engine, __FUNCTION__);

	REFLEX_ASSERT(engine.m_active);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	ApplySetCurrent<true>(m_size);
}

void OpenGL::AbstractCanvas::Flush()
{
	REFLEX_ASSERT(engine.m_active);	//opengl used in derived class

	engine.m_current_canvas = nullptr;
}

template <bool WINDOW> REFLEX_INLINE void OpenGL::AbstractCanvas::ApplySetCurrent(const iSize & size)
{
	GL_CHECK(engine, __FUNCTION__);

	REFLEX_ASSERT(engine.m_active);

	engine.m_current_canvas = this;

	{
		auto pixw = size.w * m_pixeldens;
		auto pixh = size.h * m_pixeldens;

		glViewport(0, 0, pixw, pixh);

		Float32 w = Float32(size.w);

		Float32 h = Float32(size.h);

		fPoint origin = { -(w * 0.5f), -(h * 0.5f) };

		fPoint scale = { w * 0.5f, h * (WINDOW ? -0.5f : 0.5f) };

		auto programs = engine.m_programs;

		auto program_idx = UInt(engine.m_program - programs);

		REFLEX_LOOP(idx, GetArraySize(engine.m_programs))
		{
			auto & program = programs[++program_idx & 1];

			glUseProgram(program.id);

			glUniform2fv(program.g_viewport_origin, 1, &origin.x);

			glUniform2fv(program.g_viewport_size, 1, &scale.x);
		}

		glScissor(0, 0, pixw, pixh);
	}
}

template <bool WINDOW> REFLEX_INLINE void OpenGL::AbstractCanvas::ApplySetClip(Int32 x, Int32 y, Int32 w, Int32 h)
{
	REFLEX_ASSERT(engine.m_current_canvas == this && engine.m_active);
	REFLEX_ASSERT(x >= 0 && y >= 0 && w <= m_size.w && h <= m_size.h);

	x *= m_pixeldens;
	y *= m_pixeldens;
	w = Max(w * m_pixeldens, 0);
	h = Max(h * m_pixeldens, 0);

	if constexpr (WINDOW)
	{
		glScissor(x, (m_size.h * m_pixeldens) - y - h, w, h);
	}
	else
	{
		glScissor(x, y, w, h);
	}
}

OpenGL::AbstractBitmap::AbstractBitmap(OpenGL & engine)
	: AbstractCanvas(engine, 0)
	, m_textureid(engine.m_textures.Generate())
{
	REFLEX_ASSERT(engine.m_active);

	m_size_pow2 = { 1, 1 };
}

OpenGL::AbstractBitmap::~AbstractBitmap()
{
	REFLEX_ASSERT(engine.m_active);

	engine.m_textures.Delete(RemoveConst(m_textureid));
}

template <bool RENDER_TARGET, bool MIPMAP> OpenGL::Bitmap<RENDER_TARGET,MIPMAP>::Bitmap(OpenGL & engine, bool alphachannel)
	: AbstractBitmap(engine)
	, m_internalformat(alphachannel ? GL_RGBA : GL_RGB)
	, m_fboid(0)
{
}

template <bool RENDER_TARGET, bool MIPMAP> OpenGL::Bitmap<RENDER_TARGET,MIPMAP>::~Bitmap()
{
	GL_CHECK(engine, __FUNCTION__);

	REFLEX_ASSERT(engine.m_active);

	REFLEX_ASSERT(!m_fboid);
}

template <bool RENDER_TARGET, bool MIPMAP> bool OpenGL::Bitmap<RENDER_TARGET,MIPMAP>::SetSize(const iSize & size, Int32 pixdensity)
{
	GL_CHECK(engine, "WritableBitmap::SetSize");

	REFLEX_ASSERT(engine.m_active);

	if constexpr (RENDER_TARGET)
	{
		REFLEX_ASSERT(!m_fboid);

		if (m_fboid)
		{
			engine.ReleaseRenderBuffer(m_fboid);

			m_fboid = 0;
		}
	}

	CanvasBase::SetSize(size, pixdensity);


	Int32 pixw = RoundUpPow2(m_size.w * pixdensity, 2);

	Int32 pixh = RoundUpPow2(m_size.h * pixdensity, 2);

	m_size_pow2 = { pixw / pixdensity, pixh / pixdensity };


	void * ptr = 0;

	if (m_internalformat == GL_RGBA && (m_size_pow2.w != m_size.w || m_size_pow2.h != m_size.h))
	{
		auto & buffer = engine.m_rgba_buffer;

		UInt npixel = pixw * pixh;

		if (buffer.GetSize() < npixel)
		{
			buffer.SetSize(npixel);

			buffer.Fill(0ul);	//rgba(255,255,255,0)
		}

		ptr = buffer.GetData();
	}

	glBindTexture(GL_TEXTURE_2D, m_textureid);

	glTexImage2D(GL_TEXTURE_2D, 0, m_internalformat, pixw, pixh, 0, m_internalformat, GL_UNSIGNED_BYTE, ptr);	//GL_UNSIGNED_BYTE

	if constexpr (MIPMAP)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);//GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_NEAREST

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	return true;
}

template <bool RENDER_TARGET, bool MIPMAP> void OpenGL::Bitmap<RENDER_TARGET,MIPMAP>::Write(ImageFormat format, const ArrayView <UInt8> & bytes)
{
	GL_CHECK(engine, __FUNCTION__);

	REFLEX_ASSERT(engine.m_active);

	REFLEX_ASSERT(engine.SupportsImageFormat(format));

	if (VerifyBitmap({ m_size, m_pixeldens, format }, bytes))
	{
		glBindTexture(GL_TEXTURE_2D, m_textureid);

		UInt32 w = m_size.w * m_pixeldens;

		UInt32 h = m_size.h * m_pixeldens;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);	//should be global, but there are driver bugs on OSX

#ifdef GL_TEXTURE_SWIZZLE_A
		if (format == kImageFormatLuminance)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_ONE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
		}
#endif

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, kImageFormats[format], GL_UNSIGNED_BYTE, bytes.data);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		if constexpr (MIPMAP)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);//GL_LINEAR_MIPMAP_LINEAR

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glGenerateMipmap(GL_TEXTURE_2D);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}

		(*glFlushRender)();
	}
}

template <bool RENDER_TARGET, bool MIPMAP> TRef <Renderer::Graphic> OpenGL::Bitmap<RENDER_TARGET,MIPMAP>::CreateTextures(const ArrayView < Pair <fRect> > & rects) const
{
	GL_CHECK(engine, __FUNCTION__);

	return New<Textures>(*this, rects);
}

template <bool RENDER_TARGET, bool MIPMAP> TRef <Renderer::Graphic> OpenGL::Bitmap<RENDER_TARGET, MIPMAP>::CreateTexturesWithFilter(const ArrayView < Pair <fRect> >& rects, FilterMode mode, const ArrayView <Float>& parameters) const
{
	GL_CHECK(engine, __FUNCTION__);

	return New<TexturesWithFilter>(*this, rects, mode, parameters);
}

template <bool RENDER_TARGET, bool MIPMAP> void OpenGL::Bitmap<RENDER_TARGET,MIPMAP>::SetCurrent()
{
	REFLEX_ASSERT(engine.m_active);

	GL_CHECK(engine, __FUNCTION__);

	if constexpr (RENDER_TARGET)
	{
		if (m_fboid)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_fboid);
		}
		else
		{
			m_fboid = engine.AquireRenderBuffer(Min<UInt32>(m_size_pow2.w * m_pixeldens, 16384), Min<UInt32>(m_size_pow2.h * m_pixeldens, 16384));
		}
	}

	REFLEX_ASSERT(m_size_pow2.w * m_pixeldens <= 16384);
	REFLEX_ASSERT(m_size_pow2.h * m_pixeldens <= 16384);

	ApplySetCurrent<false>(m_size_pow2);
}

template <bool RENDER_TARGET, bool MIPMAP> void OpenGL::Bitmap<RENDER_TARGET,MIPMAP>::Flush()
{
	REFLEX_ASSERT(engine.m_active);

	REFLEX_ASSERT(engine.m_current_canvas == this);

	GL_CHECK(engine, __FUNCTION__);

	if constexpr (RENDER_TARGET)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, engine.m_fbo_output);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_textureid, 0);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fboid);

		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, engine.m_fbo_output);

		(*glFlushRender)();

		UInt32 w = m_size.w * m_pixeldens;

		UInt32 h = m_size.h * m_pixeldens;

		glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);

		engine.ReleaseRenderBuffer(m_fboid);

		m_fboid = 0;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);		//!pixel android 15 magic fix
	}

	glBindTexture(GL_TEXTURE_2D, m_textureid);

	if constexpr (MIPMAP) glGenerateMipmap(GL_TEXTURE_2D);

	(*glFlushRender)();

	engine.m_current_canvas = nullptr;
}

template <class POINT> const System::Colour OpenGL::Primitives<POINT>::kWhite = { 1.0f, 1.0f, 1.0f, 1.0f };

template <class POINT> REFLEX_INLINE OpenGL::Primitives<POINT>::Primitives(OpenGL & engine, GLenum mode, const ArrayView <POINT> & points)
	: engine(engine)
	, m_nvertex(points.size)
	, m_mode(mode)
{
	Retain(engine);

	REFLEX_ASSERT(engine.m_active);

	m_vboid = engine.m_vbos.Generate();

	glBindBuffer(GL_ARRAY_BUFFER, m_vboid);

	glBufferData(GL_ARRAY_BUFFER, m_nvertex * sizeof(POINT), points.data, GL_STATIC_DRAW);
}

template <class POINT> OpenGL::Primitives<POINT>::~Primitives()
{
	REFLEX_ASSERT(engine.m_active);

	engine.m_vbos.Delete(m_vboid);

	Release(engine);
}

template <class POINT> void OpenGL::Primitives<POINT>::Render(const Transform & transform, const Colour & colour) const
{
	REFLEX_ASSERT(engine.m_active);

	engine.SelectProgram(0);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboid);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(POINT), 0);

	if constexpr (IsType<POINT,ColourPoint>::value)
	{
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(POINT), (void*)(sizeof(fPoint)));

		glEnableVertexAttribArray(1);
	}
	else
	{
		glVertexAttrib4fv(1, &kWhite.r);
	}

	engine.SetTransform(transform, colour);

	glDrawArrays(m_mode, 0, m_nvertex);

	if constexpr (IsType<POINT,ColourPoint>::value)
	{
		glDisableVertexAttribArray(1);
	}
}

OpenGL::Textures::Textures(const AbstractBitmap & bitmap, const ArrayView < Pair <fRect> > & rects)
	: bitmap(bitmap)
	, m_vboid(0)
	, m_nvertex(rects.size * 6)
{
	Retain(bitmap);

	auto & engine = RemoveConst(bitmap.engine);

	GL_CHECK(engine, __FUNCTION__);

	REFLEX_ASSERT(engine.m_active);

	auto size_pow2 = bitmap.m_size_pow2;

	Float32 fw = Reciprocal(Float32(size_pow2.w));
	Float32 fh = Reciprocal(Float32(size_pow2.h));

	auto & v = engine.m_points_buffer;

	auto & tc = v;

	v.Clear();

	for (auto & rect : rects)
	{
		auto & src = rect.a;

		auto & src_origin = src.origin;

		auto & src_size = src.size;

		auto & dst = rect.b;

		auto & dst_origin = dst.origin;

		auto & dst_size = dst.size;

		Float32 src_x1 = Float32(src_origin.x) * fw;
		Float32 src_y1 = Float32(src_origin.y) * fh;
		Float32 src_x2 = Float32(src_origin.x + src_size.w) * fw;
		Float32 src_y2 = Float32(src_origin.y + src_size.h) * fh;

		Float32 dst_x1 = dst_origin.x;
		Float32 dst_x2 = dst_x1 + dst_size.w;
		Float32 dst_y1 = dst_origin.y;
		Float32 dst_y2 = dst_y1 + dst_size.h;

		v.Push({ dst_x1, dst_y1 });
		tc.Push({ src_x1, src_y1 });

		v.Push({ dst_x2, dst_y1 });
		tc.Push({ src_x2, src_y1 });

		v.Push({ dst_x1, dst_y2 });
		tc.Push({ src_x1, src_y2 });

		v.Push({ dst_x1, dst_y2 });
		tc.Push({ src_x1, src_y2 });

		v.Push({ dst_x2, dst_y2 });
		tc.Push({ src_x2, src_y2 });

		v.Push({ dst_x2, dst_y1 });
		tc.Push({ src_x2, src_y1 });
	}

	m_vboid = engine.m_vbos.Generate();

	glBindBuffer(GL_ARRAY_BUFFER, m_vboid);

	glBufferData(GL_ARRAY_BUFFER, m_nvertex * 16, tc.GetData(), GL_STATIC_DRAW);
}

OpenGL::Textures::~Textures()
{
	auto & engine = bitmap.engine;

	GL_CHECK(engine, __FUNCTION__);

	REFLEX_ASSERT(engine.m_active);

	engine.m_vbos.Delete(m_vboid);

	Release(bitmap);
}

void OpenGL::Textures::Render(const Transform & transform, const Colour & colour) const
{
	auto & engine = bitmap.engine;

	//GL_CHECK(engine, __FUNCTION__);

	REFLEX_ASSERT(engine.m_active);

	engine.SelectProgram(1);

	engine.SetTransform(transform, colour);

	glBindTexture(GL_TEXTURE_2D, bitmap.m_textureid);

	glBindBuffer(GL_ARRAY_BUFFER, m_vboid);

	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, 0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, (void*)(8));

	glDrawArrays(GL_TRIANGLES, 0, m_nvertex);

	glDisableVertexAttribArray(1);
}


OpenGL::TexturesWithFilter::TexturesWithFilter(const Reflex::System::Common::OpenGL::AbstractBitmap &bitmap, const ArrayView<Pair<Reflex::System::fRect>> &rects, FilterMode mode, const ArrayView <Float>& in_params)
	: OpenGL::Textures(bitmap, rects)
	, m_filtermode(mode)
{
	auto widthUnits = Float(bitmap.m_size_pow2.w);
	auto heightUnits = Float(bitmap.m_size_pow2.h);

	switch (mode) {
		case kFilterModePixelate:
			REFLEX_ASSERT(in_params.size == 2);
			m_filterparams[0] = in_params[0] / widthUnits;
			m_filterparams[1] = in_params[1] / heightUnits;
			break;

		case kFilterModeBlur: {
			// out_params => direction.xy: float[2], radius: float, gaussian_weights: float[radius + 1]
			REFLEX_ASSERT(in_params.size >= 4 && in_params.size <= 20);

			m_filterparams[0] = in_params[0] / widthUnits;
			m_filterparams[1] = in_params[1] / heightUnits;
			m_filterparams[2] = in_params[2];

			auto gaussian_weights = Mid(in_params, 3);
			REFLEX_IF_DEBUG(auto tblsize = UInt(in_params[2]) + 1);
			REFLEX_ASSERT(gaussian_weights.size == tblsize);

			MemCopy(gaussian_weights.data, m_filterparams + 3, gaussian_weights.size * sizeof(m_filterparams[0]));
			break;
		}

		default:
		DEV_ERROR("Unsupported filter type");
			return;
	}
}

void OpenGL::TexturesWithFilter::Render(const Transform & transform, const Colour & colour) const
{
	auto & engine = bitmap.engine;

	REFLEX_ASSERT(engine.m_active);

	engine.SelectProgram(1);

	auto & program = *engine.m_program;

	glUniform1i(program.g_filtermode, m_filtermode);

	glUniform4fv(program.g_filterparams, 5, m_filterparams);

	Textures::Render(transform, colour);

	glUniform1i(program.g_filtermode, kFilterModeNone);
}


REFLEX_INLINE OpenGL::Check::Check(const OpenGL & engine, const char * block)
	: engine(engine)
	, m_block(block)
{
	Try(m_block);
}

REFLEX_INLINE OpenGL::Check::~Check()
{
	Try(m_block);
}

REFLEX_INLINE bool OpenGL::Check::Try(const char * msg)
{
	GLenum error = glGetError();

	if (error != GL_NO_ERROR)
	{
		ShowError(error);

		return false;
	}

	return true;
}

REFLEX_NOINLINE void OpenGL::Check::ShowError(GLenum error)
{
	CString::View errorcode;

	switch (error)
	{
	case GL_INVALID_ENUM:
		errorcode = "GL_INVALID_ENUM";
		break;

	case GL_INVALID_VALUE:
		errorcode = "GL_INVALID_VALUE";
		break;

	case GL_INVALID_OPERATION:
		errorcode = "GL_INVALID_OPERATION";
		break;

	case GL_STACK_OVERFLOW:
		errorcode = "GL_STACK_OVERFLOW";
		break;

	case GL_STACK_UNDERFLOW:
		errorcode = "GL_STACK_UNDERFLOW";
		break;

	case GL_OUT_OF_MEMORY:
		errorcode = "GL_OUT_OF_MEMORY";
		break;

	case GL_INVALID_FRAMEBUFFER_OPERATION:
		errorcode = "GL_INVALID_FRAMEBUFFER_OPERATION";
		break;

	case 0x0507:
		errorcode = "GL_CONTEXT_LOST";
		break;

	case 0x8031:
		errorcode = "GL_TABLE_TOO_LARGE1";
		break;

	default:
		break;
	}

	Common::output.Warn(m_block, errorcode);
}

GLint OpenGL::CreateShader(GLenum type, const ArrayView <UInt8> & archive, bool tx)
{
	static constexpr auto Pack = [](const CString::View & string) -> ArrayView <UInt8>
	{
		return { Reinterpret<UInt8>(string.data), string.size };
	};

	static constexpr auto AddDefine = [](Array <UInt8> & shader, const CString::View & string, bool value)
	{
		shader.Append(Join(Pack("#define "), Pack(string), UInt8(' '), value ? UInt8('1') : UInt8('0'), UInt8(10)));
	};

	Array <UInt8> shader;

	if constexpr (kDesktopOpenGL)
	{
		shader.Append(Pack("#version 120"));
	}
	else
	{
		shader.Append(Pack("#version 300 es"));
	}

	shader.Push(UInt8(10));

	AddDefine(shader, "ES", !kDesktopOpenGL);

	AddDefine(shader, "TX", tx);

	shader.Append(archive);

	auto id = glCreateShader(type);

	const GLchar * source[] = { Reinterpret<GLchar>(shader.GetData()) };

	GLint lengths[] = { GLint(shader.GetSize()) };

	glShaderSource(id, 1, source, lengths);

	glCompileShader(id);

	REFLEX_ASSERT(CheckShader(id, type == GL_VERTEX_SHADER ? ToView(L"Vertex") : ToView(L"Fragment")));

	//const WChar * filename[2][2] = { { L"D:/vertex_shader.txt", L"D:/fragment_shader.txt" }, { L"D:/vertex_shader_tx.txt", L"D:/fragment_shader_tx.txt" } };

	//auto file = AutoRelease(File::Open(filename[type == GL_FRAGMENT_SHADER][tx], File::kModeOverwrite));

	//file->Write(shader.GetData(), shader.GetSize());

	return id;
}

bool OpenGL::CheckShader(GLint id, const WString::View & path)
{
	bool pg = !path;

	GLint result;

	auto glGetiv = (pg ? glGetProgramiv : glGetShaderiv);

	glGetiv(id, pg ? GL_LINK_STATUS : GL_COMPILE_STATUS, &result);

	if (result == GL_FALSE)
	{
		GLint length;

		glGetiv(id, GL_INFO_LOG_LENGTH, &length);

		Array <GLchar> buffer(length + 1);

		(pg ? glGetProgramInfoLog : glGetShaderInfoLog)(id, length, &length, buffer.GetData());

		return false;
	}

	return true;
}

bool OpenGL::InitShaders(const ArrayView <UInt8> & vertex_shader, const ArrayView <UInt8> & fragment_shader)
{
	GL_CHECK(*this, __FUNCTION__);

	const char * attribs[2] = { "colour", "texcoord" };

	bool ok = true;

	REFLEX_LOOP(idx, 2)
	{
		auto & program = m_programs[idx];

		program.id = glCreateProgram();

		//auto options = optionss[idx];

		bool tx = True(idx);

		GLint vertex = CreateShader(GL_VERTEX_SHADER, vertex_shader, tx);

		GLint fragment = CreateShader(GL_FRAGMENT_SHADER, fragment_shader, tx);

		glAttachShader(program.id, vertex);
		glAttachShader(program.id, fragment);

		glBindAttribLocation(program.id, 0, "position");
		glBindAttribLocation(program.id, 1, attribs[idx]);

		glLinkProgram(program.id);

		program.g_viewport_origin = glGetUniformLocation(program.id, "g_viewport_origin");
		program.g_viewport_size = glGetUniformLocation(program.id, "g_viewport_size");
		program.g_offset = glGetUniformLocation(program.id, "g_offset");
		program.g_scale = glGetUniformLocation(program.id, "g_scale");
		program.g_colour = glGetUniformLocation(program.id, "g_colour");
		program.g_colour_transform = glGetUniformLocation(program.id, "g_colourTransform");
		program.g_dither_amount = glGetUniformLocation(program.id, "g_dither_amount");
		program.g_filtermode = glGetUniformLocation(program.id, "g_filtermode");
		program.g_filterparams = glGetUniformLocation(program.id, "g_filterparams");

		ok = ok && CheckShader(program.id, L"");

		glDeleteShader(vertex);
		glDeleteShader(fragment);

		// Default values
		glUniformMatrix4fv(program.g_colour_transform, 1, GL_FALSE, kDefaultColourTransform);
		glUniform1f(program.g_dither_amount, 0.0f);
		glUniform1i(program.g_filtermode, kFilterModeNone);
	}

	m_program = m_programs;

	glUseProgram(m_program->id);

	glEnableVertexAttribArray(0);

	return ok;
}

void OpenGL::DeinitShaders()
{
	REFLEX_ASSERT(m_active);

	glDisableVertexAttribArray(0);

	for (auto & i : m_programs) glDeleteProgram(i.id);
}

REFLEX_INLINE void OpenGL::SelectProgram(UInt idx)
{
	REFLEX_ASSERT(m_active);

	auto program = m_programs + idx;

	if (SetFiltered(m_program, program))
	{
		glUseProgram(program->id);

		glEnableVertexAttribArray(0);	//bug on mac, not global state

		glUniformMatrix4fv(program->g_colour_transform, 1, GL_FALSE, m_colour_transform);
		glUniform1f(program->g_dither_amount, m_dither_amount);
	}
}

REFLEX_INLINE void OpenGL::SetTransform(const Transform & transform, const Colour & colour)
{
	REFLEX_ASSERT(m_active);

	auto & origin = transform.origin;

	auto & scale = transform.scale;

	Colour c = colour;

	c.a *= transform.opacity;

	auto & program = *m_program;

	glUniform2fv(program.g_offset, 1, &origin.x);

	glUniform2fv(program.g_scale, 1, &scale.w);

	glUniform4fv(program.g_colour, 1, &c.r);
}

#undef GL_CHECK

REFLEX_END
