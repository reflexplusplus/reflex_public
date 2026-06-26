#include "metal.h"
#include "metal_resources.cpp"

#define METAL_ENABLE_AA false	//NOTE James accidentally merged this to main, but its not ready as it more than doubles rendering time. TODO need to look into dynamically enabling it.

// MARK: Common with Shaders.metal

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

constexpr MTLPixelFormat kPixelFormats[] = { MTLPixelFormatInvalid, MTLPixelFormatInvalid, MTLPixelFormatRGBA8Unorm, MTLPixelFormatBGRA8Unorm, MTLPixelFormatR8Unorm };
constexpr MTLPrimitiveType kPrimitiveTypes[] = { MTLPrimitiveTypeLine, MTLPrimitiveTypeLineStrip, MTLPrimitiveTypeTriangle, MTLPrimitiveTypeTriangleStrip, MTLPrimitiveTypePoint };
constexpr MTLPixelFormat kPixelFormatIndexToPixelFormat[] = { MTLPixelFormatInvalid, MTLPixelFormatRGBA8Unorm, MTLPixelFormatBGRA8Unorm, MTLPixelFormatR8Unorm };

enum VertexBufferIdx
{
	kVertexBufferIdxVertices, 
	kVertexBufferIdxUniforms
};

enum FragmentBufferIndex
{
	kFBI_Uniforms, 
	kFBI_ExtraParams
};

constexpr UInt kTBI_ColorIndex = 0;

constexpr UInt kSBI_ColorIndex = 0;

struct VertexPoint
{
	simd_packed_float2 position;
};

struct VertexColourPoint
{
	simd_packed_float2 position;
	simd_packed_float4 colour;
};

struct VertexTextureCoord
{
	simd_packed_float2 position;
	simd_packed_float2 texCoord;
};

struct Metal::VertexUniforms
{
	simd::float2 viewportOrigin;
	simd::float2 viewportSize;
	simd::float2 offset;
	simd::float2 scale;
	simd::float4 colour;
};

struct Metal::FragmentUniforms
{
	// Filters
	simd::float4x4 colourTransform = matrix_identity_float4x4;
	simd::float4 filterParams;
	uint32_t filterMode;
	float ditherAmount = 0.0f;
	uint32_t pad[3] = {};
};

// MARK: End (Common with Shaders.metal)


// MARK: Metal::Primitives
template <class TYPE> void Metal::MetalBuffer<TYPE>::Create(Metal& renderer, const ArrayView <TYPE> & data)
{
	m_buffer = MakeOwnedObjCRef([renderer.m_mtlDevice newBufferWithBytes:data.data length:data.size * sizeof(TYPE) options:MTLResourceStorageModeShared]);
}

template<class VertexType>
struct Metal::NativePrimitives : Renderer::Graphic {

	NativePrimitives(Metal& renderer, PipelineType kind, MTLPrimitiveType primitiveType, UInt numVertices)
		: renderer(renderer)
		, m_kind(kind)
		, m_primitiveType(primitiveType)
		, m_nvertex(numVertices)
	{
	}

	NativePrimitives(Metal& renderer, PipelineType kind, MTLPrimitiveType primitiveType, const ArrayView<VertexType> & buffer)
		: NativePrimitives(renderer, kind, primitiveType, buffer.size)
	{
		m_vbo.Create(renderer, buffer);
	}

#if REFLEX_DEBUG
	~NativePrimitives()
	{
		REFLEX_ASSERT(renderer.m_active);
	}
#endif

	void Render(const Transform& transform, const Colour& colour) const override { RenderImpl<false>(transform, colour, nil, false); }

	template <bool TEXTURE> REFLEX_INLINE void RenderImpl(const Transform& transform, const Colour& colour,	const id <MTLTexture> _Nullable texture, bool trilinearFiltering) const
	{
		auto & current = *renderer.m_renderTarget;

		__unsafe_unretained auto pass = current.m_renderCommandEncoder.Get();

		REFLEX_ASSERT(pass);

		auto& uniformsVert = *renderer.m_uniformsVert;
		auto& uniformsFrag = *renderer.m_uniformsFrag;

		uniformsVert.offset = simd_make_float2(transform.origin.x, transform.origin.y);
		uniformsVert.scale = simd_make_float2(transform.scale.w, transform.scale.h);
		uniformsVert.colour = simd_make_float4(colour.r, colour.g, colour.b, colour.a * transform.opacity);

		auto pipeline = renderer.GetRenderPipeline(m_kind, current.m_texturePixelFormatIndex);

		// TODO: Florian -- check if setting all these can be what makes Metal slower
		[pass setRenderPipelineState:pipeline];

		[pass setVertexBuffer:m_vbo offset:0 atIndex:kVertexBufferIdxVertices];

		[pass setVertexBytes:&uniformsVert length:sizeof(uniformsVert) atIndex:kVertexBufferIdxUniforms];
		[pass setFragmentBytes:&uniformsFrag length:sizeof(uniformsFrag) atIndex:kFBI_Uniforms];

		if constexpr (TEXTURE)
		{
			[pass setFragmentTexture:texture atIndex:kTBI_ColorIndex];
			[pass setFragmentSamplerState:trilinearFiltering ? renderer.m_samplerForTrilinear : renderer.m_samplerForNearest atIndex:kSBI_ColorIndex];
		}

		[pass drawPrimitives:m_primitiveType vertexStart:0 vertexCount:m_nvertex instanceCount:1];
	}

	Metal & renderer;
	const PipelineType m_kind;
	const MTLPrimitiveType m_primitiveType;
	const UInt m_nvertex;
	MetalBuffer<VertexType> m_vbo;
};

struct Metal::NativeTexturedPrimitives : NativePrimitives<VertexTextureCoord>
{
	typedef VertexTextureCoord VertexType;

	NativeTexturedPrimitives(const Bitmap& bitmap, PipelineType kind, MTLPrimitiveType primitiveType, bool trilinearFiltering, const ArrayView<Pair<fRect>>& rects, iSize size)
		: NativePrimitives<VertexType>(bitmap.m_renderer, kind, primitiveType, rects.size * 6)
		, m_bitmap(bitmap)
		, m_trilinearFiltering(trilinearFiltering)
	{
		Array<VertexType> vertices(rects.size * 6);
		auto* vertexPtr = vertices.GetData();

		Float32 fw = Reciprocal(Float32(size.w));
		Float32 fh = Reciprocal(Float32(size.h));

		REFLEX_FOREACH(rect, rects) {
			auto& src = rect.a;
			auto& src_origin = src.origin;
			auto& src_size = src.size;

			auto& dst = rect.b;
			auto& dst_origin = dst.origin;
			auto& dst_size = dst.size;

			Float32 src_x1 = Float32(src_origin.x) * fw;
			Float32 src_y1 = Float32(src_origin.y) * fh;
			Float32 src_x2 = Float32(src_origin.x + src_size.w) * fw;
			Float32 src_y2 = Float32(src_origin.y + src_size.h) * fh;

			Float32 dst_x1 = dst_origin.x;
			Float32 dst_x2 = dst_x1 + dst_size.w;
			Float32 dst_y1 = dst_origin.y;
			Float32 dst_y2 = dst_y1 + dst_size.h;

			vertexPtr->position = simd_make_float2(dst_x1, dst_y1);
			vertexPtr->texCoord = simd_make_float2(src_x1, src_y1);
			vertexPtr += 1;

			vertexPtr->position = simd_make_float2(dst_x2, dst_y1);
			vertexPtr->texCoord = simd_make_float2(src_x2, src_y1);
			vertexPtr += 1;

			vertexPtr->position = simd_make_float2(dst_x1, dst_y2);
			vertexPtr->texCoord = simd_make_float2(src_x1, src_y2);
			vertexPtr += 1;

			vertexPtr->position = simd_make_float2(dst_x1, dst_y2);
			vertexPtr->texCoord = simd_make_float2(src_x1, src_y2);
			vertexPtr += 1;

			vertexPtr->position = simd_make_float2(dst_x2, dst_y2);
			vertexPtr->texCoord = simd_make_float2(src_x2, src_y2);
			vertexPtr += 1;

			vertexPtr->position = simd_make_float2(dst_x2, dst_y1);
			vertexPtr->texCoord = simd_make_float2(src_x2, src_y1);
			vertexPtr += 1;
		}

		m_vbo.Create(bitmap.m_renderer, vertices);
	}

	void Render(const Transform& transform, const Colour& colour) const override
	{
		RenderImpl<true>(transform, colour, m_bitmap->m_texture, m_trilinearFiltering);
	}

	ConstReference <Bitmap> m_bitmap;
	bool m_trilinearFiltering;
};

struct Metal::NativePrimitivesWithFilter : NativeTexturedPrimitives
{
	NativePrimitivesWithFilter(const Bitmap& bitmap, PipelineType kind, MTLPrimitiveType primitiveType, bool trilinearFiltering, const ArrayView<Pair<fRect>>& rects, iSize size, FilterMode mode, const ArrayView <Float>& in_params)
		: NativeTexturedPrimitives(bitmap, kind, primitiveType, trilinearFiltering, rects, size)
		, m_mode(mode)
	{
		auto widthUnits = Float(bitmap.m_size.w);
		auto heightUnits = Float(bitmap.m_size.h);

		switch (mode) {
			case kFilterModePixelate:
				REFLEX_ASSERT(in_params.size == 2);
				m_params = simd_make_float4(in_params[0] / widthUnits, in_params[1] / heightUnits, 0, 0);
				break;

			case kFilterModeBlur: {
				// out_params => direction.xy: float[2], radius: float, gaussian_weights: float[radius + 1]
				REFLEX_ASSERT(in_params.size >= 4);
				m_params = simd_make_float4(in_params[0] / widthUnits, in_params[1] / heightUnits, in_params[2], 0);

				auto gaussian_weights = Mid(in_params, 3);
				REFLEX_ASSERT(gaussian_weights.size == (UInt(in_params[2]) + 1));

				m_extraparams.Create(*bitmap.m_renderer.Adr(), gaussian_weights);
				break;
			}

			default:
				DEV_ERROR("Unsupported filter type");
				return;
		}
	}

	void Render(const Transform& transform, const Colour& colour) const override {
		auto& current = *renderer.m_renderTarget;
		auto& uniformsFrag = *renderer.m_uniformsFrag;

		// TODO: check; accessing uniforms like that on the CPU might be causing stalls, maybe better to use a private (GPU mem) buffer, and blit new vars all at once to it
		uniformsFrag.filterMode = m_mode;
		uniformsFrag.filterParams = m_params;

		if (m_extraparams) {
			[current.m_renderCommandEncoder setFragmentBuffer:m_extraparams offset:0 atIndex:kFBI_ExtraParams];
		}

		NativeTexturedPrimitives::Render(transform, colour);

		uniformsFrag.filterMode = FilterMode::kFilterModeNone;
		if (m_extraparams) [current.m_renderCommandEncoder setFragmentBuffer:renderer.m_nofilterExtras offset:0 atIndex:kFBI_ExtraParams];
	}

	FilterMode m_mode;
	simd::float4 m_params;
	MetalBuffer <Float> m_extraparams;
};

// MARK: Metal::Implementation
Metal::Metal(bool dpiAware, bool useAntialiasing, bool allowRenderToTexture)
	: m_dpiAware(dpiAware)
	, m_allowRenderToTexture(allowRenderToTexture)
	, m_antialiasingEnabled(METAL_ENABLE_AA && useAntialiasing)
	, m_mtlDevice(MakeObjCRef(MTLCreateSystemDefaultDevice()))
	, m_uniformsVert(new VertexUniforms)
	, m_uniformsFrag(new FragmentUniforms)
{
	m_library = LoadShaderLibrary();
	if (!m_library) {
		DEV_LOG("Metal device or library missing");
		return;
	}

	// Older Apple GPUs (iPhone 7 and below) support antialiasing, but not for the stencil buffer (mask).
	// We would need to temporarily disable the antialiasing when the mask is active, which requires a refactoring.
	if (m_antialiasingEnabled) {
		auto isAppleGPU = [m_mtlDevice supportsFamily:MTLGPUFamilyApple1];
		auto supportsStencilBufferMultisample = [m_mtlDevice supportsFamily:MTLGPUFamilyApple3];
		m_antialiasingEnabled = !(isAppleGPU && !supportsStencilBufferMultisample);
	}

	auto vertFunc_point = MakeOwnedObjCRef([m_library newFunctionWithName:@"vertFunc_point"]);
	auto vertFunc_colPoint = MakeOwnedObjCRef([m_library newFunctionWithName:@"vertFunc_colPoint"]);
	auto vertFunc_texCoord = MakeOwnedObjCRef([m_library newFunctionWithName:@"vertFunc_texCoord"]);
	auto fragFunc_colPoint = MakeOwnedObjCRef([m_library newFunctionWithName:@"fragFunc_colPoint"]);
	auto fragFunc_texRGBA = MakeOwnedObjCRef([m_library newFunctionWithName:@"fragFunc_texRGBA"]);
	auto fragFunc_texLuminance = MakeOwnedObjCRef([m_library newFunctionWithName:@"fragFunc_texLuminance"]);

	auto vertexTexDescForFlat = CreateVertexDescriptor<VertexPoint>();
	auto vertexTexDescForGouraud = CreateVertexDescriptor<VertexColourPoint>();
	auto vertexTexDescForTexture = CreateVertexDescriptor<VertexTextureCoord>();

	m_pipelineConfigs[kPipelineTypeVertexPoint] = { vertexTexDescForFlat, vertFunc_point, fragFunc_colPoint };
	m_pipelineConfigs[kPipelineTypeVertexColorPoint] = { vertexTexDescForGouraud, vertFunc_colPoint, fragFunc_colPoint };
	m_pipelineConfigs[kPipelineTypeTextureRGBA] = { vertexTexDescForTexture, vertFunc_texCoord, fragFunc_texRGBA };
	m_pipelineConfigs[kPipelineTypeTextureLuminance] = { vertexTexDescForTexture, vertFunc_texCoord, fragFunc_texLuminance };

	REFLEX_FOREACH(config, m_pipelineConfigs) {
		if (!(config.vertexDescriptor && config.vertFunc && config.fragFunc)) {
			DEV_LOG("Metal library incomplete");
			return;
		}
	}

	constexpr auto CreateStencilDescriptor = [](id<MTLDevice> device, bool writeToStencil, bool invert) {
		auto stencilDescriptorRef = MakeOwnedObjCRef([MTLStencilDescriptor new]);
		__unsafe_unretained auto stencilDescriptor = stencilDescriptorRef.Get();
		stencilDescriptor.stencilFailureOperation = MTLStencilOperationKeep;
		stencilDescriptor.depthFailureOperation = MTLStencilOperationKeep;
		stencilDescriptor.depthStencilPassOperation = writeToStencil ? MTLStencilOperationReplace : MTLStencilOperationKeep;
		stencilDescriptor.stencilCompareFunction = writeToStencil ? MTLCompareFunctionAlways : (invert ? MTLCompareFunctionNotEqual : MTLCompareFunctionEqual);
		stencilDescriptor.writeMask = writeToStencil ? 0xFF : 0x00;

		auto depthStencilDescriptorRef = MakeOwnedObjCRef([MTLDepthStencilDescriptor new]);
		__unsafe_unretained auto depthStencilDescriptor = depthStencilDescriptorRef.Get();
		depthStencilDescriptor.depthWriteEnabled = NO;
		depthStencilDescriptor.frontFaceStencil = stencilDescriptor;
		depthStencilDescriptor.backFaceStencil = stencilDescriptor;

		return MakeOwnedObjCRef([device newDepthStencilStateWithDescriptor:depthStencilDescriptor]);
	};

	REFLEX_LOOP(idx, 4) {
		m_stencilStates[idx] = CreateStencilDescriptor(m_mtlDevice, BitCheck(idx, 0), BitCheck(idx, 1));
	}

	// When not using stencil at all
	auto depthStencilDesc = MakeOwnedObjCRef([MTLDepthStencilDescriptor new]);
	depthStencilDesc.Get().depthCompareFunction = MTLCompareFunctionAlways;
	depthStencilDesc.Get().depthWriteEnabled = NO;
	depthStencilDesc.Get().frontFaceStencil = nil;
	depthStencilDesc.Get().backFaceStencil = nil;

	m_nilStencilState = MakeOwnedObjCRef([m_mtlDevice newDepthStencilStateWithDescriptor:depthStencilDesc]);

	m_samplerForNearest = CreateSamplerState(false);
	m_samplerForTrilinear = CreateSamplerState(true);

	m_commandQueue = MakeOwnedObjCRef([m_mtlDevice newCommandQueue]);

	Float emptyBuffer;
	m_nofilterExtras.Create(*this, { &emptyBuffer, 1 });

	m_status = m_samplerForNearest && m_samplerForTrilinear && m_commandQueue;
}

Renderer::Config Metal::GetConfig() const
{
	return
	{
		{ kHD, m_dpiAware },
		{ kTX, m_allowRenderToTexture },
#if METAL_ENABLE_AA
		{ kAA, m_antialiasingEnabled },
#endif
	};
}

bool Metal::SupportsImageFormat(ImageFormat format) const
{
	return kPixelFormats[format] != MTLPixelFormatInvalid;
}

// MARK: Metal::access (must set before use)
void Metal::BeginAccess()
{
	m_active = true;
}

void Metal::EndAccess()
{
	REFLEX_ASSERT(m_active);
	m_active = false;
}

TRef<Renderer::Canvas> Metal::CreateBitmap(bool alphachannel, bool antialias)
{
	if (m_allowRenderToTexture)
	{
		return REFLEX_CREATE(RenderableBitmap, *this, antialias);
	}
	else
	{
		return REFLEX_CREATE(Bitmap, *this, antialias);
	}
}

TRef<Renderer::Graphic> Metal::CreatePrimitives(PrimitiveType primitive, const ArrayView<fPoint>& points)
{
	REFLEX_STATIC_ASSERT(sizeof(VertexPoint) == sizeof(fPoint))

	REFLEX_ASSERT(m_active);

	if (points) return REFLEX_CREATE(NativePrimitives<fPoint>, *this, kPipelineTypeVertexPoint, kPrimitiveTypes[primitive], points);

	return Graphic::null;
}

TRef<Renderer::Graphic> Metal::CreatePrimitives(PrimitiveType primitive, const ArrayView<ColourPoint>& points)
{
	REFLEX_STATIC_ASSERT(sizeof(VertexColourPoint) == sizeof(ColourPoint))

	REFLEX_ASSERT(m_active);

	if (points) return REFLEX_CREATE(NativePrimitives<ColourPoint>, *this, kPipelineTypeVertexColorPoint, kPrimitiveTypes[primitive], points);

	return Graphic::null;
}

void Metal::Clear(const Colour& colour)
{
	REFLEX_ASSERT(m_active);
	REFLEX_ASSERT(m_renderTarget);
	REFLEX_ASSERT(m_renderTarget->m_targetTexture);

	m_renderTarget->EndEncoding();

	// Reuse the current render pass for clearing, and restore it for rendering
	__unsafe_unretained auto renderPassDescriptor = m_renderTarget->m_renderPassDescriptor.Get();

	renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;

	renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(colour.r, colour.g, colour.b, colour.a);

	m_renderTarget->BeginEncoding(*this);

	m_renderTarget->EndEncoding();

	renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;

	m_renderTarget->BeginEncoding(*this);
}

void Metal::EnableBlend(bool enable)
{
	m_blendingEnabled = enable;
}

void Metal::SetDitheringAmount(Float amount)
{
	m_uniformsFrag->ditherAmount = amount / 255.0f;
}

void Metal::SetColourTransform(const ArrayView <Float> & m)
{
	REFLEX_ASSERT(m.size == 16)

	auto cell = m.data;
	REFLEX_LOOP(col, 4)
	{
		m_uniformsFrag->colourTransform.columns[col] = { cell[0], cell[1], cell[2], cell[3] };
		cell += 4;
	}
}

void Metal::SetClip(const iRect& rect)
{
	REFLEX_ASSERT(m_renderTarget);

	auto & canvas = m_renderTarget->canvas;

	auto scissorRect = ToMTLScissorRect(rect, canvas.m_dpifactor);

	m_renderTarget->m_scissorRect = scissorRect;

	[m_renderTarget->m_renderCommandEncoder setScissorRect:scissorRect];
}

void Metal::SetMask(const Graphic& mask, const Transform& transform, bool invert)
{
	auto & canvas = m_renderTarget->canvas;

	m_renderTarget->EndEncoding();

	if (!m_renderTarget->m_stencilTexture)
	{
		auto pixelSize = canvas.GetSizeInPixels();

		MTLTextureDescriptor* stencilTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kStencilPixelFormat width:pixelSize.w height:pixelSize.h mipmapped:NO];
		stencilTextureDesc.storageMode = MTLStorageModePrivate;
		stencilTextureDesc.usage = MTLTextureUsageRenderTarget;

		m_renderTarget->m_stencilTexture = MakeOwnedObjCRef([m_mtlDevice newTextureWithDescriptor:stencilTextureDesc]);

		// MSAA stencil resolve texture
		if (m_antialiasingEnabled)
		{
			stencilTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:kStencilPixelFormat width:pixelSize.w height:pixelSize.h mipmapped:NO];

			stencilTextureDesc.textureType = MTLTextureType2DMultisample;
			stencilTextureDesc.sampleCount = kNumSamplesInMultisampleMode;
			stencilTextureDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
			stencilTextureDesc.storageMode = MTLStorageModePrivate;

			m_renderTarget->m_stencilResolveTexture = MakeOwnedObjCRef([m_mtlDevice newTextureWithDescriptor:stencilTextureDesc]);
		}
	}


	//clear and write to stencil
	__unsafe_unretained auto stencilAttachment = m_renderTarget->m_renderPassDescriptor.Get().stencilAttachment;

	if (m_renderTarget->m_stencilResolveTexture)
	{
		stencilAttachment.texture = m_renderTarget->m_stencilResolveTexture;
		stencilAttachment.resolveTexture = m_renderTarget->m_stencilTexture;
		stencilAttachment.storeAction = MTLStoreActionStoreAndMultisampleResolve;
	}
	else
	{
		stencilAttachment.texture = m_renderTarget->m_stencilTexture;
		stencilAttachment.storeAction = MTLStoreActionStore;
	}

	stencilAttachment.loadAction = MTLLoadActionClear;
	stencilAttachment.clearStencil = 0;

	m_renderTarget->m_stencilState = m_stencilStates[MakeBits(true, invert)];

	m_skipDrawing = true;

	m_renderTarget->BeginEncoding(*this);

	Colour colour = { 1, 1, 1, 1 };

	mask.Render(transform, colour);	//write to mask to set the 1's

	m_renderTarget->EndEncoding();

	m_skipDrawing = false;


	//set stencil active

	stencilAttachment.loadAction = MTLLoadActionLoad; // Clear stencil buffer at the start

	m_renderTarget->m_stencilState = m_stencilStates[MakeBits(false, invert)];

	m_renderTarget->BeginEncoding(*this);
}

void Metal::ClearMask()
{
	m_renderTarget->EndEncoding();

	m_renderTarget->m_stencilState = nil;

	__unsafe_unretained auto stencilAttachment = m_renderTarget->m_renderPassDescriptor.Get().stencilAttachment;
	stencilAttachment.texture = nil;
	stencilAttachment.resolveTexture = nil;
	stencilAttachment.loadAction = MTLLoadActionClear;
	stencilAttachment.storeAction = MTLStoreActionDontCare;

	m_renderTarget->BeginEncoding(*this);
}

// MARK: Metal::info
Renderer::Canvas* _Nullable Metal::GetCurrent()
{
	if (m_renderTarget)
	{
		return &m_renderTarget->canvas;
	}

	return 0;
}

ObjCRef < id<MTLSamplerState> > Metal::CreateSamplerState(bool forAntialiased)
{
	auto samplerDescriptorRef = MakeOwnedObjCRef([MTLSamplerDescriptor new]);

	__unsafe_unretained auto samplerDescriptor = samplerDescriptorRef.Get();

	samplerDescriptor.minFilter = forAntialiased ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
	samplerDescriptor.magFilter = forAntialiased ? MTLSamplerMinMagFilterLinear : MTLSamplerMinMagFilterNearest;
	samplerDescriptor.mipFilter = forAntialiased ? MTLSamplerMipFilterLinear : MTLSamplerMipFilterNearest;
	if (!forAntialiased)
	{
		samplerDescriptor.lodMinClamp = 0;
		samplerDescriptor.lodMaxClamp = 0;
	}

	samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
	samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;

	//NOTE we may want to rather use
	//samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToZero;
	//samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToZero;
	//or
	//samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToBorderColor;
	//samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToBorderColor;
	//samplerDescriptor.borderColor  = MTLSamplerBorderColorTransparentBlack;
	//in case of issues with edges causing glitches with premult alpha

	return MakeOwnedObjCRef([m_mtlDevice newSamplerStateWithDescriptor:samplerDescriptor]);
}

ObjCRef < id<MTLRenderPipelineState> > Metal::CreateRenderPipeline(PipelineConfig & config, bool blendingEnabled, MTLPixelFormat pixelFormat, MTLPixelFormat stencilPixelFormat, bool skipDrawingPixels, bool useAntialiasing)
{
	NSError* error;

	auto pipelineStateDescriptorRef = MakeOwnedObjCRef([MTLRenderPipelineDescriptor new]);
	__unsafe_unretained auto pipelineStateDescriptor = pipelineStateDescriptorRef.Get();
	pipelineStateDescriptor.rasterSampleCount = useAntialiasing ? kNumSamplesInMultisampleMode : 1;
	pipelineStateDescriptor.vertexFunction = config.vertFunc;
	pipelineStateDescriptor.fragmentFunction = config.fragFunc;
	pipelineStateDescriptor.vertexDescriptor = config.vertexDescriptor;
	pipelineStateDescriptor.colorAttachments[0].pixelFormat = pixelFormat;
	pipelineStateDescriptor.colorAttachments[0].writeMask = skipDrawingPixels ? MTLColorWriteMaskNone : MTLColorWriteMaskAll;

	auto renderbufferAttachment = pipelineStateDescriptor.colorAttachments[0];
	renderbufferAttachment.blendingEnabled = blendingEnabled;

	if (renderbufferAttachment.blendingEnabled)
	{
		renderbufferAttachment.rgbBlendOperation = MTLBlendOperationAdd;
		renderbufferAttachment.alphaBlendOperation = MTLBlendOperationAdd;

		renderbufferAttachment.sourceRGBBlendFactor = MTLBlendFactorOne;
		renderbufferAttachment.sourceAlphaBlendFactor = MTLBlendFactorOne;

		renderbufferAttachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
		renderbufferAttachment.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
	}

	if (stencilPixelFormat != MTLPixelFormatInvalid)
	{
		pipelineStateDescriptor.stencilAttachmentPixelFormat = stencilPixelFormat;
	}

	auto result = MakeOwnedObjCRef([m_mtlDevice newRenderPipelineStateWithDescriptor:pipelineStateDescriptor error:&error]);
	if (!result || error)
	{
		DEV_ERROR("Failed to create render pipeline: ", [[error description] UTF8String]);
		result = nil;
	}
	return result;
}

ObjCRef < id<MTLRenderPipelineState> > Metal::GetRenderPipeline(PipelineType pipelineType, UInt pixelFormatIndex)
{
	UInt32 word = pipelineType | (pixelFormatIndex << 2) | MakeBit(4, m_stenciling) | MakeBit(5, m_blendingEnabled) | MakeBit(6, m_skipDrawing);

	auto & pipelineState = m_pipelineStates[word];

	if (!pipelineState)
	{
		pipelineState = CreateRenderPipeline(m_pipelineConfigs[pipelineType], m_blendingEnabled, kPixelFormatIndexToPixelFormat[pixelFormatIndex], m_stenciling ? kStencilPixelFormat : MTLPixelFormatInvalid, m_skipDrawing, m_antialiasingEnabled);
	}

	return pipelineState;
}

ObjCRef < id<MTLLibrary> > Metal::LoadShaderLibrary()
{
	auto shaderSourceRef = MakeOwnedObjCRef([[NSString alloc] initWithBytes:kMetalShaders.data length:kMetalShaders.size encoding:NSUTF8StringEncoding]);

	NSError* error = nil;
	auto library = MakeOwnedObjCRef([m_mtlDevice newLibraryWithSource:shaderSourceRef options:nil error:&error]);

#if REFLEX_DEBUG
	if (!library)
	{
		NSString* desc = error ? error.localizedDescription : @"(no message)";
		DEV_ERROR("Couldn't compile ReflexShaders.metal", ToCStringView(desc));
	}
#endif

	return library;
}

template<class VertexType> ObjCRef <MTLVertexDescriptor*> Metal::CreateVertexDescriptor()
{
	auto result = [MTLVertexDescriptor vertexDescriptor];
	result.attributes[0].format = MTLVertexFormatFloat2;
	result.attributes[0].bufferIndex = 0;
	result.attributes[0].offset = 0;

	if constexpr(std::is_same<VertexType, VertexColourPoint>::value)
	{
		result.attributes[1].format = MTLVertexFormatFloat4;
		result.attributes[1].bufferIndex = 0;
		result.attributes[1].offset = offsetof(VertexType, colour);
	}
	else if constexpr(std::is_same<VertexType, VertexTextureCoord>::value)
	{
		result.attributes[1].format = MTLVertexFormatFloat2;
		result.attributes[1].bufferIndex = 0;
		result.attributes[1].offset = offsetof(VertexType, texCoord);
	}
	else if constexpr(!std::is_same<VertexType, VertexPoint>::value)
	{
		static_assert(always_false_v<VertexType>, "non-exhaustive shader type");
	}

	result.layouts[0].stride = sizeof(VertexType);
	result.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	return MakeObjCRef(result);
}

void Metal::GenerateMipMaps(id<MTLCommandBuffer> _Nonnull commandBuffer, id<MTLTexture> _Nonnull texture)
{
	auto blitEncoderRef = MakeObjCRef([commandBuffer blitCommandEncoder]);

	__unsafe_unretained auto blitEncoder = blitEncoderRef.Get();

	[blitEncoder generateMipmapsForTexture:texture];

	[blitEncoder endEncoding];
}

MTLScissorRect Metal::ToMTLScissorRect(const iRect& rect, int dpifactor)
{
	return { NSUInteger(rect.origin.x) * dpifactor, NSUInteger(rect.origin.y) * dpifactor, NSUInteger(rect.size.w) * dpifactor, NSUInteger(rect.size.h) * dpifactor };
}

// MARK: Metal::Renderable
void Metal::Renderable::Resize(Metal & metal, iSize pixelSize)
{
	m_scissorRect = { 0,0,UInt(pixelSize.w),UInt(pixelSize.h) };

	m_stencilTexture = nil;
	m_stencilResolveTexture = nil;
}

void Metal::Renderable::SetCurrentImpl(Metal & renderer, FunctionPointer <RenderDestination(CommonCanvas&)> _Nonnull init)
{
	REFLEX_ASSERT(renderer.m_active);
	REFLEX_ASSERT(!m_renderCommandEncoder);

	if (auto current = renderer.m_renderTarget) current->EndEncoding();

	if (!m_commandBuffer)
	{
		m_commandBuffer = MakeObjCRef([renderer.m_commandQueue commandBuffer]);

		MTLRenderPassDescriptor* renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];

		m_renderPassDescriptor = MakeObjCRef(renderPassDescriptor);

		RenderDestination dest = init(canvas);
		m_targetTexture = dest.m_targetTexture;

		// For multisampling, we need an extra texture
		if (dest.wantsMultisampleTexture)
		{
			__unsafe_unretained id<MTLTexture> msaaTex = m_multisampleTexture.Get();
			__unsafe_unretained id<MTLTexture> target = m_targetTexture.Get();

			if (!msaaTex || target.width != msaaTex.width || target.height != msaaTex.height)
			{
				MTLTextureDescriptor* msaaTextureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:target.pixelFormat width:target.width height:target.height mipmapped:NO];

				msaaTextureDescriptor.textureType = MTLTextureType2DMultisample;
				msaaTextureDescriptor.sampleCount = kNumSamplesInMultisampleMode;
				msaaTextureDescriptor.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
				msaaTextureDescriptor.storageMode = MTLStorageModePrivate;

				m_multisampleTexture = MakeOwnedObjCRef([renderer.m_mtlDevice newTextureWithDescriptor:msaaTextureDescriptor]);
			}
		}
		else
		{
			m_multisampleTexture = nil;
		}

		auto pixelFormatIndexQuery = Search(kPixelFormatIndexToPixelFormat, m_targetTexture.Get().pixelFormat);
		REFLEX_ASSERT(pixelFormatIndexQuery);
		m_texturePixelFormatIndex = pixelFormatIndexQuery.value;

		if (m_multisampleTexture)
		{
			renderPassDescriptor.colorAttachments[0].texture = m_multisampleTexture;
			renderPassDescriptor.colorAttachments[0].resolveTexture = m_targetTexture;
			renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
			renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStoreAndMultisampleResolve;
		}
		else
		{
			renderPassDescriptor.colorAttachments[0].texture = m_targetTexture;
			renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
			renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
		}
	}

	#if REFLEX_DEBUG
	if (renderer.m_renderTarget && renderer.m_renderTarget != this)
	{
		REFLEX_ASSERT_EX(!renderer.m_renderTarget->m_renderCommandEncoder, "Unfinished pass");
	}
	#endif

	renderer.m_renderTarget = this;

	BeginEncoding(renderer);
}

template <bool SCREEN> void Metal::Renderable::FlushImpl(Metal & renderer, id<CAMetalDrawable> _Nullable drawable, bool create_mipmaps)
{
	REFLEX_ASSERT(renderer.m_active);
	REFLEX_ASSERT(m_commandBuffer);
	REFLEX_ASSERT(renderer.m_renderTarget == this);

	EndEncoding();

	if constexpr (SCREEN)
	{
		[m_commandBuffer presentDrawable:drawable];
	}
	else if (create_mipmaps)
	{
		GenerateMipMaps(m_commandBuffer, m_targetTexture);
	}

	[m_commandBuffer commit];

	// Cannot be used anymore

	m_commandBuffer = nil;

	#if REFLEX_DEBUG
	if (renderer.m_renderTarget)
	{
		REFLEX_ASSERT_EX(!renderer.m_renderTarget->m_renderCommandEncoder, "Unfinished pass");
	}
	#endif

	renderer.m_renderTarget = 0;
}

void Metal::Renderable::BeginEncoding(Metal & renderer)
{
	REFLEX_ASSERT(m_renderPassDescriptor);
	REFLEX_ASSERT(m_commandBuffer);
	REFLEX_ASSERT(!m_renderCommandEncoder);

	auto size = canvas.m_size;

	auto sizePixels = canvas.GetSizeInPixels();


	MTLViewport viewport;

	viewport.originX = 0;
	viewport.originY = 0;
	viewport.width = sizePixels.w;
	viewport.height = sizePixels.h;
	viewport.znear = 0;
	viewport.zfar = 1;


	m_renderCommandEncoder = MakeObjCRef([m_commandBuffer renderCommandEncoderWithDescriptor:m_renderPassDescriptor]);

	[m_renderCommandEncoder setFragmentBuffer:renderer.m_nofilterExtras offset:0 atIndex:kFBI_ExtraParams];
	[m_renderCommandEncoder setViewport:viewport];

	Float32 w = Float32(size.w);
	Float32 h = Float32(size.h);

	renderer.m_uniformsVert->viewportOrigin = simd_make_float2(-(w * 0.5f), -(h * 0.5f));
	renderer.m_uniformsVert->viewportSize = simd_make_float2(w * 0.5f, h * -0.5f);


	[m_renderCommandEncoder setScissorRect:m_scissorRect];

	if ((renderer.m_stenciling = True(m_stencilState)))
	{
		[m_renderCommandEncoder setDepthStencilState:m_stencilState];

		[m_renderCommandEncoder setStencilReferenceValue:1];
	}
	else
	{
		// Metal does not accept a nil state, so we pass an object that corresponds to the initial setup
		[m_renderCommandEncoder setDepthStencilState:renderer.m_nilStencilState];
	}
}

void Metal::Renderable::EndEncoding()
{
	[m_renderCommandEncoder endEncoding];

	m_renderCommandEncoder = nil;
}

// MARK: Metal::ScreenCanvas::Implementation
Metal::ScreenCanvas::ScreenCanvas(Metal& renderer)
	: CommonCanvas(renderer)
	, m_renderable(*this)
{
}

Metal::ScreenCanvas::~ScreenCanvas()
{
	REFLEX_ASSERT(m_renderer->m_active);
}

bool Metal::ScreenCanvas::SetSize(const iSize & size, Int32 dpifactor)
{
	REFLEX_ASSERT_EX(size.w && size.h, "System::Canvas::SetSize");

	m_size = size;

	m_dpifactor = dpifactor;

	m_renderable.Resize(m_renderer, GetSizeInPixels());

	return true;
}

void Metal::ScreenCanvas::SetCurrent()
{
	REFLEX_ASSERT(!m_renderer->m_renderTarget);	//invalid usage

	m_renderable.SetCurrentImpl(m_renderer, [](CommonCanvas & canvas)
	{
		auto self = Cast<ScreenCanvas>(canvas);

		__unsafe_unretained auto drawable = self->GetMetalLayer().nextDrawable;

		self->m_drawable = MakeObjCRef(drawable);

		return Renderable::RenderDestination { MakeObjCRef(drawable.texture), self->m_renderer->m_antialiasingEnabled };
	});
}

void Metal::ScreenCanvas::Flush()
{
	REFLEX_ASSERT(m_drawable);

	m_renderable.FlushImpl<true>(m_renderer, m_drawable, 0);

	// Cannot be used anymore
	m_drawable = nil;
}

// MARK: Metal::Bitmap Implementation
Metal::Bitmap::Bitmap(Metal& renderer, bool antiAlias, MTLTextureUsage usage)
	: CommonCanvas(renderer)
	, m_textureUsage(usage)
	, m_antialias(antiAlias)
	, m_imageFormat(kImageFormatRGBA)
{
}

Metal::Bitmap::~Bitmap()
{
	REFLEX_ASSERT(m_renderer->m_active);
}

void Metal::Bitmap::RegenerateTextureIfNecessary()
{
	auto size = GetSizeInPixels();
	auto mtlPixelFormat = kPixelFormats[m_imageFormat];

	if (__unsafe_unretained auto texture = m_texture.Get())
	{
		if (size.w == texture.width && size.h == texture.height && mtlPixelFormat == texture.pixelFormat) return;
		// This should mostly never happen
		DEV_LOG("Recreating texture");
	}

	auto textureDescRef = MakeObjCRef([MTLTextureDescriptor texture2DDescriptorWithPixelFormat:mtlPixelFormat width:size.w height:size.h mipmapped:m_antialias]);
	__unsafe_unretained auto textureDesc = textureDescRef.Get();
	textureDesc.textureType = MTLTextureType2D;
	textureDesc.storageMode = MTLStorageModeShared;
	textureDesc.usage = m_textureUsage;

	m_texture = MakeOwnedObjCRef([m_renderer->m_mtlDevice newTextureWithDescriptor:textureDesc]);
}

bool Metal::Bitmap::SetSize(const iSize& size, Int32 dpiFactor)
{
	REFLEX_ASSERT_EX(size.w && size.h, "System::Canvas::SetSize");
	
	m_size = size;
	m_dpifactor = dpiFactor;

	m_texture = nil;

	return true;
}

void Metal::Bitmap::Write(ImageFormat format, const ArrayView<UInt8>& bytes)
{
	REFLEX_ASSERT(m_renderer->m_active);
	// MEMO: GLX bugfix
	if (kBPP[format] == 3)
	{
		DEV_LOG("Metal::Bitmap::Write unsupported format");
		return;
	}

	m_imageFormat = format;
	RegenerateTextureIfNecessary();

	if (VerifyBitmap({ m_size, m_dpifactor, format }, bytes))
	{
		auto sizePixels = GetSizeInPixels();

		UInt bytesPerRow = sizePixels.w * kBPP[format];

		MTLRegion region = MTLRegionMake2D(0, 0, sizePixels.w, sizePixels.h);

		[m_texture replaceRegion:region mipmapLevel:0 withBytes:bytes.data bytesPerRow:bytesPerRow];

		if (m_antialias)
		{
			auto commandBufferRef = MakeObjCRef([m_renderer->m_commandQueue commandBuffer]);

			__unsafe_unretained auto commandBuffer = commandBufferRef.Get();

			GenerateMipMaps(commandBuffer, m_texture);

			[commandBuffer commit];
		}
	}
	else
	{
		DEV_LOG("TEMP: Failed to verify bitmap");
	}
}

TRef<Renderer::Graphic> Metal::Bitmap::CreateTextures(const ArrayView<Pair<fRect>>& rects) const
{
	REFLEX_ASSERT(m_renderer->m_active);

	RemoveConst(this)->RegenerateTextureIfNecessary();

	auto pipeline = m_imageFormat == kImageFormatLuminance ? kPipelineTypeTextureLuminance : kPipelineTypeTextureRGBA;

	return REFLEX_CREATE(NativeTexturedPrimitives, *this, pipeline, MTLPrimitiveTypeTriangle, m_antialias, rects, GetSize());
}

TRef<Renderer::Graphic> Metal::Bitmap::CreateTexturesWithFilter(const ArrayView<Pair<fRect>>& rects, FilterMode mode, const ArrayView <Float>& parameters) const
{
	REFLEX_ASSERT(m_renderer->m_active);

	RemoveConst(this)->RegenerateTextureIfNecessary();

	auto pipeline = m_imageFormat == kImageFormatLuminance ? kPipelineTypeTextureLuminance : kPipelineTypeTextureRGBA;

	return REFLEX_CREATE(NativePrimitivesWithFilter, *this, pipeline, MTLPrimitiveTypeTriangle, m_antialias, rects, GetSize(), mode, parameters);
}

// MARK: Metal::BitmapWithRendering Implementation
Metal::RenderableBitmap::RenderableBitmap(Metal& renderer, bool intendUsingAntialiasing)
	: Bitmap(renderer, intendUsingAntialiasing, MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead)
	, m_renderable(*this)
{
}

Metal::RenderableBitmap::~RenderableBitmap()
{
	REFLEX_ASSERT(m_renderer->m_renderTarget != &m_renderable);
}

bool Metal::RenderableBitmap::SetSize(const iSize& size, Int32 dpiFactor)
{
	Bitmap::SetSize(size, dpiFactor);

	m_renderable.Resize(m_renderer, GetSizeInPixels());

	return true;
}

void Metal::RenderableBitmap::SetCurrent()
{
	m_renderable.SetCurrentImpl(m_renderer, [](CommonCanvas & canvas)
	{
		auto self = Cast<RenderableBitmap>(canvas);

		self->RegenerateTextureIfNecessary();

		return Renderable::RenderDestination { self->m_texture, self->m_renderer->m_antialiasingEnabled };
	});
}

void Metal::RenderableBitmap::Flush()
{
	m_renderable.FlushImpl<false>(m_renderer, 0, m_antialias);
}

REFLEX_END_INTERNAL
