#pragma once

#include "graphics.h"
#import <MetalKit/MetalKit.h>




// MARK: Declarations
REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

template<class> inline constexpr bool always_false_v = false;

///
/// Generic renderer for Metal (GPU driver for Apple environments).
/// Renders to an UIView, adding a CAMetalLayer, should work for both fullscreen and a window (although it hasn't been tested).
///
struct Metal : public Renderer
{
	static constexpr MTLPixelFormat kColorPixelFormat = MTLPixelFormatBGRA8Unorm; // MTLPixelFormatRGBA8Unorm unsupported on iOS <= 15.5
	static constexpr MTLPixelFormat kStencilPixelFormat = MTLPixelFormatStencil8;
	static constexpr UInt kNumSamplesInMultisampleMode = 4;
	static constexpr CString::View kEngineName = "Metal";

	struct ScreenCanvas;

	// lifetime
	Metal(bool dpiAware, bool useAntialiasing, bool allowRenderToTexture);

	Metal(const Metal&) = delete;
	Metal& operator = (const Metal&) = delete;

	CString::View GetEngineName() const override { return kEngineName; }
	bool Status() const override { return m_status; }
	Config GetConfig() const override;
	bool SupportsImageFormat(ImageFormat format) const override;


	// access (must set before use)
	void BeginAccess() override;
	void EndAccess() override;


	// Canvas (render targets)
	TRef<Canvas> CreateCanvas(void* systemWindowPtr) override = 0;
	TRef<Canvas> CreateBitmap(bool alphachannel, bool antialias) override;


	// Graphics
	TRef<Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView<fPoint>& points) override;
	TRef<Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView<ColourPoint>& points) override;


	// render (a valid Canvas must be set as current)
	void Clear(const Colour& color) override;
	void EnableBlend(bool enable) override;
	void SetDitheringAmount(Float amount) override;
	void SetColourTransform(const ArrayView <Float> & m) override;
	void SetClip(const iRect& rect) override;
	void SetMask(const Graphic& mask, const Transform& transform, bool invert) override;
	void ClearMask() override;

	// info
	Canvas* GetCurrent() override;


	id<MTLDevice> GetMetalDevice() const { return m_mtlDevice; }

	bool IsDpiAware() const { return m_dpiAware; }


private:

	template<class VertexType> struct NativePrimitives;
	struct NativeTexturedPrimitives;
	struct NativePrimitivesWithFilter;

	struct VertexUniforms;
	struct FragmentUniforms;

	struct Renderable;
	struct CommonCanvas;
	struct Bitmap;
	struct RenderableBitmap;

	enum PipelineType 
	{
		kPipelineTypeVertexPoint,
		kPipelineTypeVertexColorPoint,
		kPipelineTypeTextureRGBA,
		kPipelineTypeTextureLuminance,

		kNumPipelineType
	};

	template<class DataType>
	struct MetalBuffer 
	{
		MetalBuffer() {}

		void Create(Metal& renderer, const ArrayView <DataType> & data);

		operator bool () const { return m_buffer; }

		operator id<MTLBuffer> () const { return m_buffer; }

		ObjCRef < id<MTLBuffer> > m_buffer;
	};

	struct PipelineConfig 
	{
		ObjCRef <MTLVertexDescriptor*> vertexDescriptor;
		ObjCRef < id<MTLFunction> > vertFunc, fragFunc;
	};


	ObjCRef < id<MTLLibrary> > LoadShaderLibrary();

	template <class VertexType> ObjCRef <MTLVertexDescriptor*> CreateVertexDescriptor();

	ObjCRef < id<MTLSamplerState> > CreateSamplerState(bool forAntialiased);


	ObjCRef < id<MTLRenderPipelineState> > GetRenderPipeline(PipelineType pipelineType, UInt pixelFormatIndex);

	ObjCRef < id<MTLRenderPipelineState> > CreateRenderPipeline(PipelineConfig & config, bool blendingEnabled, MTLPixelFormat pixelFormat, MTLPixelFormat stencilPixelFormat, bool skipDrawingPixels, bool useAntialiasing);


	static void GenerateMipMaps(id<MTLCommandBuffer> commandBuffer, id<MTLTexture> texture);

	static MTLScissorRect ToMTLScissorRect(const iRect& rect, int dpifactor);


	const bool m_dpiAware, m_allowRenderToTexture;
	bool m_antialiasingEnabled;
	bool m_status = false;


	ObjCRef < id<MTLDevice> > m_mtlDevice;
	ObjCRef < id<MTLLibrary> > m_library;
	ObjCRef < id<MTLSamplerState> > m_samplerForTrilinear, m_samplerForNearest;
	ObjCRef < id<MTLCommandQueue> > m_commandQueue;
	std::unique_ptr<VertexUniforms> m_uniformsVert; // because the type is not defined yet
	std::unique_ptr<FragmentUniforms> m_uniformsFrag;
	MetalBuffer<Float> m_nofilterExtras;
	ObjCRef < id<MTLDepthStencilState> > m_stencilStates[4];
	ObjCRef < id<MTLDepthStencilState> > m_nilStencilState;
	PipelineConfig m_pipelineConfigs[kNumPipelineType];
	Map < UInt32, ObjCRef < id<MTLRenderPipelineState> > > m_pipelineStates;

	bool m_active = false;
	bool m_blendingEnabled = true;
	bool m_stenciling = false;
	bool m_skipDrawing = false; // for use when stencil is active
	Renderable * m_renderTarget = nil;
};

struct Metal::CommonCanvas : public Renderer::Canvas
{
	CommonCanvas(Metal& renderer)
		: m_renderer(renderer)
	{
	}

	CommonCanvas(const CommonCanvas&) = delete;

	CommonCanvas& operator = (const CommonCanvas&) = delete;


	const iSize& GetSize() const override { return m_size; }

	Int32 GetPixelDensity() const override { return m_dpifactor; }


	iSize GetSizeInPixels() const { return { m_size.w * m_dpifactor, m_size.h * m_dpifactor }; }


	Reference <Metal> m_renderer;
	Int32 m_dpifactor = 1;
	iSize m_size = { 1, 1 };
};

///
/// Represents the "rendering" behavioural part of a canvas.
/// It could be directly in the CommonCanvas class, but not every canvas type has the ability to render,
/// so we chose composition over inheritance.
struct Metal::Renderable
{
	struct RenderDestination
	{
		ObjCRef <id<MTLTexture>> m_targetTexture;
		bool wantsMultisampleTexture;
	};

	Renderable(CommonCanvas & canvas)
		: canvas(canvas)
	{
	}

	void Resize(Metal & metal, iSize pixelSize);

	///
	/// Sets the renderable as the current render target.
	/// Meant to be called by an implementation of CommonCanvas::SetCurrent.
	/// During the initialisation, will call the initRenderPassDescriptor argument, which is expected to return a pair of textures to draw to.
	/// Only one is used (render texture) if antialiasing is not used. Otherwise the first texture is the resolve texture, and the second
	/// texture is the multisample texture.
	/// Note: the callback looks like this because it uses a static function (not a lambda).
	///
	void SetCurrentImpl(Metal & metal, FunctionPointer <RenderDestination(CommonCanvas&)> initRenderPassDescriptor);

	template <bool SCREEN> void FlushImpl(Metal & metal, id<CAMetalDrawable> drawable, bool createMipMaps);

	void BeginEncoding(Metal & metal);

	void EndEncoding();


	CommonCanvas & canvas;
	MTLScissorRect m_scissorRect;
	UInt8 m_texturePixelFormatIndex = 0;
	ObjCRef < id<MTLTexture> > m_targetTexture;
	ObjCRef < id<MTLTexture> > m_multisampleTexture;
	ObjCRef < MTLRenderPassDescriptor* > m_renderPassDescriptor;
	ObjCRef < id<MTLRenderCommandEncoder> > m_renderCommandEncoder;
	ObjCRef < id<MTLCommandBuffer> > m_commandBuffer;
	ObjCRef < id<MTLTexture> > m_stencilTexture;
	ObjCRef < id<MTLTexture> > m_stencilResolveTexture;
	__unsafe_unretained id<MTLDepthStencilState> m_stencilState = nil;
};

struct Metal::ScreenCanvas : public Metal::CommonCanvas
{
	ScreenCanvas(Metal & renderer);

	~ScreenCanvas() override;


	virtual CAMetalLayer* GetMetalLayer() = 0;


	bool SetSize(const iSize& size, Int32 dpifactor) override;

	void SetCurrent() override;

	void Flush() override;

	void Write(ImageFormat format, const ArrayView<UInt8>& data) override { REFLEX_ASSERT_EX(false, "Cannot write to screen canvas");}

	TRef<Graphic> CreateTextures(const ArrayView<Pair<fRect>>& rects) const override { return Graphic::null; }

	TRef <Graphic> CreateTexturesWithFilter(const ArrayView < Pair <fRect> >& rects, FilterMode mode, const ArrayView <Float>& parameters) const override { return Graphic::null; }


	Renderable m_renderable;
	ObjCRef < id<CAMetalDrawable> > m_drawable;
};

struct Metal::Bitmap : public Metal::CommonCanvas
{
	Bitmap(Metal & renderer, bool intendUsingAntialiasing, MTLTextureUsage textureUsage = MTLTextureUsageShaderRead);

	~Bitmap() override;


	bool SetSize(const iSize& size, Int32 dpiFactor) override;

	void Write(ImageFormat format, const ArrayView<UInt8>& data) override;

	TRef<Graphic> CreateTextures(const ArrayView<Pair<fRect>>& rects) const override;

	TRef<Graphic> CreateTexturesWithFilter(const ArrayView<Pair<fRect>>& rects, FilterMode mode, const ArrayView <Float>& parameters) const override;

	void SetCurrent() override { DEV_WARN("Cannot render on Metal::Bitmap"); }

	void Flush() override {}


	ObjCRef < id<MTLTexture> > m_texture;


protected:

	void RegenerateTextureIfNecessary();	//Defered creation of the texture to the moment it's used

	const MTLTextureUsage m_textureUsage;
	const bool m_antialias;
	ImageFormat m_imageFormat;
};

struct Metal::RenderableBitmap : public Metal::Bitmap
{
	RenderableBitmap(Metal& renderer, bool antiAlias);

	~RenderableBitmap();


	bool SetSize(const iSize& size, Int32 dpiFactor) override;

	void SetCurrent() override;

	void Flush() override;


	Renderable m_renderable;
};

REFLEX_END_INTERNAL
