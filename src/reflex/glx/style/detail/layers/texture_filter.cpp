#include "colour_filter.h"



//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct AbstractTextureFilter
{
	REFLEX_DECLARE_KEY32(Blur);
	REFLEX_DECLARE_KEY32(Pixelate);

	struct Properties final : public StandardWrapperProperties
	{
		Size params;
		Float opacity = 1.0f;
		Point offset;
	};

	struct Scratch final : public StandardWrapperScratch
	{
		Size params_z;

		System::iSize texture_size;

		System::fPoint origin;		//blur only
		Size padded_size;			//blur only
		Float samplingfactor = 1;	//blur only
		bool rebuild = true;		//blur only

		Reference <System::Renderer::Canvas> bitmap;
		Reference <Graphic> drawable_or_blurrer;
	};

	static void OnBind(const GenericLayer & layer, GenericLayer::ObjectState & data, Object & object)
	{
		auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

		scratch->bitmap = Core::g_renderer->CreateBitmap(true, false);

		AllocateLayers(object, properties->layers, scratch->content_state);
	}

	static System::iSize ComputeTextureSize(System::fSize size)
	{
		constexpr Int32 kWrap = Int32(kMaxTextureSize - 1);

		return { Truncate(size.w + Core::kRoundingTolerance) & kWrap, Truncate(size.h + Core::kRoundingTolerance) & kWrap };
	}

	static constexpr UInt kPixelDensity = 1; // MEMO: lower precision on retina screens for performance
};

struct BlurImpl : public AbstractTextureFilter
{
	struct Renderer;

	static void Init(GenericPropertiesSchema & schema);
};

struct BlurImpl::Renderer : public Graphic
{
	Renderer(const Properties & properties, Scratch & scratch, Float radius, UInt iradius, Size downscaled_size, System::iSize texture_size, Int32 pixel_density)
		: m_opacity(properties.opacity)
		, m_redraw(true)
	{
		Float sigma = radius / 3.0f;

		const ArrayView <Float> renderer_params_view = { m_renderer_params, 3 + iradius + 1 };

		m_renderer_params[0] = 1; // horizontal pass
		m_renderer_params[1] = 0;
		m_renderer_params[2] = radius;

		CalcGaussianWeights(iradius, sigma, m_renderer_params + 3);

		Pair <Rect> rects = { { {}, downscaled_size }, { {}, downscaled_size } };

		m_drawable_firstpass = scratch.bitmap->CreateTexturesWithFilter({ &rects, 1 }, System::Renderer::kFilterModeBlur, renderer_params_view);

		m_interm_bitmap[0] = Core::g_renderer->CreateBitmap(true, false);
		m_interm_bitmap[0]->SetSize(texture_size, pixel_density);

		m_interm_bitmap[1] = Core::g_renderer->CreateBitmap(true, true);
		m_interm_bitmap[1]->SetSize(texture_size, pixel_density);

		m_renderer_params[0] = 0;
		m_renderer_params[1] = 1; // vertical pass

		m_interm_drawable[0] = m_interm_bitmap[0]->CreateTexturesWithFilter({ &rects, 1 }, System::Renderer::kFilterModeBlur, renderer_params_view);

		rects.b.origin = properties.offset + scratch.origin;
		rects.b.size = scratch.padded_size;

		m_interm_drawable[1] = m_interm_bitmap[1]->CreateTextures({ &rects, 1 });
	}

	void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		constexpr System::Renderer::Transform identity;

		if (SetFiltered(m_redraw, false))
		{
			auto renderer = Core::g_renderer;

			renderer->SetColourTransform({ st_no_colour_filter.data->GetData(), 16 });

			auto & ctx = *Core::RenderContext::st_current;

			MaskLayer::Scope <false> disable_mask_scope(renderer, nullptr);

			//// Horizontal pass
			//{
			//	RenderTargetScope scope(ctx, m_interm_bitmap[0], kTransparent);

			//	m_drawable_firstpass->Render(identity, kWhite);
			//}

			//// Vertical pass
			//{
			//	RenderTargetScope scope(ctx, m_interm_bitmap[1], kTransparent);

			//	m_interm_drawable[0]->Render(identity, kWhite);
			//}

			//above code optimised with one RenderTargetScope

			RenderTargetScope scope(ctx, m_interm_bitmap[1], kTransparent);	//m_interm_bitmap[1] will be set current at end

			scope.canvas = m_interm_bitmap[0].Adr();

			scope.canvas->SetCurrent();

			renderer->Clear(kTransparent);

			m_drawable_firstpass->Render(identity, kWhite);

			scope.canvas->Flush();

			scope.canvas = m_interm_bitmap[1].Adr();

			scope.canvas->SetCurrent();

			m_interm_drawable[0]->Render(identity, kWhite);

			auto & previous_stack_entry = *ColourFilterImpl::Renderer::st_current;

			renderer->SetColourTransform({ previous_stack_entry.matrix.data->GetData(), 16 });
		}

		auto t = colour;

		t.a *= m_opacity;

		m_interm_drawable[1]->Render(transform, t);
	}

	static void CalcGaussianWeights(UInt radius, Float sigma, Float * weights)
	{
		if (radius)
		{
			auto inv_sigma2 = 1.0f / (sigma * sigma);
			auto r1 = Exp(-0.5f * inv_sigma2);	// exp(-1/(2*sigma^2))
			auto q = Exp(-inv_sigma2);			// exp(-1/(sigma^2))

			Float sum = 1.0f;
			weights[0] = 1.0f;

			Float r = r1;
			Float w_prev = 1.0f;

			for (UInt i = 1; i <= radius; ++i)
			{
				auto wi = w_prev * r;	// w[i] = w[i-1] * r_i
				weights[i] = wi;
				sum += wi * 2.0f;
				w_prev = wi;
				r *= q;					// r_{i+1} = r_i * q
			}

			auto inv_sum = 1.0f / sum;

			for (UInt i = 0; i <= radius; ++i) weights[i] *= inv_sum;
		}
		else
		{
			weights[0] = 1.0f;
		}
	}


	const Float m_opacity;

	Reference <Graphic> m_drawable_firstpass;
	Reference <System::Renderer::Canvas> m_interm_bitmap[2];
	Reference <Graphic> m_interm_drawable[2];

	mutable bool m_redraw;

	Float m_renderer_params[4];	//!last, varadic size

	static inline const Matrix4x4 st_no_colour_filter;
};

void BlurImpl::Init(GenericPropertiesSchema & schema)
{
	RegisterStandardWrapper(schema);

	RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, params), "radius");
	RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, opacity), "opacity");
	RegisterPointProperty(schema, REFLEX_OFFSETOF(Properties, offset), "offset");

	SetLayerInitFn(schema, [](GenericLayer & self, const void* pproperties, void * pscratch, GenericLayer::VTable & vtable)
	{
		auto properties = Cast<Properties>(pproperties);

		PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

		vtable.OnBind = &OnBind;

		vtable.OnAccommodate = &StandardWrapperAccommodate;

		vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
		{
			auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

			Float samplingfactor = 1; // no need for an intermediate texture by default

			auto radius = properties->params.w;

			scratch->origin = { -radius, -radius };


			Size padded_size = size + Reflex::MakeSize(radius * 2.0f);

			while (radius > 16.0f)
			{
				samplingfactor *= 0.5f;
				radius *= 0.5f;
			}

			Size downscaled_size = padded_size * samplingfactor;

			auto texture_size = Max(ComputeTextureSize(downscaled_size), System::Renderer::Canvas::kMinValidSize);

			scratch->padded_size = padded_size;
			scratch->samplingfactor = samplingfactor;

			bool texturesize_changed = SetFiltered(scratch->texture_size, texture_size);
			bool radius_changed = SetFiltered(scratch->params_z.w, radius);
			
			if (texturesize_changed || radius_changed)
			{
				scratch->rebuild = true;
			}
			else
			{
				Cast<Renderer>(scratch->drawable_or_blurrer)->m_redraw = true;
			}

			AlignLayers(scratch->content_state, size, contenth);
		};

		vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
		{
			auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

			if (scratch->texture_size.w * scratch->texture_size.h)
			{
				auto ctx = Core::RenderContext::st_current;

				auto pixel_density = ctx->canvas->GetPixelDensity();

				auto samplingfactor = scratch->samplingfactor;

				if (SetFiltered(scratch->rebuild, false))
				{
					auto radius = scratch->params_z.w;

					scratch->bitmap->SetSize(scratch->texture_size, pixel_density);

					UInt iradius = Truncate(radius);

					Size downscaled_size = scratch->padded_size * samplingfactor;

					auto texture_size = scratch->texture_size;

					scratch->drawable_or_blurrer = Reflex::Detail::Constructor<Renderer>::CreateVariableSize(Reflex::g_default_allocator, iradius * sizeof(Float), properties, scratch, radius, iradius, downscaled_size, texture_size, pixel_density);
				}

				RenderTargetScope scope(*ctx, scratch->bitmap, kTransparent);

				auto origin_adjusted = -scratch->origin.x * samplingfactor;
				scope.transform.origin = { origin_adjusted, origin_adjusted };
				scope.transform.scale = Reflex::MakeSize(samplingfactor);

				RedrawLayers(scratch->content_state, kNormal);

				DrawLayers(scope, kWhite, scratch->content_state);

				Cast<Renderer>(scratch->drawable_or_blurrer)->m_redraw = true;

				return scratch->drawable_or_blurrer;
			}
			else
			{
				return Null<System::Renderer::Graphic>();
			}
		};
	});
}

struct PixelateImpl : public AbstractTextureFilter
{
	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapper(schema);

		RegisterSizeProperty(schema, REFLEX_OFFSETOF(Properties, params), "size");

		SetLayerInitFn(schema, [](GenericLayer & self, const void* pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

			vtable.OnBind = &OnBind;

			vtable.OnAccommodate = &StandardWrapperAccommodate;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				auto params = properties->params;

				auto texture_size = ComputeTextureSize(size);

				bool size_changed = SetFiltered(scratch->texture_size, texture_size);
				bool params_changed = SetFiltered(scratch->params_z, params);

				if (size_changed || params_changed)
				{
					Pair <Rect> rects = { { {}, size }, { {}, size } };

					scratch->bitmap->SetSize(Max(texture_size, System::Renderer::Canvas::kMinValidSize), kPixelDensity);

					rects.b.origin = {};

					scratch->drawable_or_blurrer = scratch->bitmap->CreateTexturesWithFilter({ &rects, 1 }, System::Renderer::kFilterModePixelate, { &properties->params.w, 2 });
				}

				AlignLayers(scratch->content_state, size, contenth);
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				if (scratch->texture_size.w * scratch->texture_size.h)
				{
					RenderTargetScope scope(*Core::RenderContext::st_current, scratch->bitmap, kTransparent);

					RedrawLayers(scratch->content_state, kNormal);

					DrawLayers(scope, kWhite, scratch->content_state);
				}
				else
				{
					return Null<System::Renderer::Graphic>();
				}

				return scratch->drawable_or_blurrer;
			};
		});
	}
};

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_texture_fx_layers[]
{
	{ "Blur", &GenericLayer::CreateSchema<BlurImpl>, &GenericLayer::Create<BlurImpl> },
	{ "Pixelate", &GenericLayer::CreateSchema<PixelateImpl>, &GenericLayer::Create<PixelateImpl> },
};
