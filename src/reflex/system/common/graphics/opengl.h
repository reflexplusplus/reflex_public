#pragma once

#include "graphics.h"




//
//preprocessor

#if defined(REFLEX_OPENGL_ES)
REFLEX_STATIC_ASSERT(REFLEX_OPENGL_ES)
#else
#define REFLEX_OPENGL_ES false
#endif

#ifndef STDCALL
#define STDCALL
#endif

#define REFLEX_OPENGL_API(RTN, NAME, ...) static inline RTN (STDCALL *NAME)(__VA_ARGS__) = 0;




//
//declarations

REFLEX_NS(Reflex::System::Common)

class OpenGL;

REFLEX_END




//
//Common::OpenGL

class Reflex::System::Common::OpenGL : public Renderer
{
public:

	static constexpr CString::View kEngineName = "OpenGL";

	static constexpr bool kDesktopOpenGL = !REFLEX_OPENGL_ES;

	enum Capabilities
	{
		kCapabilityRenderToTexture = 1,
		kCapabilityAA = 2,
		kCapabilityHD = 4
	};

	class AbstractCanvas;

	class Check;



protected:

	REFLEX_OPENGL_API(void, glFlushRender);

	REFLEX_OPENGL_API(void, glGenBuffers, GLsizei n, GLuint *buffers);
	REFLEX_OPENGL_API(void, glDeleteBuffers, GLsizei n, const GLuint *buffers);
	REFLEX_OPENGL_API(void, glBindBuffer, GLenum target, GLuint buffer);
	REFLEX_OPENGL_API(void, glBufferData, GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
	REFLEX_OPENGL_API(void, glBufferSubData, GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);

	REFLEX_OPENGL_API(GLuint, glCreateProgram);
	REFLEX_OPENGL_API(void, glDeleteProgram, GLuint);
	REFLEX_OPENGL_API(GLuint, glCreateShader, GLenum);
	REFLEX_OPENGL_API(void, glDeleteShader, GLuint);
	REFLEX_OPENGL_API(void, glShaderSource, GLuint, GLsizei, const GLchar * const *, const GLint *);
	REFLEX_OPENGL_API(void, glCompileShader, GLuint);
	REFLEX_OPENGL_API(void, glAttachShader, GLuint, GLuint);
	REFLEX_OPENGL_API(void, glLinkProgram, GLuint);
	REFLEX_OPENGL_API(void, glGetProgramiv, GLuint, GLenum, GLint *);
	REFLEX_OPENGL_API(void, glGetProgramInfoLog, GLuint, GLsizei, GLsizei *, GLchar *);
	REFLEX_OPENGL_API(void, glGetShaderiv, GLuint, GLenum, GLint *);
	REFLEX_OPENGL_API(void, glGetShaderInfoLog, GLuint, GLsizei, GLsizei *, GLchar *);
	REFLEX_OPENGL_API(void, glUseProgram, GLuint);
	REFLEX_OPENGL_API(GLint, glGetUniformLocation, GLuint, const GLchar *);
	REFLEX_OPENGL_API(void, glUniform1f, GLint, GLfloat);
	REFLEX_OPENGL_API(void, glUniform1i, GLint, GLint);
	REFLEX_OPENGL_API(void, glUniform2fv, GLint, GLsizei, const GLfloat*);
	REFLEX_OPENGL_API(void, glUniform4fv, GLint, GLsizei, const GLfloat*);
	REFLEX_OPENGL_API(void, glUniformMatrix4fv, GLint, GLsizei, GLboolean, const GLfloat *);
	REFLEX_OPENGL_API(void, glEnableVertexAttribArray, GLuint);
	REFLEX_OPENGL_API(void, glDisableVertexAttribArray, GLuint);
	REFLEX_OPENGL_API(void, glVertexAttrib4fv, GLuint, const GLfloat *);
	REFLEX_OPENGL_API(void, glVertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid *);
	REFLEX_OPENGL_API(void, glBindAttribLocation, GLuint program, GLuint index, const GLchar *name);

	//V3.0

	REFLEX_OPENGL_API(void, glGenerateMipmap, GLenum target);
	REFLEX_OPENGL_API(void, glGenFramebuffers, GLsizei n, GLuint *framebuffers);
	REFLEX_OPENGL_API(void, glDeleteFramebuffers, GLsizei n, const GLuint *framebuffers);
	REFLEX_OPENGL_API(void, glBindFramebuffer, GLenum target, GLuint framebuffer);
	REFLEX_OPENGL_API(void, glFramebufferTexture2D, GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	REFLEX_OPENGL_API(void, glBlitFramebuffer, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
	REFLEX_OPENGL_API(GLenum, glCheckFramebufferStatus, GLenum target);
	REFLEX_OPENGL_API(void, glGenRenderbuffers, GLsizei n, GLuint *renderbuffers);
	REFLEX_OPENGL_API(void, glDeleteRenderbuffers, GLsizei n, const GLuint *framebuffers);
	REFLEX_OPENGL_API(void, glBindRenderbuffer, GLenum target, GLuint buffer);
	REFLEX_OPENGL_API(void, glRenderbufferStorage, GLenum target, GLenum internalformat, GLsizei w, GLsizei h);
	//REFLEX_OPENGL_API(void, glRenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei w, GLsizei h);
	REFLEX_OPENGL_API(void, glFramebufferRenderbuffer, GLenum target, GLenum attachment, GLenum rbt, GLuint rbo);



	//static info

	static Float32 GetVersion();

	static bool CheckExtension(const CString::View & extension);



	//lifetime

	OpenGL();

	void Init(UInt8 flags);

	void Deinit();

	static void InitContext();		//special case for macOS, was known to need this after setcontext



	//access

	virtual void CorrectStrokes(WString & path) = 0;

	virtual void BeginAccess() override;

	virtual void EndAccess() override;



private:

	struct AbstractBitmap;

	template <class POINT> struct Primitives;

	template <bool RENDER_TARGET, bool MIPMAP> struct Bitmap;

	struct Textures;

	struct TexturesWithFilter;


	CString::View GetEngineName() const override { return kEngineName; }

	Config GetConfig() const override;


	TRef <Canvas> CreateBitmap(bool alphachannel, bool antialias) override;

	TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <fPoint> & points) override;

	TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <ColourPoint> & points) override;


	void EnableBlend(bool enable) override;

	void SetDitheringAmount(Float amount) override;

	void SetColourTransform(const ArrayView <Float> & m) override;

	void Clear(const Colour & color) override;

	void SetClip(const iRect & irect) override;

	void SetMask(const Graphic & graphic, const Transform & transform, bool invert) override;

	void ClearMask() override;


	Canvas * GetCurrent() override;



	//buffers

	UInt32 AquireRenderBuffer(UInt32 wp2, UInt32 hp2);

	void ReleaseRenderBuffer(UInt32 rbid);

	template <bool RB, bool AA> static Canvas * CreateBitmapImpl(OpenGL & engine, bool alphachannel);

	REFLEX_TBINDER_2P(CreateBitmapImpl);



	//shaders

	bool InitShaders(const ArrayView <UInt8> & vertex_shader, const ArrayView <UInt8> & fragment_shader);

	void DeinitShaders();

	static GLint CreateShader(GLenum type, const ArrayView <UInt8> & archive, bool tx);

	static bool CheckShader(GLint id, const WString::View & path);

	void SelectProgram(UInt idx);

	void SetTransform(const Transform & transform, const Colour & colour);



	//config

	UInt8 m_capabilities;

	Int32 m_texturesize;



	//shader state

	struct Program
	{
		GLuint id;

		GLint g_viewport_origin, g_viewport_size;

		GLint g_offset, g_scale, g_colour, g_colour_transform, g_dither_amount, g_filtermode, g_filterparams;
	};

	Program m_programs[2];

	Program * m_program;

	Float m_colour_transform[16];
	Float m_dither_amount;



	//state

	struct Objects
	{
		~Objects()
		{
			REFLEX_ASSERT(!m_used);

			Flush();
		}

		void Init(decltype(glGenBuffers) gen, decltype(glDeleteBuffers) del)
		{
			m_generate = gen;

			m_delete = del;

			m_free.Allocate(1024);
		}

		GLuint Generate()
		{
			GLuint id;

			if (m_free)
			{
				id = m_free.GetLast();

				m_free.Pop();
			}
			else
			{
				m_generate(1, &id);

				REFLEX_ASSERT(id);
			}

			if (REFLEX_DEBUG) { m_used.Push(id); }

			return id;
		}

		void Delete(GLuint & id)
		{
			REFLEX_ASSERT(id);

			REFLEX_ASSERT(Search(m_used, id));

			if (REFLEX_DEBUG) { Remove(m_used, id); }

			m_free.Push(id);

			id = 0;
		}

		void Flush()
		{
			if (auto free_size = m_free.GetSize())
			{
				m_delete(free_size, m_free.GetData());
			}

			m_free.Clear();
		}

		decltype(glGenBuffers) m_generate = 0;

		decltype(glDeleteBuffers) m_delete = 0;

		Array <GLuint> m_free, m_used;
	};

	Objects m_vbos;


	struct RenderBuffer
	{
		UInt32 w, h;

		GLuint framebuffer;

		GLuint colorbuffer;

		GLuint stencilbuffer;

		bool used;
	};

	GLuint m_fbo_output;

	Array <RenderBuffer> m_renderbuffers;


	Objects m_textures;



	//render state

	bool m_active;

	AbstractCanvas * m_current_canvas;


	//temp

	Array <fPoint> m_points_buffer;

	Array <UInt32> m_rgba_buffer;


	static Float32 st_matrixcache[16];

	static const GLenum kPrimitiveTypes[kNumPrimitiveType];

	static const UInt32 kImageFormats[kNumImageFormat];

	static constexpr Float kDefaultColourTransform[16] =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};
};

class Reflex::System::Common::OpenGL::AbstractCanvas : public Common::CanvasBase
{
public:

	AbstractCanvas(OpenGL & engine, UInt internaltype);

	~AbstractCanvas();


	void SetCurrent() override;

	void Flush() override;

	TRef <Graphic> CreateTextures(const ArrayView < Pair <fRect> >& rects) const override { return Graphic::null; }

	TRef <Graphic> CreateTexturesWithFilter(const ArrayView < Pair <fRect> > & rects, FilterMode mode, const ArrayView <Float>& parameters) const override { return Graphic::null; }


	template <bool WINDOW> void ApplySetCurrent(const iSize & size);

	template <bool WINDOW> void ApplySetClip(Int32 x, Int32 y, Int32 w, Int32 h);


	OpenGL & engine;

};
