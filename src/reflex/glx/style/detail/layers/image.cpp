#include "types.h"
#include "layerproperties.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

GLX_DECLARE_ENUM(FitModes, "none", "stretch", "contain", "cover");

constexpr const char * kBitmapPropertyName = "source";

constexpr UInt32 kBitmapPropertyId = K32("source");

REFLEX_INLINE void FloatShift(Float owner_size, Float & item_pos, Float & item_size, Orientation orientation)
{
	switch (orientation)
	{
	case kOrientationCenter:
		item_pos += (owner_size * 0.5f) - (item_size * 0.5f);
		break;

	case kOrientationFar:
		item_pos += (owner_size - item_size);
		break;

	case kOrientationFit:
		item_size = owner_size;
		break;

	default:
		break;
	}
}

struct ImageProperties : public StandardPropertiesWithColour
{
	AxisProperty autofit;

	Pair <Orientation> anchor;
};

struct ImageSetWithFrame
{
	static void RegisterProperty(GenericPropertiesSchema & schema, UIntNative offset);

	REFLEX_INLINE const ImageSet::Frame & GetFrame() const
	{
		return image_set->GetFrames()[frame_idx];
	}

	REFLEX_INLINE Size GetSize() const
	{
		return image_set->GetFrames()[frame_idx].c;
	}

	ConstReference <ImageSet> image_set;

	UInt frame_idx = 0;
};

struct ImageGraphicImpl : public Graphic
{
	ImageGraphicImpl(const Graphic & image, const System::Renderer::Transform & transform)
		: image(image),
		m_transform(transform)
	{
	}

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		image.Render(TransformMatrix(transform, m_transform.origin, m_transform.scale), colour);
	}

	const Graphic & image;

	System::Renderer::Transform m_transform;
};

struct ImageGraphicImplWithColour : public ImageGraphicImpl
{
	ImageGraphicImplWithColour(const Graphic & image, const System::Renderer::Transform & transform, const Colour & colour)
		: ImageGraphicImpl(image, transform),
		m_colour(colour)
	{
	}

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
	{
		ImageGraphicImpl::Render(transform, colour * m_colour);
	}

	const Colour m_colour;
};

struct ImageImpl
{
	struct Properties : public ImageProperties
	{
		ImageSetWithFrame image_set_with_frame;

		FitMode fit = kFitModeNone;
	};

	struct Scratch : public StandardScratch
	{
		template <FitMode FITMODE> static System::Renderer::Transform CalculateTransform(const ImageProperties & properties, const StandardScratch & scratch, Size image_size)
		{
			auto point = scratch.inner.origin;

			auto view_size = scratch.inner.size;

			switch (FITMODE)
			{
			case kFitModeStretch:
				image_size = view_size;
				break;

			case kFitModeCover:
			{
				auto image_aspect = image_size.w / image_size.h;
				auto bounding_aspect = view_size.w / view_size.h;

				auto scale = image_aspect > bounding_aspect ? (view_size.h / image_size.h) : (view_size.w / image_size.w);

				image_size *= scale;
			}
			break;

			case kFitModeContain:
			{
				auto image_aspect = image_size.w / image_size.h;
				auto bounding_aspect = view_size.w / view_size.h;

				auto scale = image_aspect > bounding_aspect ? (view_size.w / image_size.w) : (view_size.h / image_size.h);

				image_size *= scale;
			}
			break;

			default:
				break;
			}

			FloatShift(view_size.w, point.x, image_size.w, properties.anchor.a);
			FloatShift(view_size.h, point.y, image_size.h, properties.anchor.b);

			return { point, image_size };
		}

		decltype (&CalculateTransform<kFitModeNone>) fitfn;
	};

	REFLEX_NOINLINE static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		ImageSetWithFrame::RegisterProperty(schema, REFLEX_OFFSETOF(Properties, image_set_with_frame));
		RegisterAxisProperty(schema, REFLEX_OFFSETOF(Properties, autofit), kAutoFit, kPropertyGroupCustom0);
		RegisterAlignmentProperty(schema, REFLEX_OFFSETOF(Properties, anchor), "anchor", kPropertyGroupCustom1, kBindStageNone);
		RegisterEnumProperty(schema, REFLEX_OFFSETOF(Properties, fit), "fit", &kFitModes, kPropertyGroupCustom1);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			using FitFn = decltype(&Scratch::CalculateTransform<kFitModeNone>);

			auto & properties = *Cast<Properties>(pproperties);

			auto & scratch = *Cast<Scratch>(pscratch);

			bool is_responsive = false;

			switch (properties.fit)
			{
			case kFitModeNone:
				if (self.propertyflags & kPropertyGroupCustom1)
				{
					scratch.fitfn = &Scratch::CalculateTransform<kFitModeNone>;
				}
				else
				{
					scratch.fitfn = [](const ImageProperties & properties, const StandardScratch & scratch, Size image_size) -> System::Renderer::Transform
					{
						return { .scale = image_size };
					};
				}
				break;

			case kFitModeCover:
				scratch.fitfn = &Scratch::CalculateTransform<kFitModeCover>;
				is_responsive = true;
				break;

			case kFitModeContain:
				scratch.fitfn = &Scratch::CalculateTransform<kFitModeContain>;
				is_responsive = true;
				break;

			case kFitModeStretch:
				scratch.fitfn = &Scratch::CalculateTransform<kFitModeStretch>;
				break;
			}

			RemoveConst(self.flags) &= Layer::kOptimisationFlagNotResponsive;

			vtable.OnAlign = BindStandardAlign(self);

			if (properties.autofit)
			{
				vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					contentsize += Sum(properties->indent);

					auto image_size = properties->image_set_with_frame.GetSize();

					contentsize += image_size * kAxisToSize[properties->autofit];
				};

				if ((properties.autofit | kAxisPropertyY) && is_responsive)	//RESPONSIVE
				{
					RemoveConst(self.flags) &= ~Layer::kOptimisationFlagNotResponsive;	//CLEAN UP as param?

					vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
					{
						auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

						StandardIndent(layer, data, size, contenth);

						auto & viewsize = scratch->inner.size;

						auto image_size = properties->image_set_with_frame.GetSize();

						auto scale = viewsize.w / image_size.w;

						contenth = AxisSum<YAxis>(properties->indent) + Reflex::RoundUp(image_size.h * scale);
					};
				}
			}

			if (properties.fit == kFitModeCover)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					struct GraphicImplWithClip : public ImageGraphicImplWithColour
					{
						GraphicImplWithClip(const Properties & properties, const Scratch & scratch, const ImageSet::Frame & frame)
							: ImageGraphicImplWithColour(frame.b, scratch.fitfn(properties, scratch, frame.c), properties.colour),
							m_clip(scratch.inner)
						{
						}

						virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
						{
							auto & ctx = *Core::RenderContext::st_current;

							auto prev_clip = TransformAndApplyClip<true,true>(ctx, m_clip);

							ImageGraphicImplWithColour::Render(transform, colour);

							ApplyClip(ctx, prev_clip);
						}

						Rect m_clip;
					};

					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					return REFLEX_CREATE(GraphicImplWithClip, properties, scratch, properties->image_set_with_frame.GetFrame());
				};
			}
			else if (self.propertyflags & kPropertyGroupColour)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					auto & frame = properties->image_set_with_frame.GetFrame();

					return REFLEX_CREATE(ImageGraphicImplWithColour, frame.b, scratch->fitfn(properties, scratch, frame.c), properties->colour);
				};
			}
			else
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					auto & frame = properties->image_set_with_frame.GetFrame();

					return REFLEX_CREATE(ImageGraphicImpl, frame.b, scratch->fitfn(properties, scratch, frame.c));
				};
			}
		});
	}
};

struct ImageSelectorImpl
{
	struct Properties : public ImageProperties
	{
		static const ImageSet::Frame * GetFrameNormalized(const Properties & properties)
		{
			if (auto frames = properties.image_set->GetFrames())
			{
				Int max = frames.size - 1;

				auto idx = Clip(Truncate(Reinterpret<Float32>(properties.frame_data) * Float32(max)), 0, max);

				return frames.data + idx;
			}
			else
			{
				return nullptr;
			}
		}

		static const ImageSet::Frame * GetFrameById(const Properties & properties)
		{
			return SearchValue<KeyCompare>(properties.image_set->GetFrames(), Reinterpret<Key32>(properties.frame_data));
		}

		static const ImageSet::Frame * GetFrameByIdx(const Properties & properties)
		{
			auto frames = properties.image_set->GetFrames();

			auto idx = Reinterpret<UInt32>(properties.frame_data);

			if (idx < frames.size)
			{
				return frames.data + idx;
			}
			else
			{
				return nullptr;
			}
		}


		ConstReference <ImageSet> image_set;

		UInt frame_data = 0;

		decltype (&GetFrameNormalized) get_frame = [](const Properties & properties) -> const Detail::ImageSet::Frame *
		{
			auto frames = properties.image_set->GetFrames();

			return frames ? frames.data : nullptr;
		};
	};

	struct Scratch : public StandardScratch
	{
		ConstReference <Graphic> image;
		
		Size image_size = kNormal;
	};


	static constexpr UInt16 kFrameOffset = UInt16(REFLEX_OFFSETOF(Properties, frame_data));

	static void OnSetFloat32Property(const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto properties = ToPointer<Properties>(ToUIntNative(adr) - kFrameOffset);

		*Cast<Float32>(adr) = Cast<Data::Float32Property>(object)->value;

		properties->get_frame = &Properties::GetFrameNormalized;
	}

	REFLEX_NOINLINE static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		constexpr UInt16 kImageSetOffset = REFLEX_OFFSETOF(Properties, image_set);

		auto pdefs = schema.RegisterProperties(2);

		pdefs[0] = { kImageSetOffset, kPropertyGroupNone, kBindStageNone, kBitmapPropertyName, MakeAddress<Data::Key32Property>(kBitmapPropertyId), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto image_set = FindResource<ImageSet>(st_current_style, Cast<Data::Key32Property>(object)->value);

			if (image_set->GetFrames())
			{
				*Cast< ConstReference <ImageSet> >(adr) = image_set;
			}
		} };

		pdefs[1] = { kImageSetOffset, kPropertyGroupNone, kBindStageAccommodate, kBitmapPropertyName, MakeAddress<ImageSet>(kBitmapPropertyId), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto image_set = Cast<ImageSet>(object);

			if (image_set->GetFrames())
			{
				*Cast<ConstReference<ImageSet>>(adr) = image_set;
			}
		} };


		schema.RegisterProperty({ kFrameOffset, kPropertyGroupNone, kBindStageAccommodate, "frame", MakeAddress<Data::Key32Property>(K32("frame")), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto properties = ToPointer<Properties>(ToUIntNative(adr) - kFrameOffset);

			*Cast<Key32>(adr) = Cast<Data::Key32Property>(object)->value;

			properties->get_frame = &Properties::GetFrameById;
		} });

		schema.RegisterProperty({ kFrameOffset, kPropertyGroupNone, kBindStageAccommodate, "frame", MakeAddress<Data::UInt32Property>(K32("frame")), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto properties = ToPointer<Properties>(ToUIntNative(adr) - kFrameOffset);

			*Cast<UInt32>(adr) = Cast<Data::UInt32Property>(object)->value;

			properties->get_frame = &Properties::GetFrameByIdx;
		} });

		schema.RegisterProperty({ kFrameOffset, kPropertyGroupNone, kBindStageAccommodate, "frame", MakeAddress<Data::Float32Property>(K32("frame")), 0, &OnSetFloat32Property });

		schema.RegisterProperty({ kFrameOffset, kPropertyGroupNone, kBindStageAccommodate, "frame", MakeAddress<Data::ArrayOfFloat32Property>(K32("frame")), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto & values = Cast<Data::ArrayOfFloat32Property>(object)->value;

			if (values)
			{
				Reflex::Detail::Initialiser <Data::Float32Property> raw;

				raw->value = values.GetFirst();

				OnSetFloat32Property(raw, adr, data, stylesheet_flags);
			}
		} });

		RegisterAxisProperty(schema, REFLEX_OFFSETOF(Properties, autofit), kAutoFit, kPropertyGroupCustom0);

		RegisterAlignmentProperty(schema, REFLEX_OFFSETOF(Properties, anchor), "anchor", kPropertyGroupCustom1, kBindStageNone);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				contentsize += Sum(properties->indent);

				if (auto pframe = properties->get_frame(properties))
				{
					scratch->image = pframe->b;

					auto image_size = pframe->c;

					scratch->image_size = image_size;

					contentsize += image_size * kAxisToSize[properties->autofit];
				}
				else
				{
					scratch->image.Clear();

					scratch->image_size = {};
				}
			};

			if (self.propertyflags & kPropertyGroupCustom1)
			{
				vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & self, Size size, Float & contenth)
				{
					auto [properties, scratch] = self.GetPropertiesAndScratch<Properties, Scratch>();

					StandardIndent(layer, self, size, contenth);

					scratch->inner.origin = ImageImpl::Scratch::CalculateTransform<kFitModeNone>(properties, scratch, scratch->image_size).origin;
				};
			}
			else
			{
				vtable.OnAlign = BindStandardAlign(self);
			}

			if (self.propertyflags & kPropertyGroupColour)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					return REFLEX_CREATE(ImageGraphicImplWithColour, scratch->image, { scratch->inner.origin, scratch->image_size }, properties->colour);
				};
			}
			else
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					return REFLEX_CREATE(ImageGraphicImpl, scratch->image, { scratch->inner.origin, scratch->image_size });
				};
			}
		});
	}
};

struct ImageFillImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		ImageSetWithFrame image_set_with_frame;
	};

	struct Scratch : public StandardScratch
	{
		Reference <Graphic> texture;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		ImageSetWithFrame::RegisterProperty(schema, REFLEX_OFFSETOF(Properties, image_set_with_frame));

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				StandardIndent(layer, data, size, contenth);

				auto & [image_set, frame_idx] = properties->image_set_with_frame;

				auto & frame = image_set->GetFrames()[frame_idx];

				auto image_size = frame.c;

				auto & inner = scratch->inner;

				UInt ncol = Truncate(Max(inner.size.w, 0.0f) / image_size.w) + 1;

				UInt nrow = Truncate(Max(inner.size.h, 0.0f) / image_size.h) + 1;

				UInt n = nrow * ncol;

				Array < Pair <System::fRect> > buffer(n);

				auto prects = buffer.GetData();

				System::fRect src = { kNormalRect.origin, { image_size.w, image_size.h } };

				REFLEX_LOOP(y, nrow)
				{
					Float fy = inner.origin.y + (y * image_size.h);

					REFLEX_LOOP(x, ncol)
					{
						auto & rects = *prects++;

						rects.a = src;

						rects.b = { { inner.origin.x + (x * image_size.w), fy }, src.size };
					}
				}

				scratch->texture = image_set->source_bitmap->CreateTextures(buffer);
			};

			if (self.propertyflags & kPropertyGroupColour)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					return REFLEX_CREATE(GraphicRendererWithOffsetAndColour, scratch->texture, kOrigin, properties->colour);
				};
			}
			else
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags)->TRef <Graphic>
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					return scratch->texture;
				};
			}
		});
	}
};

REFLEX_NOINLINE void ImageSetWithFrame::RegisterProperty(GenericPropertiesSchema & schema, UIntNative offset_)
{
	UInt16 offset = UInt16(offset_);

	auto pdefs = schema.RegisterProperties(3);

	pdefs[0] = { offset, kPropertyGroupNone, kBindStageNone, kBitmapPropertyName, MakeAddress<Data::Key32Property>(kBitmapPropertyId), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto image_set = FindResource<ImageSet>(st_current_style, Cast<Data::Key32Property>(object)->value);

		if (image_set->GetFrames())
		{
			*Cast<ImageSetWithFrame>(adr) = { image_set, 0 };
		}
	} };

	pdefs[1] = { offset, kPropertyGroupNone, kBindStageNone, kBitmapPropertyName, MakeAddress<Data::ArrayOfKey32Property>(kBitmapPropertyId), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto & attributes = Cast<Data::ArrayOfKey32Property>(object)->value;

		if (attributes.GetSize() == 2)
		{
			auto image_set = FindResource<ImageSet>(st_current_style, attributes[0]);

			if (auto idx = Search<KeyCompare>(image_set->GetFrames(), attributes[1]))
			{
				*Cast<ImageSetWithFrame>(adr) = { image_set, idx.value };
			}
		}
	} };

	pdefs[2] = { offset, kPropertyGroupNone, kBindStageBuildLayout, kBitmapPropertyName, MakeAddress<ImageSet>(kBitmapPropertyId), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	{
		auto image_set = Cast<ImageSet>(object);

		if (image_set->GetFrames())
		{
			*Cast<ImageSetWithFrame>(adr) = { image_set, 0 };
		}
	} };
}

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_image_layers[3]
{
	{ "Image", &GenericLayer::CreateSchema<ImageImpl>, &GenericLayer::Create<ImageImpl> },
	{ "ImageSelector", &GenericLayer::CreateSchema<ImageSelectorImpl>, &GenericLayer::Create<ImageSelectorImpl> },
	{ "ImageFill", &GenericLayer::CreateSchema<ImageFillImpl>, &GenericLayer::Create<ImageFillImpl> },
};
