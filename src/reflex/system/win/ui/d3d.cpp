#include "d3d.h"

#include "resources.h"
#include "window.h"
#include "../ms_com.h"

REFLEX_DISABLE_WARNINGS
#include <d3dcompiler.h>
REFLEX_ENABLE_WARNINGS

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

//#if REFLEX_DEBUG
//#define D3D_DEBUG 0
//#else
#define D3D_DEBUG 0
//#endif

#define D3D_CHECK(fn, args) fn##args




//
//declarations

Reflex::System::Win::D3dNullSwapChain Reflex::System::Win::D3dNullSwapChain::self;

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

class Direct3D : public Reflex::System::Renderer
{
public:

	class AbstractCanvas;

	class WindowCanvas;

	template <bool RENDER_TARGET> class Bitmap;


	struct AbstractVBO;

	template <class POINT> struct Primitives;

	struct Textures;

	struct FilteredTextures;


	template <class TYPE> using ComPtr = COM::Reference <TYPE>;

	template <class TYPE>
	struct Shader
	{
		ComPtr <ID3DBlob> blob;

		ComPtr <TYPE> sh;

		ComPtr <ID3D11InputLayout> layout;
	};

	struct Uniforms
	{
		fPoint gOffset;
		fSize gScale;
		Colour gColour;
	};

	struct TextureFilterUniforms
	{
		Float gFilterParams[4] = {};
		UInt32 gFilterMode = 0;
		UInt32 pad[3]; // 16-byte aligned
	};

	struct PixelUniforms
	{
		Float gColourTransform[16] = 
		{
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
		Float gDitherAmount = 0.0f;
		UInt32 pad[3] = {};
	};

	struct FilterParam
	{
		Float params[4];
	};

	enum ShaderType
	{
		kShaderTypePoint,
		kShaderTypeColourPoint,

		kShaderTypeTextureLuminance,
		kShaderTypeTextureRGBA,

		//kShaderTypePointWithMask,
		//kShaderTypeColourPointWithMask,

		//kShaderTypeTextureLuminance_GuassianBlur,
		//kShaderTypeTextureRGBA_GuassianBlur,

		kNumShaderType
	};

	static constexpr CString::View kEngineName = "Direct3D";

	Direct3D(bool hd, const Config & config);

	~Direct3D();

	CString::View GetEngineName() const override { return kEngineName; }

	bool Status() const override { return m_status; }

	bool SupportsImageFormat(ImageFormat format) const override { return format == kImageFormatRGBA || format == kImageFormatLuminance; }

	Config GetConfig() const override;


	void BeginAccess() override { m_shader = kNumShaderType; };

	void EndAccess() override { m_current = nullptr; };


	//objects

	TRef <Canvas> CreateCanvas(void * systemwindow) override;

	TRef <Canvas> CreateBitmap(bool alphachannel, bool antialias) override;


	TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <fPoint> & points) override;

	TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <ColourPoint> & points) override;


	//drawing

	void Clear(const Colour & colour) override;

	void EnableBlend(bool enable) override;

	void SetDitheringAmount(Float amount) override;

	void SetColourTransform(const ArrayView <Float> & m) override;

	void SetClip(const iRect & rect) override;

	void SetMask(const Graphic & graphic, const Transform & transform, bool invert) override;

	void ClearMask() override;


	Canvas * GetCurrent() override;


	void SelectShader(ShaderType type);

	template <class TYPE> static void CompileShader(ID3D11Device & device, const ArrayView <Pair<const char*>> & defines, Shader <TYPE> & shader);

	template <class TYPE> static bool CreateInputLayout(ID3D11Device & device, Shader <TYPE> & shader, const ArrayView <D3D11_INPUT_ELEMENT_DESC> & desc);

	template <class TYPE> static D3D11_BUFFER_DESC MakeConstBufferDesc();


	REFLEX_INLINE static bool Check(const char * desc, bool test)
	{
		if (!test)
		{
			Common::output.Error("D3D", desc, "FAIL", desc);
		}

		return test;
	}
	
	template <auto METHOD_PTR, class CLASS, class... VARGS> REFLEX_INLINE static bool CallChecked(const char * desc, CLASS && cls, VARGS &&... args)
	{
		HRESULT hr = (Deref(cls).*METHOD_PTR)(std::forward<VARGS>(args)...);

		return Check(desc, SUCCEEDED(hr));
	}


	bool m_status;

	bool m_dpiaware, /*m_enable_aa,*/ m_enable_tx;


	ComPtr <IDXGIFactory> m_factory;

	ComPtr <ID3D11Device> m_device;

	ComPtr <ID3D11DeviceContext> m_devicecontext;

	ComPtr <ID3D11RasterizerState> m_rasterstate, m_rasterstencil;

	ComPtr <ID3D11BlendState> m_blendstates[2];

	ComPtr <ID3D11DepthStencilState> m_stencilstates[3];

	ComPtr <ID3D11SamplerState> m_samplerstates[2];


	ShaderType m_shader = kNumShaderType;

	Pair < Shader <ID3D11VertexShader>, Shader <ID3D11PixelShader> > m_shaders[kNumShaderType];


	ComPtr <ID3D11Buffer> m_viewport, m_matrix, m_colfilter, m_texture_nofilter;

	
	Float m_dither_amount;

	PixelUniforms m_pixel_uniforms = {};


	AbstractCanvas * m_current;

	ID3D11SamplerState * m_samplerstate_z;


	D3D11_RECT m_cliprect;	//work around for d3d11 bug with stencil and scissor??


	Array <fPoint> m_points_buffer;


	static constexpr D3D_PRIMITIVE_TOPOLOGY kPrimitiveTypes[kNumPrimitiveType] =
	{
		D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
		D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
		D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
	};

	static constexpr Pair <DXGI_FORMAT,ShaderType> kImageFormats[kNumImageFormat] =
	{
		{DXGI_FORMAT_R8G8B8A8_UNORM, kShaderTypeTextureRGBA},
		{DXGI_FORMAT_R8G8B8A8_UNORM, kShaderTypeTextureRGBA},
		{DXGI_FORMAT_R8G8B8A8_UNORM, kShaderTypeTextureRGBA},
		{DXGI_FORMAT_R8G8B8A8_UNORM, kShaderTypeTextureRGBA},	//DXGI_FORMAT_B8G8R8A8_UNORM
		{DXGI_FORMAT_A8_UNORM, kShaderTypeTextureLuminance},
	};

	static inline const PixelUniforms kDefaultPixelUniforms = {};

};

class Direct3D::AbstractCanvas : public Canvas
{
public:

	AbstractCanvas(Direct3D& engine, UInt samplecount);

	~AbstractCanvas();

	const iSize& GetSize() const override;

	Int GetPixelDensity() const override;

	void SetCurrent() override;

	bool SetSize(const iSize & size, Int32 dpifactor);


	void SetRenderTargetView(ID3D11DeviceContext & ctx, ID3D11DepthStencilView * stencilview)
	{
		auto ptr = m_rendertargetview.Get();

		ctx.OMSetRenderTargets(1, &ptr, stencilview);
	}


	Direct3D& engine;

	Int32 m_idpifactor;

	Int m_pixeldensity;

	iSize m_size;


	UInt m_samplecount;

	ComPtr <IDXGISwapChain> m_swapchain;

	ComPtr <ID3D11RenderTargetView> m_rendertargetview;

	ComPtr <ID3D11Texture2D> m_stencil;

	ComPtr <ID3D11DepthStencilView> m_stencilview;

	bool m_stenciling = false;
};

class Direct3D::WindowCanvas : public Direct3D::AbstractCanvas
{
public:

	WindowCanvas(Direct3D & engine, HWND hwnd);

	bool SetSize(const iSize & size, Int32 dpifactor) override;

	void Write(ImageFormat format, const ArrayView <UInt8> & data) override { REFLEX_ASSERT(false); }

	TRef <Graphic> CreateTextures(const ArrayView < Pair <fRect> > & rects) const override { return Graphic::null; }

	TRef <Graphic> CreateTexturesWithFilter(const ArrayView < Pair <fRect> >& rects, FilterMode mode, const ArrayView <Float>& parameters) const override { return Graphic::null; }

	void Flush() override;

	const HWND m_hwnd;
};

template <bool RENDER_TARGET>
class Direct3D::Bitmap : public Direct3D::AbstractCanvas
{
public:

	Bitmap(Direct3D & engine, bool alphachannel, bool antialias);

	bool SetSize(const iSize & size, Int32 dpifactor) override;

	void Write(ImageFormat format, const ArrayView <UInt8> & data) override;

	TRef <Graphic> CreateTextures(const ArrayView < Pair <fRect> >& rects) const override;

	TRef <Graphic> CreateTexturesWithFilter(const ArrayView < Pair <fRect> > & rects, FilterMode mode, const ArrayView <Float>& parameters) const override;

	void Flush() override;


	bool m_alphachannel;

	bool m_aa;

	ShaderType m_rendertype;

	ComPtr <ID3D11Texture2D> m_texture;

	mutable ComPtr <ID3D11ShaderResourceView> m_textureview;

	ID3D11SamplerState * m_samplerstate;
};

struct Direct3D::AbstractVBO : public System::Renderer::Graphic
{
	AbstractVBO(Direct3D & engine, UInt nvertex);

	~AbstractVBO();

	void Draw(ID3D11DeviceContext & ctx, const Transform & t, const Colour & colour, D3D_PRIMITIVE_TOPOLOGY type, UINT vertex_stride) const;


	Direct3D & engine;

	const ComPtr <ID3D11Buffer> m_vbo;

	UInt m_nvertex;
};

template <class POINT>
struct Direct3D::Primitives : public AbstractVBO
{
	Primitives(Direct3D & engine, D3D_PRIMITIVE_TOPOLOGY type, const typename ArrayView <POINT> & points);

	void Render(const Transform & transform, const Colour & colour) const override;


	D3D_PRIMITIVE_TOPOLOGY m_type;
};

struct Direct3D::Textures : public AbstractVBO
{
	Textures(const AbstractCanvas & bitmap, const ArrayView < Pair <fRect> > & rects);

	void Render(const Transform & transform, const Colour & colour) const override;

	ConstReference <AbstractCanvas> m_bitmap;
};

struct InvalidTextures : public System::Renderer::Graphic
{
	InvalidTextures(const System::Renderer::Canvas& canvas)
		: m_bitmap(canvas)
	{
	}

	void Render(const System::Renderer::Transform& transform, const Colour& colour) const override
	{
	}

	ConstReference <System::Renderer::Canvas> m_bitmap;
};

struct Direct3D::FilteredTextures : public Textures
{
	FilteredTextures(const AbstractCanvas& bitmap, const ArrayView < Pair <fRect> >& rects, FilterMode mode, const ArrayView <Float>& in_params);

	void Render(const Transform& transform, const Colour& colour) const override;


	bool m_status = false;

private:

	ComPtr <ID3D11Buffer> m_filter_constants, m_filter_dynamic;
	ComPtr <ID3D11ShaderResourceView> m_filter_dynamic_view;
};



//
//renderer implementation

Direct3D::Direct3D(bool dpiaware, const Config & config)
	: m_dpiaware(dpiaware)
	, m_status(true)
	, m_enable_tx(Common::GetValue(config, kTX, true))
	, m_current(nullptr)
	, m_cliprect(0,0,4096,4096)
	, m_dither_amount(0.0f)
{
	try
	{
		MS_TRY(CreateDXGIFactory, (__uuidof(IDXGIFactory), (void **)m_factory.WriteAdr()));

		D3D_FEATURE_LEVEL featurelevel = D3D_FEATURE_LEVEL_1_0_CORE;

		MS_TRY(D3D11CreateDevice, (nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, (D3D_DEBUG ? D3D11_CREATE_DEVICE_DEBUG : 0) | D3D11_CREATE_DEVICE_SINGLETHREADED, nullptr, 0, D3D11_SDK_VERSION, m_device.WriteAdr(), &featurelevel, m_devicecontext.WriteAdr()));

		if (!m_devicecontext || featurelevel < D3D_FEATURE_LEVEL_11_0) throw(false);


		auto & device = *m_device;


		//UINT n;

		//m_enable_aa = m_enable_aa && SUCCEEDED(device.CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &n));


		auto rasterizer_desc = Init<D3D11_RASTERIZER_DESC>();

		rasterizer_desc.FillMode = D3D11_FILL_SOLID;
		rasterizer_desc.CullMode = D3D11_CULL_NONE;
		rasterizer_desc.FrontCounterClockwise = true;
		rasterizer_desc.DepthBias = false;
		rasterizer_desc.DepthBiasClamp = 0;
		rasterizer_desc.SlopeScaledDepthBias = 0;
		rasterizer_desc.DepthClipEnable = false;
		rasterizer_desc.ScissorEnable = true;

		rasterizer_desc.MultisampleEnable = false;// m_enable_aa;
		rasterizer_desc.AntialiasedLineEnable = false;

		MS_TRY(device.CreateRasterizerState, (&rasterizer_desc, m_rasterstate.WriteAdr()));

		m_devicecontext->RSSetState(m_rasterstate);


		auto blend_desc = Init<D3D11_BLEND_DESC>();

		blend_desc.RenderTarget[0].BlendEnable = 0;
		blend_desc.AlphaToCoverageEnable = FALSE;
		blend_desc.IndependentBlendEnable = FALSE;
		blend_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blend_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blend_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		blend_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		MS_TRY(device.CreateBlendState, (&blend_desc, m_blendstates[0].WriteAdr()));

		blend_desc.RenderTarget[0].BlendEnable = 1;

		MS_TRY(device.CreateBlendState, (&blend_desc, m_blendstates[1].WriteAdr()));

		m_devicecontext->OMSetBlendState(m_blendstates[1], 0, kMaxUInt32);


		auto sampler_desc = Init<D3D11_SAMPLER_DESC>();

		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		//sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		//sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		//sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		//sampler_desc.BorderColor[0] = 0.0f;
		//sampler_desc.BorderColor[1] = 0.0f;
		//sampler_desc.BorderColor[2] = 0.0f;
		//sampler_desc.BorderColor[3] = 0.0f;
		sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampler_desc.MinLOD = 0;
		sampler_desc.MaxLOD = Float(kMaxInt16);

		MS_TRY(device.CreateSamplerState, (&sampler_desc, m_samplerstates[0].WriteAdr()));

		sampler_desc.MinLOD = -Float(kMaxInt16);
		sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

		MS_TRY(device.CreateSamplerState, (&sampler_desc, m_samplerstates[1].WriteAdr()));


		auto stencil_desc = Init<D3D11_DEPTH_STENCIL_DESC>();

		stencil_desc.DepthEnable = false;
		stencil_desc.StencilEnable = true;
		stencil_desc.StencilReadMask = 0xFF;
		stencil_desc.StencilWriteMask = 0xFF;

		stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
		stencil_desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		stencil_desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		stencil_desc.BackFace = stencil_desc.FrontFace;

		MS_TRY(device.CreateDepthStencilState, (&stencil_desc, m_stencilstates[0].WriteAdr()));


		stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS;
		stencil_desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		stencil_desc.BackFace = stencil_desc.FrontFace;

		MS_TRY(device.CreateDepthStencilState, (&stencil_desc, m_stencilstates[1].WriteAdr()));


		stencil_desc.FrontFace.StencilFunc = D3D11_COMPARISON_GREATER_EQUAL;
		stencil_desc.BackFace = stencil_desc.FrontFace;

		MS_TRY(device.CreateDepthStencilState, (&stencil_desc, m_stencilstates[2].WriteAdr()));



		typedef Pair <const char *> Define;

		Array <Define> defines;

		Array <D3D11_INPUT_ELEMENT_DESC> inputs = { D3D11_INPUT_ELEMENT_DESC {"POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };

		REFLEX_LOOP(idx, kNumShaderType)
		{
			defines.Clear();

			inputs.SetSize(1);

			switch (idx & 3)
			{
			case kShaderTypePoint:
				break;

			case kShaderTypeColourPoint:
				defines.Push({ "COLOUR", "1" });
				inputs.Push({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
				break;

			case kShaderTypeTextureLuminance:
				defines.Push({ "TEXTURE", "1" });
				defines.Push({ "LUMINANCE", "1" });
				inputs.Push({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
				break;

			case kShaderTypeTextureRGBA:
				defines.Push({ "TEXTURE", "1" });
				inputs.Push({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 });
				break;
			}

			defines.Push({ 0, 0 });

			auto & [vertex,pixel] = m_shaders[idx];

			CompileShader(device, defines, vertex);

			CreateInputLayout(device, vertex, inputs);

			CompileShader(device, defines, pixel);
		}


		auto buffer_desc = MakeConstBufferDesc<Pair<fPoint>>();

		MS_TRY(device.CreateBuffer, (&buffer_desc, nullptr, m_viewport.WriteAdr()));


		buffer_desc = MakeConstBufferDesc<Uniforms>();

		MS_TRY(device.CreateBuffer, (&buffer_desc, nullptr, m_matrix.WriteAdr()));


		D3D11_SUBRESOURCE_DATA init_data = {};
		init_data.pSysMem = &kDefaultPixelUniforms;

		buffer_desc = MakeConstBufferDesc<PixelUniforms>();

		MS_TRY(device.CreateBuffer, (&buffer_desc, &init_data, m_colfilter.WriteAdr()));


		TextureFilterUniforms defaultTexFilterData;
		init_data.pSysMem = &defaultTexFilterData;

		buffer_desc = MakeConstBufferDesc<TextureFilterUniforms>();

		MS_TRY(device.CreateBuffer, (&buffer_desc, &init_data, m_texture_nofilter.WriteAdr()));


		globals->m_enable_truefullscreen = true;
	}
	catch (bool)
	{
		m_status = false;
	}
}

Direct3D::~Direct3D()
{
	if (m_devicecontext) m_devicecontext->Flush();	//not valid if m_status = false
}

TRef <Renderer::Canvas> Direct3D::CreateCanvas(void * syswindow)
{
	return REFLEX_CREATE(WindowCanvas, *this, (HWND)syswindow);
}

TRef <Renderer::Canvas> Direct3D::CreateBitmap(bool alphachannel, bool antialias)
{
	if (m_enable_tx)
	{
		return REFLEX_CREATE(Bitmap<true>, *this, alphachannel, antialias);
	}
	else
	{
		return REFLEX_CREATE(Bitmap<false>, *this, alphachannel, antialias);
	}
}

TRef <Renderer::Graphic> Direct3D::CreatePrimitives(PrimitiveType primitive, const ArrayView <fPoint> & points)
{
	if (points)
	{
		auto primitives = REFLEX_CREATE(Primitives<fPoint>, *this, kPrimitiveTypes[primitive], points);

		if (Check("CreatePrimitives", primitives->m_vbo)) return primitives;

		AutoRelease(primitives);
	}

	return Graphic::null;
}

TRef <Renderer::Graphic> Direct3D::CreatePrimitives(PrimitiveType primitive, const ArrayView <ColourPoint> & points)
{
	if (points)
	{
		return REFLEX_CREATE(Primitives<ColourPoint>, *this, kPrimitiveTypes[primitive], points);
	}
	else
	{
		return Graphic::null;
	}
}

void Direct3D::Clear(const Colour & colour)
{
	if (ID3D11RenderTargetView * rtv = m_current->m_rendertargetview) m_devicecontext->ClearRenderTargetView(rtv, &colour.r);
}

void Direct3D::EnableBlend(bool enable)
{
	m_devicecontext->OMSetBlendState(m_blendstates[enable], 0, kMaxUInt32);
}

void Direct3D::SetDitheringAmount(Float amount)
{
	m_dither_amount = amount;

	m_pixel_uniforms.gDitherAmount = amount ? (amount / 255.0f) : 0.0f;

	D3D11_MAPPED_SUBRESOURCE resource;
	
	m_devicecontext->Map(m_colfilter, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	*Reinterpret<PixelUniforms>(resource.pData) = m_pixel_uniforms;

	m_devicecontext->Unmap(m_colfilter, 0);
}

void Direct3D::SetColourTransform(const ArrayView <Float> & matrix_16)
{
	MemCopy(matrix_16.data, m_pixel_uniforms.gColourTransform, sizeof(m_pixel_uniforms.gColourTransform));

	SetDitheringAmount(m_dither_amount);
}

void Direct3D::SetClip(const iRect & rect)
{
	auto & canvas = *m_current;

	auto idpifactor = canvas.m_idpifactor;

	D3D11_RECT & d3drect = m_cliprect;

	d3drect.left = rect.origin.x * idpifactor;
	d3drect.top = rect.origin.y * idpifactor;
	d3drect.right = d3drect.left + (rect.size.w * idpifactor);
	d3drect.bottom = d3drect.top + (rect.size.h * idpifactor);

	m_devicecontext->RSSetScissorRects(1, &d3drect);
}

void Direct3D::SetMask(const Graphic & graphic, const Transform & transform, bool invert)
{
	auto & ctx = *m_devicecontext;

	auto & canvas = *m_current;

	if (!canvas.m_stencil)
	{
		auto pixeldensity = canvas.m_pixeldensity;

		auto stencil_desc = Init<D3D11_TEXTURE2D_DESC>();

		stencil_desc.Width = Max(canvas.m_size.w * pixeldensity, 8);
		stencil_desc.Height = Max(canvas.m_size.h * pixeldensity, 8);
		stencil_desc.MipLevels = 1;
		stencil_desc.ArraySize = 1;
		stencil_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
		stencil_desc.SampleDesc.Count = canvas.m_samplecount;
		stencil_desc.SampleDesc.Quality = 0;
		stencil_desc.Usage = D3D11_USAGE_DEFAULT;
		stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		if (CallChecked<&ID3D11Device::CreateTexture2D>("SetMask", m_device, &stencil_desc, nullptr, canvas.m_stencil.WriteAdr()))
		{
			auto stencilview_desc = Init<D3D11_DEPTH_STENCIL_VIEW_DESC>();

			stencilview_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			stencilview_desc.ViewDimension = (canvas.m_samplecount > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;
			stencilview_desc.Texture2D.MipSlice = 0;

			if (CallChecked<&ID3D11Device::CreateDepthStencilView>("SetMask", m_device, canvas.m_stencil, &stencilview_desc, canvas.m_stencilview.WriteAdr()))
			{
				goto Ok;
			}
		}

		canvas.m_stencil.Clear();
	}

	
	//TODO to avoid warning should switch to null shader

	REFLEX_MARKER(Ok);

	ctx.OMSetDepthStencilState(m_stencilstates[0], 0);

	ctx.OMSetRenderTargets(0, nullptr, canvas.m_stencilview);

	ctx.ClearDepthStencilView(canvas.m_stencilview, D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 0, 0);

	Colour rgba = { 1.0f, 1.0f, 1.0f, 1.0f };

	graphic.Render(transform, rgba);

	ctx.OMSetDepthStencilState(m_stencilstates[1 + invert], 0);

	canvas.SetRenderTargetView(ctx, canvas.m_stencilview);

	//REFLEX_ASSERT(!canvas.m_stenciling);

	canvas.m_stenciling = true;
}

void Direct3D::ClearMask()
{
	auto & canvas = *m_current;

	m_devicecontext->OMSetDepthStencilState(0, 0);

	canvas.SetRenderTargetView(*m_devicecontext, nullptr);

	canvas.m_stenciling = false;
}

Renderer::Canvas * Direct3D::GetCurrent()
{
	return m_current;
}

Renderer::Config Direct3D::GetConfig() const
{
	Config::Item hd = { kHD, m_dpiaware };
	Config::Item tx = { kTX, m_enable_tx };
	//Config::Item aa = { kAA, m_enable_aa };

	return { hd, tx/*, aa */};
}




//
//canvas implementation

Direct3D::AbstractCanvas::AbstractCanvas(Direct3D & engine, UInt samplecount)
	: engine(engine)
	, m_idpifactor(1)
	, m_samplecount(samplecount)
{
	Retain(engine);

	m_size = { 16,16 };

	m_pixeldensity = 1;
}

Direct3D::AbstractCanvas::~AbstractCanvas()
{
	Release(engine);
}

bool Direct3D::AbstractCanvas::SetSize(const iSize & size, Int32 dpifactor)
{
	REFLEX_ASSERT_EX(size.w && size.h, "System::Canvas::SetSize");

	m_size = size;

	m_idpifactor = dpifactor;

	m_pixeldensity = dpifactor;

	return true;
}

const System::iSize & Direct3D::AbstractCanvas::GetSize() const
{
	return m_size;
}

Int Direct3D::AbstractCanvas::GetPixelDensity() const
{
	return m_pixeldensity;
}

REFLEX_INLINE void Direct3D::AbstractCanvas::SetCurrent()
{
	REFLEX_ASSERT(m_rendertargetview);	//should never happen, client responsiblity to SetSize before doing stuff

	engine.m_current = this;

	auto & ctx = *engine.m_devicecontext;


	fSize size = { Float32(m_size.w), Float32(m_size.h) };

	auto fpixeldensity = Float(m_pixeldensity);

	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, size.w * fpixeldensity, size.h * fpixeldensity, 0.0f, 1.0f };

	ctx.RSSetViewports(1, &viewport);

	SetRenderTargetView(ctx, nullptr);


	D3D11_RECT rect;

	rect.left = 0;
	rect.top = 0;
	rect.right = m_size.w * m_pixeldensity;
	rect.bottom = m_size.h * m_pixeldensity;

	ctx.RSSetScissorRects(1, &rect);


	for (auto & [vertex, pixel] : engine.m_shaders)
	{
		ctx.VSSetShader(vertex.sh, nullptr, 0);


		D3D11_MAPPED_SUBRESOURCE resource;

		ctx.Map(engine.m_viewport, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

		auto & vp = *Reinterpret<Pair<fPoint, fSize>>(resource.pData);

		vp.a = { -(size.w * 0.5f), -(size.h * 0.5f) };

		vp.b = { size.w * 0.5f, size.h * -0.5f };

		ctx.Unmap(engine.m_viewport, 0);


		ctx.VSSetConstantBuffers(0, 1, engine.m_viewport.ReadAdr());

		ctx.VSSetConstantBuffers(1, 1, engine.m_matrix.ReadAdr());

		ctx.PSSetConstantBuffers(0, 1, engine.m_colfilter.ReadAdr());
	}

	engine.m_shader = kNumShaderType;
}

Direct3D::WindowCanvas::WindowCanvas(Direct3D & engine, HWND hwnd)
	: AbstractCanvas(engine, /*engine.m_enable_aa ? 4 :*/ 1)
	, m_hwnd(hwnd)
{
	m_size.w = 16;

	m_size.h = 16;

	auto swp_desc = Init<DXGI_SWAP_CHAIN_DESC>();

	swp_desc.BufferCount = 1;
	swp_desc.OutputWindow = hwnd;
	swp_desc.SampleDesc.Count = m_samplecount;
	swp_desc.SampleDesc.Quality = 0;
	swp_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swp_desc.BufferDesc.Format  = DXGI_FORMAT_R8G8B8A8_UNORM;
	swp_desc.BufferDesc.RefreshRate.Denominator = 1;
	swp_desc.BufferDesc.RefreshRate.Numerator = 0;
	//swp_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swp_desc.Windowed = true;

	D3D_CHECK(engine.m_factory->MakeWindowAssociation, (hwnd, DXGI_MWA_NO_ALT_ENTER));

	D3D_CHECK(engine.m_factory->CreateSwapChain, (engine.m_device, &swp_desc, m_swapchain.WriteAdr()));

	if (!m_swapchain) m_swapchain = &D3dNullSwapChain::self;

	if (auto implementation = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA)))
	{
		implementation->m_swapchainhack = m_swapchain;
	}
}

bool Direct3D::WindowCanvas::SetSize(const iSize & size, Int32 dpifactor)
{
	AbstractCanvas::SetSize(size, dpifactor);

	auto & ctx = *engine.m_devicecontext;

	m_stencilview.Clear();

	m_stencil.Clear();

	m_rendertargetview.Clear();

	IDXGISwapChain * null_swapchain = &D3dNullSwapChain::self;

	if (m_swapchain != null_swapchain)
	{
		ID3D11RenderTargetView * null[] = { nullptr };

		ctx.OMSetRenderTargets(1, null, nullptr);

		ctx.Flush();

		D3D_CHECK(m_swapchain->ResizeBuffers, (0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

		ComPtr <ID3D11Texture2D> framebuffer;

		D3D_CHECK(m_swapchain->GetBuffer, (0, __uuidof(ID3D11Texture2D), (void **)framebuffer.WriteAdr()));

		D3D_CHECK(engine.m_device->CreateRenderTargetView, (framebuffer, 0, m_rendertargetview.WriteAdr()));

		D3D_CHECK(engine.m_factory->MakeWindowAssociation, (m_hwnd, DXGI_MWA_NO_ALT_ENTER));
	}

	return true;
}

void Direct3D::WindowCanvas::Flush()
{
	REFLEX_ASSERT(engine.m_current == this);

	m_swapchain->Present(0, DXGI_PRESENT_DO_NOT_WAIT);
}

template <bool RENDER_TARGET> Direct3D::Bitmap<RENDER_TARGET>::Bitmap(Direct3D & engine, bool alphachannel, bool aa)
	: AbstractCanvas(engine, 1)
	, m_alphachannel(alphachannel)
	, m_aa(aa)
	, m_rendertype(kShaderTypeTextureRGBA)
	, m_samplerstate(engine.m_samplerstates[aa])
{
}

template <bool RENDER_TARGET> bool Direct3D::Bitmap<RENDER_TARGET>::SetSize(const iSize & insize, Int32 dpifactor)
{
	constexpr auto desc = "Bitmap::SetSize";

	REFLEX_ASSERT(!m_stenciling);

	AbstractCanvas::SetSize(insize, dpifactor);

	if constexpr (RENDER_TARGET)
	{
		m_stencilview.Clear();

		m_stencil.Clear();

		m_rendertargetview.Clear();

		m_textureview.Clear();

		m_texture.Clear();


		auto & device = *engine.m_device;

		auto texture_desc = Init<D3D11_TEXTURE2D_DESC>();

		texture_desc.Width = m_size.w * dpifactor;
		texture_desc.Height = m_size.h * dpifactor;
		texture_desc.MipLevels = m_aa ? 0 : 1;
		texture_desc.ArraySize = 1;
		texture_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texture_desc.SampleDesc.Count = m_samplecount;
		texture_desc.Usage = D3D11_USAGE_DEFAULT;
		texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texture_desc.MiscFlags = m_aa ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		if (CallChecked<&ID3D11Device::CreateTexture2D>(desc, device, &texture_desc, nullptr, m_texture.WriteAdr()))
		{
			auto rendertargetview_desc = Init<D3D11_RENDER_TARGET_VIEW_DESC>();

			rendertargetview_desc.Format = texture_desc.Format;
			rendertargetview_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rendertargetview_desc.Texture2D.MipSlice = 0;

			if (CallChecked<&ID3D11Device::CreateRenderTargetView>(desc, device, m_texture, &rendertargetview_desc, m_rendertargetview.WriteAdr()))
			{
				if (CallChecked<&ID3D11Device::CreateShaderResourceView>(desc, device, m_texture, nullptr, m_textureview.WriteAdr()))
				{
					return true;
				}
			}

			m_textureview.Clear();

			m_rendertargetview.Clear();

			m_texture.Clear();
		}

		return false;
	}
	else
	{
		return true;
	}
}

template <bool RENDER_TARGET> void Direct3D::Bitmap<RENDER_TARGET>::Write(ImageFormat format, const ArrayView <UInt8> & data)
{
	constexpr auto desc = "Bitmap::Write";

	REFLEX_ASSERT(!m_stenciling);

	if (Common::VerifyBitmap({ m_size, m_pixeldensity, format }, data) && kBPP[format] != 3)
	{
		m_stencilview.Clear();

		m_stencil.Clear();

		m_rendertargetview.Clear();

		m_textureview.Clear();

		m_texture.Clear();


		auto & device = *engine.m_device;

		auto & d3dformat = kImageFormats[format];

		auto texture_desc = Init<D3D11_TEXTURE2D_DESC>();

		texture_desc.Width = m_size.w * m_pixeldensity;
		texture_desc.Height = m_size.h * m_pixeldensity;
		texture_desc.MipLevels = m_aa ? 0 : 1;
		texture_desc.ArraySize = 1;
		texture_desc.Format = d3dformat.a;
		texture_desc.SampleDesc.Count = m_samplecount;
		texture_desc.Usage = D3D11_USAGE_DEFAULT;	//D3D11_USAGE_IMMUTABLE;
		texture_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		texture_desc.MiscFlags = m_aa ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

		m_rendertype = d3dformat.b;

		if (CallChecked<&ID3D11Device::CreateTexture2D>(desc, device, &texture_desc, nullptr, m_texture.WriteAdr()))	//setting data direct requires all mips pre-generated
		{
			engine.m_devicecontext->UpdateSubresource(m_texture, 0, 0, data.data, kBPP[format] * (texture_desc.Width), 0);

			if (CallChecked<&ID3D11Device::CreateShaderResourceView>(desc, device, m_texture, nullptr, m_textureview.WriteAdr()))
			{
				if (m_aa) engine.m_devicecontext->GenerateMips(m_textureview);

				return;
			}
		}

		m_textureview.Clear();

		m_texture.Clear();
	}
}

template <bool RENDER_TARGET> TRef <Renderer::Graphic> Direct3D::Bitmap<RENDER_TARGET>::CreateTextures(const ArrayView < Pair <fRect> >& rects) const
{
	REFLEX_ASSERT(rects);

	if (m_textureview)
	{
		Textures* textures = REFLEX_CREATE(Textures, *this, rects);

		if (Check("Bitmap::CreateTextures", textures->m_vbo)) return textures;

		AutoRelease(textures);
	}

	return REFLEX_CREATE(InvalidTextures, *this);	//prevent memory leak, GLX expect Texture to retain owning Bitmap
}

template <bool RENDER_TARGET> TRef <Renderer::Graphic> Direct3D::Bitmap<RENDER_TARGET>::CreateTexturesWithFilter(const ArrayView < Pair <fRect> > & rects, FilterMode mode, const ArrayView <Float>& parameters) const
{
	REFLEX_ASSERT(rects);

	if (m_textureview)
	{
		FilteredTextures* textures = REFLEX_CREATE(FilteredTextures, *this, rects, mode, parameters);

		if (Check("Bitmap::CreateTexturesWithFilter", textures->m_status)) return textures;

		AutoRelease(textures);
	}

	return REFLEX_CREATE(InvalidTextures, *this);	//prevent memory leak, GLX expect Texture to retain owning Bitmap
}

template <bool RENDER_TARGET> void Direct3D::Bitmap<RENDER_TARGET>::Flush()
{
	if constexpr (RENDER_TARGET)
	{
		REFLEX_ASSERT(engine.m_current == this);

		if (m_textureview && m_aa) engine.m_devicecontext->GenerateMips(m_textureview);

		engine.m_current = nullptr;
	}
}

//template <bool RENDER_TARGET> RawBitmap Direct3D::Bitmap<RENDER_TARGET>::ExportBitmap() const
//{
//	ComPtr <ID3D11Texture2D> texture;
//
//
//	D3D11_TEXTURE2D_DESC description;
//
//	RemoveConst(m_texture)->GetDesc(&description);
//
//	description.BindFlags = 0;
//	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
//	description.Usage = D3D11_USAGE_STAGING;
//	description.MiscFlags = 0;
//
//	{
//		auto hr = engine.m_device->CreateTexture2D(&description, nullptr, &texture);
//
//		if (FAILED(hr)) return {};
//	}
//
//	{
//		engine.m_devicecontext->CopyResource(texture, RemoveConst(m_texture));
//	}
//
//
//	D3D11_MAPPED_SUBRESOURCE mapped;
//	{
//		auto hr = engine.m_devicecontext->Map(texture, 0, D3D11_MAP_READ, 0, &mapped);
//
//		if (FAILED(hr)) return {};
//	}
//
//
//	RawBitmap bitmap;
//
//	auto & out = bitmap.b;
//
//	UInt32 density = m_pixeldensity * m_pixeldensity;
//
//	UInt32 bpp = kBPP[kImageFormatRGB] * density;
//
//	out.SetSize(m_size.w * m_size.h * bpp);
//
//
//	bitmap.a.size = m_size;
//	bitmap.a.pixdensity = m_pixeldensity;
//	bitmap.a.format = kImageFormatRGB;
//
//	UInt idx = 0;
//
//	if (description.Format == DXGI_FORMAT_R32G32B32A32_FLOAT)
//	{
//		auto pixels = Reinterpret<Float32>(mapped.pData);
//
//		REFLEX_LOOP(y, m_size.h * m_pixeldensity)
//		{
//			REFLEX_LOOP(x, m_size.w * m_pixeldensity)
//			{
//				auto r = *pixels++;
//				auto g = *pixels++;
//				auto b = *pixels++;
//
//				out[idx++] = Truncate(r * 255.0f);
//				out[idx++] = Truncate(g * 255.0f);
//				out[idx++] = Truncate(b * 255.0f);
//			}
//		}
//	}
//	else if (false/*description.Format == DXGI_FORMAT_R32G32B32A32_FLOAT*/)
//	{
//		auto pixels = Reinterpret<Float32>(mapped.pData);
//
//		REFLEX_LOOP(y, m_size.h * m_pixeldensity)
//		{
//			REFLEX_LOOP(x, m_size.w * m_pixeldensity)
//			{
//				auto r = *pixels++;
//				auto g = *pixels++;
//				auto b = *pixels++;
//				auto a = *pixels++;
//
//				out[idx++] = Truncate(r * 255.0f);
//				out[idx++] = Truncate(g * 255.0f);
//				out[idx++] = Truncate(b * 255.0f);
//			}
//		}
//	}
//
//	return bitmap;
//}

REFLEX_INLINE Direct3D::AbstractVBO::AbstractVBO(Direct3D & engine, UInt nvertex)
	: engine(engine)
	, m_nvertex(nvertex)
{
	Retain(engine);
}

REFLEX_INLINE Direct3D::AbstractVBO::~AbstractVBO()
{
	Release(engine);
}

REFLEX_INLINE void Direct3D::AbstractVBO::Draw(ID3D11DeviceContext & ctx, const Transform & t, const Colour & colour, D3D_PRIMITIVE_TOPOLOGY type, UINT vertex_stride) const
{
	D3D11_MAPPED_SUBRESOURCE resource;

	ctx.Map(engine.m_matrix, 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);

	auto & uniforms = *Reinterpret<Uniforms>(resource.pData);

	uniforms.gOffset = t.origin;
	uniforms.gScale = t.scale;
	uniforms.gColour = colour;
	uniforms.gColour.a = colour.a * t.opacity;

	ctx.Unmap(engine.m_matrix, 0);

	UINT vertex_offset = 0;

	ctx.IASetVertexBuffers(0, 1, m_vbo.ReadAdr(), &vertex_stride, &vertex_offset);

	ctx.IASetPrimitiveTopology(type);

	ctx.Draw(m_nvertex, 0);
}

template <class POINT> REFLEX_INLINE Direct3D::Primitives<POINT>::Primitives(Direct3D & engine, D3D11_PRIMITIVE_TOPOLOGY type, const typename ArrayView <POINT> & points)
	: AbstractVBO(engine, points.size)
	, m_type(type)
{
	auto vbo_desc = Init<D3D11_BUFFER_DESC>();

	vbo_desc.ByteWidth = m_nvertex * sizeof(POINT);

	vbo_desc.Usage = D3D11_USAGE_IMMUTABLE;

	vbo_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbo_data = { points.data };

	engine.m_device->CreateBuffer(&vbo_desc, &vbo_data, RemoveConst(m_vbo).WriteAdr());
}

template <class POINT> void Direct3D::Primitives<POINT>::Render(const Transform & t, const Colour & colour) const
{
	auto & ctx = *engine.m_devicecontext;

	engine.SelectShader(IsType<POINT,ColourPoint>::value ? kShaderTypeColourPoint : kShaderTypePoint);

	Draw(ctx, t, colour, m_type, sizeof(POINT));
}

Direct3D::Textures::Textures(const AbstractCanvas & canvas, const ArrayView < Pair <fRect> > & rects)
	: AbstractVBO(RemoveConst(canvas.engine), rects.size * 6)
	, m_bitmap(canvas)
{
	auto bitmap = Cast< Bitmap<false> >(this->m_bitmap);

	auto & engine = RemoveConst(bitmap->engine);

	auto & points = engine.m_points_buffer;

	auto & tc = points;

	points.Clear();

	Float32 fw = Reciprocal(Float32(bitmap->m_size.w));

	Float32 fh = Reciprocal(Float32(bitmap->m_size.h));

	for (auto & rect : rects)
	{
		auto & src = rect.a;

		auto & dst = rect.b;

		auto & src_origin = src.origin;

		auto & src_size = src.size;

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

		points.Push({ dst_x1, dst_y1 });
		tc.Push({ src_x1, src_y1 });

		points.Push({ dst_x2, dst_y1 });
		tc.Push({ src_x2, src_y1 });

		points.Push({ dst_x1, dst_y2 });
		tc.Push({ src_x1, src_y2 });

		points.Push({ dst_x1, dst_y2 });
		tc.Push({ src_x1, src_y2 });

		points.Push({ dst_x2, dst_y2 });
		tc.Push({ src_x2, src_y2 });

		points.Push({ dst_x2, dst_y1 });
		tc.Push({ src_x2, src_y1 });
	}

	auto vbo_desc = Init<D3D11_BUFFER_DESC>();

	vbo_desc.ByteWidth = m_nvertex * sizeof(Pair<fPoint>);	//16

	vbo_desc.Usage = D3D11_USAGE_IMMUTABLE;

	vbo_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbo_data = { points.GetData() };

	engine.m_device->CreateBuffer(&vbo_desc, &vbo_data, RemoveConst(m_vbo).WriteAdr());
}

void Direct3D::Textures::Render(const Transform & t, const Colour & colour) const
{
	auto bitmap = Cast< Bitmap <false> >(*this->m_bitmap);

	auto & engine = bitmap->engine;

	auto & ctx = *engine.m_devicecontext;

	engine.SelectShader(bitmap->m_rendertype);

	ctx.PSSetShaderResources(0, 1, bitmap->m_textureview.ReadAdr());

	if (SetFiltered(engine.m_samplerstate_z, bitmap->m_samplerstate))
	{
		ctx.PSSetSamplers(0, 1, &engine.m_samplerstate_z);
	}

	Draw(ctx, t, colour, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, sizeof(Pair<fPoint>));
}

Direct3D::FilteredTextures::FilteredTextures(const AbstractCanvas& bitmap, const ArrayView<Pair<fRect>>& rects, FilterMode mode, const ArrayView<Float>& in_params)
	: Textures(bitmap, rects)
{
	if (!m_vbo) return;

	constexpr char desc[] = "Direct3D::FilteredTextures::FilteredTextures";

	auto& device = *engine.m_device;

	auto buffer_desc = MakeConstBufferDesc<TextureFilterUniforms>();

	TextureFilterUniforms filter_constants;
	Float* out_params = filter_constants.gFilterParams;

	filter_constants.gFilterMode = mode;

	auto widthUnits = Float(bitmap.m_size.w);
	auto heightUnits = Float(bitmap.m_size.h);

	switch (mode)
	{
	case kFilterModePixelate:
		REFLEX_ASSERT(in_params.size == 2);

		out_params[0] = in_params[0] / widthUnits;
		out_params[1] = in_params[1] / heightUnits;
		break;

	case kFilterModeBlur:
	{
		// out_params => direction.xy: float[2], radius: float, gaussian_weights: float[radius + 1]
		REFLEX_ASSERT(in_params.size >= 4);

		filter_constants.gFilterParams[0] = in_params[0] / widthUnits;
		filter_constants.gFilterParams[1] = in_params[1] / heightUnits;
		filter_constants.gFilterParams[2] = in_params[2];

		UInt tblsize = UInt(in_params[2]) + 1;

		// Then the blur coeffs
		auto gaussian_weights = Mid(in_params, 3);

		REFLEX_ASSERT(gaussian_weights.size == tblsize);

		D3D11_BUFFER_DESC buffer_desc = {
			.ByteWidth = sizeof(FilterParam) * tblsize,
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_SHADER_RESOURCE,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			.StructureByteStride = sizeof(FilterParam),
		};

		D3D11_SUBRESOURCE_DATA init_data = {
			.pSysMem = gaussian_weights.data
		};

		if (!CallChecked<&ID3D11Device::CreateBuffer>(desc, device, &buffer_desc, &init_data, m_filter_dynamic.WriteAdr()))
		{
			return;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC view_desc = {
			.Format = DXGI_FORMAT_UNKNOWN,
			.ViewDimension = D3D11_SRV_DIMENSION_BUFFER,
			.Buffer = {
				.FirstElement = 0,
				.NumElements = gaussian_weights.size
			}
		};

		if (!CallChecked<&ID3D11Device::CreateShaderResourceView>(desc, device, *m_filter_dynamic.ReadAdr(), &view_desc, m_filter_dynamic_view.WriteAdr()))
		{
			return;
		}
		break;
	}

	default:
		DEV_ERROR("Unsupported filter type");
		return;
	}

	D3D11_SUBRESOURCE_DATA init_data = {
		.pSysMem = &filter_constants
	};

	m_status = CallChecked<&ID3D11Device::CreateBuffer>(desc, device, &buffer_desc, &init_data, m_filter_constants.WriteAdr());
}

inline void Direct3D::FilteredTextures::Render(const Transform& transform, const Colour& colour) const
{
	auto& engine = Cast< Bitmap <false> >(*this->m_bitmap)->engine;

	auto& ctx = *engine.m_devicecontext;

	ctx.PSSetConstantBuffers(1, 1, m_filter_constants.ReadAdr());
	if (m_filter_dynamic_view)	ctx.PSSetShaderResources(2, 1, m_filter_dynamic_view.ReadAdr());

	Textures::Render(transform, colour);

	ctx.PSSetConstantBuffers(1, 1, engine.m_texture_nofilter.ReadAdr());
}

REFLEX_INLINE void Direct3D::SelectShader(ShaderType type)
{
	if (SetFiltered(m_shader, type))
	{
		auto & ctx = *m_devicecontext;

		auto & [vertex, pixel] = m_shaders[type];

		ctx.VSSetShader(vertex.sh, nullptr, 0);

		ctx.PSSetShader(pixel.sh, nullptr, 0);

		ctx.IASetInputLayout(vertex.layout);

		m_samplerstate_z = 0;
	}
}

template <class TYPE> void Direct3D::CompileShader(ID3D11Device & device, const ArrayView <Pair<const char *>> & defines, Shader <TYPE> & shader)
{
	typedef Reflex::IsType <TYPE,ID3D11VertexShader> VERTEX;

	ComPtr <ID3DBlob> error;

	const char * type = VERTEX::value ? "vs_5_0" : "ps_5_0";

	const char * entry = VERTEX::value ? "VertexMain" : "PixelMain";

	auto debug = D3D_DEBUG ? D3DCOMPILE_DEBUG : 0;

	if constexpr (REFLEX_DEBUG && false)	//load shader from disk for development
	{
		auto filepath = ToWString(ToView(__FILE__));

		auto parts = Split(filepath, L'\\');

		parts.Shrink(2);

		parts.Append({ L"resources", L"direct3d.hlsl"});

		auto path = Merge(parts, kPathDelimiter);

		auto file = Make<System::FileHandle>(path);

		if (auto file_size = UInt32(file->GetSize()))
		{
			Array <UInt8> blob(file_size);

			file->Read(blob.GetData(), blob.GetSize());

			MS_TRY(D3DCompile, (blob.GetData(), blob.GetSize(), "direct3d.hlsl", Reinterpret<D3D_SHADER_MACRO>(defines.data), D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, type, D3DCOMPILE_ENABLE_STRICTNESS | debug, 0, shader.blob.WriteAdr(), error.WriteAdr()));

			goto Ok;
		}
	}

	MS_TRY(D3DCompile, (kDirect3dShader.data, kDirect3dShader.size, "direct3d.hlsl", Reinterpret<D3D_SHADER_MACRO>(defines.data), D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, type, D3DCOMPILE_ENABLE_STRICTNESS | debug, 0, shader.blob.WriteAdr(), error.WriteAdr()));

	goto Ok;
	REFLEX_MARKER(Ok);

	if constexpr (VERTEX::value)
	{
		MS_TRY(device.CreateVertexShader, (shader.blob->GetBufferPointer(), shader.blob->GetBufferSize(), nullptr, (ID3D11VertexShader**)shader.sh.WriteAdr()));
	}
	else
	{
		MS_TRY(device.CreatePixelShader, (shader.blob->GetBufferPointer(), shader.blob->GetBufferSize(), nullptr, (ID3D11PixelShader**)shader.sh.WriteAdr()));
	}
}

template <class TYPE> bool Direct3D::CreateInputLayout(ID3D11Device & device, Shader <TYPE> & shader, const ArrayView <D3D11_INPUT_ELEMENT_DESC> & desc)
{
	MS_TRY(device.CreateInputLayout, (desc.data, desc.size, shader.blob->GetBufferPointer(), shader.blob->GetBufferSize(), shader.layout.WriteAdr()));

	return true;
}

template <class TYPE> D3D11_BUFFER_DESC Direct3D::MakeConstBufferDesc()
{
	REFLEX_STATIC_ASSERT(((sizeof(TYPE) / 16) * 16) == sizeof(TYPE));

	auto desc = Init<D3D11_BUFFER_DESC>();

	desc.ByteWidth = sizeof(TYPE);

	desc.Usage = D3D11_USAGE_DYNAMIC; // mappable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // mappable ? D3D11_CPU_ACCESS_WRITE : 0;

	return desc;
}

TRef <Renderer> CreateDirect3D(const Renderer::Config & config)
{
	return REFLEX_CREATE(Direct3D, InstantiateDpiAwareness(config), config);
}

const Common::RendererFactory g_direct3d_factory(Direct3D::kEngineName, &CreateDirect3D);

REFLEX_END_INTERNAL
