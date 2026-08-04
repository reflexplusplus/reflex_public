#include "render.h"




//
//special

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct RenderImpl
{
	struct Properties : public StandardWrapperProperties
	{
		Float density = 0.0f;
		Margin pad;
	};

	struct Scratch : public StandardWrapperScratch
	{
		Reference <System::Renderer::Canvas> bitmap;

		Size size_z;

		Reference <System::Renderer::Graphic> image;

		Point origin;

		Int32 valid_size = false;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapper(schema);

		RegisterMarginProperty(schema, REFLEX_OFFSETOF(Properties, pad), "pad");

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, density), "density");

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);
			
			vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				scratch->bitmap = Core::g_renderer->CreateBitmap(true, true);

				AllocateLayers(object, properties->layers, scratch->content_state);
			};

			vtable.OnAccommodate = &StandardWrapperAccommodate;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				if (SetFiltered(scratch->size_z, size))
				{
					auto origin = Reinterpret<Point>(properties->pad.near);

					scratch->origin = origin;

					Int mult = Truncate(RoundUpPow2(properties->density, 1.0f));	//Truncate(RoundUp(scale))

					auto pixel_density = Clip<UInt>(mult, g_library->m_pixeldensity, kMaxLayerDensity);

					Size fsize = size + Sum(properties->pad);

					Int32 iw = Max<Int32>(Truncate(fsize.w + Core::kRoundingTolerance), 0) & (kMaxTextureSize - 1);

					Int32 ih = Max<Int32>(Truncate(fsize.h + Core::kRoundingTolerance), 0) & (kMaxTextureSize - 1);

					auto valid_size = iw * ih;

					scratch->valid_size = valid_size;

					if (valid_size)
					{
						auto & bitmap = *scratch->bitmap;

						bitmap.SetSize({ iw, ih }, pixel_density);

						Pair <System::fRect> rects =
						{
							{ {}, fsize },
							{ { -origin.x, -origin.y }, fsize }
						};

						scratch->image = bitmap.CreateTextures({ &rects, 1 });
					}
				}

				AlignLayers(scratch->content_state, size, contenth);
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				if (scratch->valid_size)
				{
					{
						RenderTargetScope scope(*Core::RenderContext::st_current, scratch->bitmap, kTransparent);

						scope.transform.origin = scratch->origin;

						RedrawLayers(scratch->content_state, kNormal);

						DrawLayers(scope, kWhite, scratch->content_state);
					}

					return scratch->image;
				}
				else
				{
					return Null<Graphic>();
				}
			};
		});
	}
};

struct ChildrenImpl
{
	struct Properties //: public StandardProperties
	{
	};

	struct Scratch //: public StandardScratch
	{
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				if (data.owner->ChildrenHaveZIndex())
				{
					struct SortedRenderer : public System::Renderer::Graphic
					{
						SortedRenderer(GLX::Object & object)
							: zsort(BuildZIndex(object))
						{
						}

						virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
						{
							auto & ctx = *Core::RenderContext::st_current;

							for (auto & i : zsort.value) i.value->Draw(ctx);
						}

						ZSort zsort;
					};

					return REFLEX_CREATE(SortedRenderer, data.owner);
				}
				else
				{
					struct FastRenderer : public System::Renderer::Graphic
					{
						FastRenderer(GLX::Object & object)
							: object(object)
						{
						}

						virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
						{
							auto & ctx = *Core::RenderContext::st_current;

							for (auto & i : object) i.Draw(ctx);
						}

						GLX::Object & object;
					};

					return REFLEX_CREATE(FastRenderer, data.owner);
				};
			};
		});
	}
};

struct ClipImpl
{
	using Properties = StandardWrapperWithIndentProperties;

	using Scratch = StandardWrapperWithIndentScratch;

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapperWithIndent(schema);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

			vtable.OnBind = &StandardWrapperBind;

			vtable.OnAccommodate = &StandardWrapperAccommodate;

			vtable.OnAlign = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				AlignLayers(scratch->content_state, size, contenth);

				scratch->inner = Indent(size, properties->indent);
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				struct ClipRenderer : public StandardLayersRenderer
				{
					ClipRenderer(Scratch & scratch, const Size & pixelsize)
						: StandardLayersRenderer(scratch.content_state, pixelsize),
						m_clip(scratch.inner)
					{
					}

					virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
					{
						auto & ctx = *Core::RenderContext::st_current;

						auto prev_clip = TransformAndApplyClip<true,true>(ctx, m_clip);

						DrawLayers(ctx, colour, layers);

						ApplyClip(ctx, prev_clip);
					}

					Rect m_clip;
				};

				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				return REFLEX_CREATE(ClipRenderer, scratch, pixelsize);
			};
		});
	};
};

struct MaskImpl
{
	struct Properties : public StandardWrapperProperties
	{
		const ArrayOfLayer mask;
		const bool invert;
	};

	struct Scratch : public StandardWrapperScratch
	{
		Array <LayerWithState> mask_state;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapper(schema);

		RegisterLayersProperty(schema, REFLEX_OFFSETOF(Properties,mask), "mask");

		RegisterBoolProperty(schema, REFLEX_OFFSETOF(Properties, invert), "invert");

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

			vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				AllocateLayers(object, properties->mask, scratch->mask_state);

				AllocateLayers(object, properties->layers, scratch->content_state);
			};

			vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				AccommodateLayers(scratch->mask_state, contentsize);

				AccommodateLayers(scratch->content_state, contentsize);
			};
			
			vtable.OnAlign = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				AlignLayers(scratch->mask_state, size, contenth);

				AlignLayers(scratch->content_state, size, contenth);
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				return REFLEX_CREATE(MaskLayer, *REFLEX_CREATE(StandardLayersRenderer, scratch->mask_state, pixelsize), scratch->content_state, pixelsize, properties->invert);
			};
		});
	};
};

//struct RescaleImpl
//{
//	using Properties = StandardWrapperWithIndentProperties;
//
//	struct Scratch : public StandardWrapperWithIndentScratch
//	{
//		Scale scale, inverse_scale;
//	};
//
//	static void Init(GenericPropertiesSchema & schema)
//	{
//		RegisterStandardWrapperWithIndent("Rescale", schema);
//
//		schema.oninit = [](GenericLayer & layer, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
//		{
//			auto properties = Cast<Properties>(pproperties);
//
//			ForwardOptimisationFlags(Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector, layer, properties->layers);
//
//			vtable.OnBind = &StandardWrapperBind;
//
//			vtable.OnAccommodate = &StandardWrapperAccommodate;
//
//			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, const Size & size, Float & contenth)
//			{
//				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();
//
//				auto inner = Indent(kNormal, properties->indent);
//
//				auto innerpoint_px = inner.origin * Reinterpret<Scale>(size);
//				auto innersize_px = inner.size * Reinterpret<Scale>(size);
//
//				AlignLayers(scratch->content_state, kNormal, contenth);
//
//				scratch->scale = Reinterpret<Scale>(innersize_px);
//
//				scratch->inverse_scale = Reciprocal(scratch->scale);
//
//				scratch->inner.origin = innerpoint_px;
//			};
//
//			if (layer.flags & Layer::kOptimisationFlagVector)
//			{
//				vtable.OnVectoriseColour = [](const GenericLayer & layer, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
//				{
//					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();
//
//					auto start = points.GetSize();
//
//					VectoriseLayers(scratch->content_state, pixelsize * scratch->inverse_scale, points);
//
//					auto region = Reflex::Detail::Splice<false, System::ColourPoint>(points.GetRegion(), start).b;
//
//					Rescale(region, scratch->scale);
//
//					Translate(region, scratch->inner.origin);
//				};
//			}
//
//			vtable.OnRedraw = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags)
//			{
//				struct GraphicImpl : public StandardLayersRendererWithOffset
//				{
//					GraphicImpl(const Scratch & scratch)
//						: StandardLayersRendererWithOffset(scratch, kNormal, scratch.inner.origin),
//						scale(scratch.scale)
//					{
//					}
//
//					virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
//					{
//						auto & matrix = Cast<Core::Matrix>(RemoveConst(transform));
//
//						Core::Matrix::Scope scope(matrix, offset, scale);
//
//						DrawLayers(matrix, colour, layers);
//					}
//
//					const Scale scale;
//				};
//
//				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();
//
//				return REFLE_NEW(Graphic,GraphicImpl>(scratch);
//			};
//		};
//	};
//};

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_special_layers[]
{
	{ "Clip", &GenericLayer::CreateSchema<ClipImpl>, &GenericLayer::Create<ClipImpl> },
	{ "Mask", &GenericLayer::CreateSchema<MaskImpl>, &GenericLayer::Create<MaskImpl> },
	{ "Render", &GenericLayer::CreateSchema<RenderImpl>, &GenericLayer::Create<RenderImpl> },
};
