#include "wrappers.h"



//
//boxlayer

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

enum Axis : UInt8
{
	kAxisX,
	kAxisY
};

GLX_DECLARE_ENUM(Axis, "x", "y");

ArrayRegion <System::ColourPoint> VectoriseLayers(const Array <LayerWithState> & layers, Size pixelsize, ColourPoints & output)
{
	auto top = output.GetSize();

	for (auto & i : layers)
	{
		auto layer = Cast<GenericLayer>(i.a);

		auto data = Cast<GenericLayer::ObjectState>(i.b);

		layer->Vectorise(data, pixelsize, output);
	}

	return Reflex::Detail::Mid<true>(ToRegion(output), top, kMaxUInt32);
}

struct ColourImpl
{
	struct Properties : public StandardWrapperProperties
	{
		Colour colour;
	};

	using Scratch = StandardWrapperScratch;

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapper(schema);

		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, colour), "colour");
		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, colour), "color");

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector, properties->layers);

			vtable.OnBind = &StandardWrapperBind;
			vtable.OnAccommodate = &StandardWrapperAccommodate;
			vtable.OnAlign = &StandardWrapperAlign;

			vtable.OnVectoriseColour = [](const GenericLayer & layer, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				UInt start = points.GetSize();

				VectoriseLayers(scratch->content_state, pixelsize, points);

				ArrayRegion <ColourPoint> region = { points.GetData() + start, points.GetSize() - start };

				ModulateColour(region, properties->colour);
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				return REFLEX_CREATE(StandardLayersRendererWithOffsetAndColour, scratch->content_state, pixelsize, kOrigin, properties->colour);
			};
		});
	}
};

struct AlignImpl
{
	struct Properties : public StandardWrapperWithIndentProperties
	{
		Size size;
		Pair <Orientation> position = { kOrientationFit, kOrientationFit };
		//bool normalized = false;
	};

	struct Scratch : public StandardWrapperWithIndentScratch
	{
		Size content_size;
		AxisProperty fit;
	};

	template <bool INDENT> static void OnAccommodate(const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
	{
		auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

		//if (properties->normalized)
		//{
		//	contentsize = {};
		//}
		//else
		{
			contentsize = properties->size;
		}

		AccommodateLayers(scratch->content_state, contentsize);

		scratch->fit = kAxisFit[OrientationToAlignmentEx(properties->position)];

		//if (properties->normalized)
		//{
		//	scratch->content_size = properties->size;

		//	contentsize *= properties->size;
		//}
		//else
		{
			scratch->content_size = contentsize;
		}

		if constexpr (INDENT) contentsize += Sum(properties->indent);
	}
	
	template <bool INDENT> static void OnAlign(const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & content_h)
	{
		auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

		auto & inner = scratch->inner;

		if constexpr (INDENT)
		{
			inner = Indent(size, properties->indent);
		}
		else
		{
			inner = { kOrigin, size };
		}

		auto inner_size = inner.size;

		auto position = properties->position;

		auto content_size = scratch->content_size;

		//if (!(layer.flags & GenericLayer::kOptimisationFlagNotResponsive))
		//{
		//	Float h = content_h;

		//	for (auto & i : scratch->content_state)
		//	{
		//		i.a->Align(i.b, size, h);
		//	}

		//	if (NORMALIZED)
		//	{
		//		h = h / Max(size.h, 1.0f);
		//	}

		//	content_size.h = h;
		//	//content_h = Max(content_h, h);
		//}

		switch (scratch->fit)
		{
		case kAxisPropertyNone:
			//if constexpr (NORMALIZED)
			//{
			//	inner.size *= content_size;
			//}
			//else
			{
				inner.size = content_size;
			}
			inner.origin.x += Align1D(inner_size.w, inner.size.w, position.a);
			inner.origin.y += Align1D(inner_size.h, inner.size.h, position.b);
			break;

		case kAxisPropertyX:
			//if constexpr (NORMALIZED)
			//{
			//	inner.size.h = content_size.h * inner_size.h;
			//}
			//else
			{
				inner.size.h = content_size.h;
			}
			inner.origin.y += Align1D(inner_size.h, inner.size.h, position.b);
			break;

		case kAxisPropertyY:
			//if constexpr (NORMALIZED)
			//{
			//	inner.size.w = content_size.w * inner_size.w;
			//}
			//else
			{
				inner.size.w = content_size.w;
			}
			inner.origin.x += Align1D(inner_size.w, inner.size.w, position.a);
			break;

		default:
			break;
		}

		inner = SnapToPixels(inner);

		AlignLayers(scratch->content_state, inner.size, content_h);
	};

	REFLEX_TBINDER_1P(OnAlign);

	static constexpr AxisProperty kAxisFit[16] =
	{
		kAxisPropertyNone, kAxisPropertyNone, kAxisPropertyNone, kAxisPropertyX,
		kAxisPropertyNone, kAxisPropertyNone, kAxisPropertyNone, kAxisPropertyX,
		kAxisPropertyNone, kAxisPropertyNone, kAxisPropertyNone, kAxisPropertyX,
		kAxisPropertyY, kAxisPropertyY, kAxisPropertyY, kAxisPropertyXY,
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapper(schema);

		RegisterIndent(schema, REFLEX_OFFSETOF(Properties, indent));
		RegisterSizeProperty(schema, REFLEX_OFFSETOF(Properties, size), "size");
		RegisterAlignmentProperty(schema, REFLEX_OFFSETOF(Properties, position), "position", kPropertyGroupNone, kBindStageAccommodate);
		//RegisterBoolProperty(schema, REFLEX_OFFSETOF(Properties, normalized), "normalized", kPropertyGroupCustom0, kBindStageNone);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector, properties->layers);

			vtable.OnBind = &StandardWrapperBind;

			UInt8 indent = self.propertyflags & kPropertyGroupIndent;

			vtable.OnAccommodate = indent ? &OnAccommodate<true> : &OnAccommodate<false>;
			vtable.OnAlign = indent ? &OnAlign<true> : &OnAlign<false>; // OnAlignBinder::Bind(indent | UInt8(MakeBit(1, properties->normalized)));

			vtable.OnVectoriseColour = [](const GenericLayer & layer, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				auto region = VectoriseLayers(scratch->content_state, pixelsize, points);

				Translate(region, scratch->inner.origin);
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				return REFLEX_CREATE(StandardLayersRendererWithOffset, scratch->content_state, pixelsize, scratch->inner.origin);
			};
		});
	}
};

//struct AspectImpl
//{
//	struct Properties : public StandardWrapperProperties
//	{
//		Float ratio = 1.0f;
//	};
//
//	struct Scratch : public StandardWrapperScratch
//	{
//	};
//
//	static void Init(GenericPropertiesSchema & schema)
//	{
//		RegisterStandardWrapper(schema);
//
//		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, ratio), "ratio");
//
//		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
//		{
//			RemoveConst(self.flags) = 0;
//
//			//RemoveConst(self.flags) &= ~Layer::kOptimisationFlagNotResponsive;	//CLEAN UP as param?
//
//			vtable.OnBind = &StandardWrapperBind;
//			vtable.OnAccommodate = &StandardWrapperAccommodate;
//
//			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
//			{
//				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();
//
//				AlignLayers(scratch->content_state, size, contenth);
//
//				contenth = size.w * properties->ratio;
//			};
//
//			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
//			{
//				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();
//
//				return REFLEX_CREATE(StandardLayersRenderer, scratch->content_state, pixelsize);
//			};
//
//			//vtable.OnVectoriseColour = &Standardl
//		});
//	}
//};

struct TranslateImpl
{
	struct Properties : public StandardWrapperProperties
	{
		Key32 unit;
		Point offset;
	};

	struct Scratch : public StandardWrapperScratch
	{
		ConstReference <AbstractViewPort::ViewState> viewport;
		Point offset;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapper(schema);

		RegisterKey32Property(schema, REFLEX_OFFSETOF(Properties, unit), "unit", kPropertyGroupCustom0, kBindStageNone);
		RegisterPointProperty(schema, REFLEX_OFFSETOF(Properties, offset), "offset", kPropertyGroupNone, kBindStageRedraw);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector, properties->layers);

			vtable.OnBind = &StandardWrapperBind;

			if (properties->unit == K32("px"))
			{
				vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & content_size)
				{
					StandardWrapperAccommodate(layer, data, content_size);

					auto scratch = data.GetScratch<Scratch>(layer);

					scratch->viewport = GetContainingViewPort(data.owner)->view_state;
				};

				vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
				{
					StandardWrapperAlign(layer, data, size, contenth);

					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					scratch->offset = properties->offset * scratch->viewport->GetPixelsPerUnit();
				};
			}
			else
			{
				vtable.OnAccommodate = &StandardWrapperAccommodate;

				vtable.OnAlign = &StandardWrapperAlign;
			}

			if (self.flags & Layer::kOptimisationFlagVector)
			{
				vtable.OnVectoriseColour = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					auto region = VectoriseLayers(scratch->content_state, pixelsize, points);

					Translate(region, properties->offset);
				};
			}
			else
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					auto & offset = (scratch->viewport) ? scratch->offset : properties->offset;

					return REFLEX_CREATE(StandardLayersRendererWithOffset, scratch->content_state, pixelsize, offset);
				};
			}
		});
	};
};

struct RotateImpl
{
	struct Properties : public StandardWrapperWithIndentProperties
	{
		Size origin;
		Float32 angle = 0.0f;
	};

	using Scratch = StandardWrapperWithIndentScratch;

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapperWithIndent(schema);

		RegisterAngleProperty(schema, REFLEX_OFFSETOF(Properties, angle));

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector, properties->layers);

			vtable.OnBind = &StandardWrapperBind;

			vtable.OnAccommodate = &StandardWrapperAccommodate;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto properties = data.GetProperties<StandardWrapperWithIndentProperties>();

				auto scratch = data.GetScratch<StandardWrapperWithIndentScratch>(layer);

				auto & inner = scratch->inner;

				inner = Indent(size, properties->indent);

				AlignLayers(scratch->content_state, inner.size, contenth);
			};

			if (self.flags & Layer::kOptimisationFlagVector)
			{
				vtable.OnVectoriseColour = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					auto region = VectoriseLayers(scratch->content_state, pixelsize, points);

					auto & inner = scratch->inner;

					if (self.propertyflags & kPropertyGroupIndent)
					{
						Translate(region, inner.origin);
					}

					auto center = inner.origin + (inner.size * 0.5f);

					Rotate(region, Reinterpret<Point>(center), properties->angle * k2Pif);
				};
			}
			else
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags)
				{
					return Null<Graphic>();
				};
			}
		});
	};
};

struct ScaleImpl
{
	struct Properties : public StandardWrapperProperties
	{
		Scale scale = kNormal;
		Size origin = { 0.5f, 0.5f };
	};

	struct Scratch : public StandardWrapperScratch
	{
		Scale pixelsize = kNormal;
		Point offset;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapper(schema);

		RegisterSizeProperty(schema, REFLEX_OFFSETOF(Properties,scale), "scale", kPropertyGroupNone, kBindStageRedraw);
		RegisterSizeProperty(schema, REFLEX_OFFSETOF(Properties, origin), "origin", kPropertyGroupNone, kBindStageRedraw);
		
		schema.Pop();

		GenericPropertiesSchema::Item item = { UInt16(REFLEX_OFFSETOF(Properties, origin)), kPropertyGroupNone, kBindStageNone, "origin", MakeAddress<Data::Key32Property>(K32("origin")), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto orientation = kAlignmentToOrientation[ParseAlignment(Cast<Data::Key32Property>(object)->value, kAlignmentCenter)];

			auto & point = *Cast<Point>(adr);

			point.x = kOrientationToAlign1D[orientation.a].a;
			point.y = kOrientationToAlign1D[orientation.b].a;
		} };

		schema.RegisterProperty(item);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector, properties->layers);

			vtable.OnBind = &StandardWrapperBind;

			vtable.OnAccommodate = &StandardWrapperAccommodate;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				StandardWrapperAlign(layer, data, size, contenth);

				auto origin = properties->origin;

				auto original_origin = size * origin;

				auto scale = properties->scale;

				auto newpoint = size * scale * origin;

				scratch->pixelsize = { scale.w ? 1.0f / scale.w : 1.0f, scale.h ? 1.0f / scale.h : 1.0f };

				scratch->offset = Reinterpret<Point>(original_origin - newpoint);
			};

			if (self.flags & Layer::kOptimisationFlagVector)
			{
				vtable.OnVectoriseColour = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					auto region = VectoriseLayers(scratch->content_state, pixelsize * scratch->pixelsize, points);

					Rescale(region, properties->scale);

					Translate(region, scratch->offset);
				};
			}
			else
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					struct GraphicImpl : public StandardLayersRendererWithOffset
					{
						GraphicImpl(const Properties & properties, Scratch & scratch, const Size & pixelsize)
							: StandardLayersRendererWithOffset(scratch.content_state, pixelsize, scratch.offset),
							scale(properties.scale)
						{
						}
						
						virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
						{
							auto & ctx = *Core::RenderContext::st_current;

							TransformScope a = { ctx, offset, scale };

							DrawLayers(ctx, colour, layers);
						}
						
						const Scale scale;
					};
					
					return REFLEX_CREATE(GraphicImpl, properties, scratch, pixelsize);
				};
			}
		});
	};
};

struct IfImpl
{
	enum CompareOp : UInt8
	{
		kCompareOpNone,

		kCompareOpGT,
		kCompareOpGTE,
		kCompareOpLT,
		kCompareOpLTE,

		kNumCompareOp
	};

	GLX_DECLARE_ENUM(CompareOp, "", "gt", "gte", "lt", "lte");

	struct Properties : public StandardWrapperProperties
	{
		const ArrayOfLayer _true;
		CompareOp op2;
		Float input = 0.0f;
		Float value = 0.0f;
	};

	struct Scratch //: public StandardScratch
	{
		Array <LayerWithState> false_true_state[2];
		Array <LayerWithState> * presult;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndent(schema);

		RegisterLayersProperty(schema, REFLEX_OFFSETOF(Properties, layers), "else");
		RegisterLayersProperty(schema, REFLEX_OFFSETOF(Properties,_true), "then");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, input), "input");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, value), "value");
		RegisterEnumProperty(schema, REFLEX_OFFSETOF(Properties, op2), "op", &kCompareOp);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);
			PropogateOptimisationFlags(self, self.flags, properties->_true);

			vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				AllocateLayers(object, properties->layers, scratch->false_true_state[0]);
				
				AllocateLayers(object, properties->_true, scratch->false_true_state[1]);
			};

			vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & content_size)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				for (auto & i : scratch->false_true_state) AccommodateLayers(i, content_size);
			};

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				bool result = false;

				switch (properties->op2)
				{
				case kCompareOpGT:
					result = properties->input > properties->value;
					break;

				case kCompareOpGTE:
					result = properties->input >= properties->value;
					break;

				case kCompareOpLT:
					result = properties->input < properties->value;
					break;

				case kCompareOpLTE:
					result = properties->input <= properties->value;
					break;

				default:
					break;
				}
				
				for (auto & i : scratch->false_true_state) AlignLayers(i, size, contenth);

				scratch->presult = scratch->false_true_state + result;
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				return REFLEX_CREATE(StandardLayersRenderer, *scratch->presult, pixelsize);
			};
		});
	};
};

struct InlineImpl
{
	struct Properties : public StandardWrapperWithIndentProperties
	{
		Axis axis = kAxisX;
	};

	struct Scratch
	{
		Size indent_sum;
		Array <LayerWithState> layer_states;
		Array <Pair<Size,Rect>> layer_sizes;
	};

	struct Renderer : public System::Renderer::Graphic
	{
		Renderer(Scratch & scratch)
			: layers(scratch.layer_states)
			, layer_sizes(scratch.layer_sizes)
		{
		}

		void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
		{
			auto & ctx = *Core::RenderContext::st_current;

			auto psizes = layer_sizes.GetData();

			for (auto & i : layers)
			{
				TransformScope scope = { ctx, (*psizes++).b.origin };

				i.c->Render(ctx.transform, colour);
			}
		}

		Array <LayerWithState> & layers;
		const Array <Pair<Size, Rect>> & layer_sizes;
	};

	static TRef <Graphic> OnRedraw(const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags)
	{
		auto scratch = data.GetScratch<Scratch>(self);

		RedrawLayers(scratch->layer_states, pixelsize);

		return REFLEX_CREATE(Renderer, scratch);
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterLayersProperty(schema, REFLEX_OFFSETOF(Properties, layers), "content");
		RegisterIndent(schema, REFLEX_OFFSETOF(Properties, indent));
		RegisterEnumProperty(schema, REFLEX_OFFSETOF(Properties, axis), "axis", &kAxis);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

			vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				AllocateLayers(object, properties->layers, scratch->layer_states);

				scratch->layer_sizes.SetSize(scratch->layer_states.GetSize());
			};

			vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & content_size)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				bool axis = True(properties->axis);
				bool ortho = !axis;

				REFLEX_ASSERT(!(content_size.w || content_size.h));

				Size size;

				auto psizes = scratch->layer_sizes.GetData();

				for (auto & i : scratch->layer_states)
				{
					auto & layer = *i.a;
					auto & state = i.b;

					Size max;

					layer.Accommodate(state, max);

					(&size.w)[axis] += (&max.w)[axis];

					auto & ortho_v = (&size.w)[ortho];

					ortho_v = Max(ortho_v, (&max.w)[ortho]);

					(*psizes++).a = max;
				}

				auto indent_sum = Sum(properties->indent);

				content_size = indent_sum + size;

				scratch->indent_sum = indent_sum;
			};

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & content_h)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				bool axis = True(properties->axis);
				bool ortho = !axis;

				auto & [indent_near,indent_far] = properties->indent;

				auto ortho_size = GetSize(ortho, size) - GetSize(ortho, scratch->indent_sum);

				Float axis_pos = GetSize(axis, indent_near);
				Float ortho_indent = GetSize(ortho, indent_near);

				auto psizes = scratch->layer_sizes.GetData();

				content_h = scratch->indent_sum.h;

				for (auto & i : scratch->layer_states)
				{
					auto & layer = *i.a;
					auto & state = i.b;

					auto & [item_content_size,item_rect] = (*psizes++);

					item_rect.size = item_content_size;

					SetPoint(axis, item_rect.origin, axis_pos);
					SetPoint(ortho, item_rect.origin, ortho_indent);

					SetSize(ortho, item_rect.size, ortho_size);

					layer.Align(state, item_rect.size, item_content_size.h);

					axis_pos += GetSize(axis, item_rect.size);

					content_h += item_content_size.h;
				}
			};

			vtable.OnRedraw = &OnRedraw;
		});
	};
};

struct SplitImpl
{
	struct Properties : public StandardWrapperProperties
	{
		Axis axis = kAxisX;
		ConstReference <Data::ArrayOfFloat32Property> ratios;
	};

	struct Scratch : public InlineImpl::Scratch
	{
		Array <Float> ratios;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterLayersProperty(schema, REFLEX_OFFSETOF(Properties, layers), "content");
		RegisterEnumProperty(schema, REFLEX_OFFSETOF(Properties, axis), "axis", &kAxis);
		RegisterFloatArrayProperty(schema, REFLEX_OFFSETOF(Properties, ratios), "ratio", kPropertyGroupNone, kBindStageNone);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

			vtable.OnBind = [](const GenericLayer & layer, GenericLayer::ObjectState & data, GLX::Object & object)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				auto num_layer = properties->layers.GetSize();

				AllocateLayers(object, properties->layers, scratch->layer_states);
			
				scratch->layer_sizes.SetSize(num_layer);

				auto fnum_layer = Float(num_layer);

				auto & ratios = properties->ratios->value;

				scratch->ratios.SetSize(num_layer);

				if (ratios)
				{
					Float32 remainder = 1.0f;

					REFLEX_LOOP(idx, scratch->ratios.GetSize())
					{
						auto ratio = ratios[idx % ratios.GetSize()];

						ratio = Min(ratio, remainder);

						scratch->ratios[idx] = ratio;

						remainder -= ratio;
					}

					scratch->ratios.GetLast() += remainder;
				}
				else if (num_layer)
				{
					auto spread = 1.0f / fnum_layer;

					scratch->ratios.Fill(spread);
				}
			};

			vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & content_size)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				bool axis = True(properties->axis);
				bool ortho = !axis;

				REFLEX_ASSERT(!(content_size.w || content_size.h));

				Size size;

				for (auto & i : scratch->layer_states)
				{
					auto & child = *i.a;
					auto & state = i.b;

					Size max;

					child.Accommodate(state, max);

					(&size.w)[axis] = Max((&size.w)[axis], (&max.w)[axis]);
					(&size.w)[ortho] = Max((&size.w)[ortho], (&max.w)[ortho]);
				}

				content_size = size;
			};

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				bool axis = True(properties->axis);
				bool ortho = !axis;

				UInt count = scratch->layer_states.GetSize();

				Float total_axis = GetSize(axis, size);
				Float ortho_size = GetSize(ortho, size);

				Float pos = 0.0f;
				Float remaining = total_axis;

				auto prects = scratch->layer_sizes.GetData();

				REFLEX_LOOP(idx, count)
				{
					Float ratio = scratch->ratios[idx];
					Float item_axis;

					item_axis = Quantise(total_axis * ratio, kPixelSize);

					Rect rect;
					rect.origin = MakePoint(axis, pos);
					rect.size = MakeSize(axis, item_axis, ortho_size);

					(*prects++).b = rect;

					pos += item_axis;
					remaining -= item_axis;
				}

				if (remaining && count)
				{
					auto & rect = scratch->layer_sizes.GetLast().b;

					SetSize(axis, rect.size, GetSize(axis, rect.size) + remaining);
				}

				prects = scratch->layer_sizes.GetData();

				for (auto & i : scratch->layer_states)
				{
					auto & child = *i.a;
					auto & state = i.b;
					auto & rect = (*prects++).b;

					Float max_h = 0.0f;

					child.Align(state, rect.size, max_h);
				}

				contenth = GetSize(axis, size);
			};

			vtable.OnRedraw = &InlineImpl::OnRedraw;
		});
	}
};

struct BarImpl
{
	struct Properties : public StandardWrapperWithIndentProperties
	{
		Float32 min_size = 0.0f;
		Range range = kNormalRange;
		Range region;
	};

	struct Scratch : public StandardWrapperWithIndentScratch
	{
		Rect track;
	};

	static Pair <Float> GetAxisOrtho(bool y, const Point & point)
	{
		auto ptr = &point.x;

		return { ptr[y], ptr[!y] };
	}

	static Pair <Float> GetAxisOrtho(bool y, const Size & size)
	{
		auto ptr = &size.w;

		return { ptr[y], ptr[!y] };
	}

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapperWithIndent(schema);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties,min_size), "min_size");
		RegisterRangeProperty(schema, REFLEX_OFFSETOF(Properties,range), "range", kPropertyGroupNone, kBindStageRealign);
		RegisterRangeProperty(schema, REFLEX_OFFSETOF(Properties,region), "region", kPropertyGroupNone, kBindStageRealign);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto properties = Cast<Properties>(pproperties);

			PropogateOptimisationFlags(self, Layer::kOptimisationFlagNotResponsive, properties->layers);

			vtable.OnBind = &StandardWrapperBind;

			vtable.OnAccommodate = &StandardWrapperAccommodate;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				auto & inner = scratch->inner;

				inner = Indent(size, properties->indent);

				auto range = properties->range;

				auto region = properties->region;

				auto flowflags = scratch->flowflags;

				bool axis = (flowflags & kFlowY);

				bool ortho = !axis;

				
				auto [axis_inner_origin, ortho_inner_origin] = GetAxisOrtho(axis, inner.origin);
				
				auto [axis_inner_size, ortho_inner_size] = GetAxisOrtho(axis, inner.size);

	
				auto track_start_normalized = NormaliseValue(region.start, range);

				auto track_end_normalized = NormaliseValue(region.start + region.length, range);

				auto track_size_normalized = track_end_normalized - track_start_normalized;

				if (flowflags & kFlowInvert)
				{
					track_start_normalized = (1.0f - track_start_normalized) - track_size_normalized;
				}


				SetPoint(axis, scratch->track.origin, axis_inner_origin + (track_start_normalized * axis_inner_size));

				SetPoint(ortho, scratch->track.origin, ortho_inner_origin);

				
				SetSize(axis, scratch->track.size, track_size_normalized * axis_inner_size);
				
				SetSize(ortho, scratch->track.size, ortho_inner_size);


				scratch->track = SnapToPixels(scratch->track);


				auto expand = properties->min_size - GetSize(axis, scratch->track.size);

				if (expand > 0.0f)
				{
					scratch->track = Expand(scratch->track, MakeSize(axis, expand * 0.5f));
				}

				AlignLayers(scratch->content_state, scratch->track.size, contenth);
			};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				return REFLEX_CREATE(StandardLayersRendererWithOffset, scratch->content_state, pixelsize, scratch->track.origin);
			};
		});
	};
};

struct TileImpl
{
	struct Properties : public StandardWrapperWithIndentProperties
	{
		AxisProperty axis = kAxisPropertyXY;
		Size stride;
	};

	struct Scratch : public StandardWrapperWithIndentScratch
	{
		Array <Point> offsets;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterStandardWrapperWithIndent(schema);

		RegisterAxisProperty(schema, REFLEX_OFFSETOF(Properties,axis), "axis", kPropertyGroupNone, kBindStageNone);
		RegisterSizeProperty(schema, REFLEX_OFFSETOF(Properties,stride), "stride");

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			auto & properties = *Cast<Properties>(pproperties);

			vtable.OnBind = &StandardWrapperBind;

			vtable.OnAccommodate = &StandardWrapperAccommodate;

			if (properties.axis == kAxisPropertyXY)
			{
				vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					auto & inner = scratch->inner;

					auto & offsets = scratch->offsets;

					auto stride = properties->stride;

					inner = Indent(size, properties->indent);

					AlignLayers(scratch->content_state, stride, contenth);

					auto ncol = Truncate(inner.size.w / Reflex::Max(stride.w, 8.0f));

					auto nrow = Truncate(inner.size.h / Reflex::Max(stride.h, 8.0f));

					if (ncol * stride.w < inner.size.w) ncol++;

					if (nrow * stride.h < inner.size.h) nrow++;

					auto position = inner.origin;

					offsets.Clear();

					if (ncol > 0 && nrow > 0)
					{
						auto poffset = Extend(offsets, ncol * nrow).data;

						REFLEX_LOOP(y, nrow)
						{
							position.x = inner.origin.x;

							REFLEX_LOOP(x, ncol)
							{
								*poffset++ = position;

								position.x += stride.w;
							}

							position.y += stride.h;
						}
					}
				};
			}
			else
			{
				vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					auto & inner = scratch->inner;

					auto & offsets = scratch->offsets;

					inner = Indent(size, properties->indent);

					Rect tile = inner;

					bool y = True(properties->axis | kAxisPropertyY);

					auto stride = GetSize(y, properties->stride);

					SetSize(y, tile.size, stride);

					AlignLayers(scratch->content_state, tile.size, contenth);

					auto axis_size = GetSize(y, inner.size);

					auto n = Truncate(axis_size / Reflex::Max(stride, 1.0f));

					offsets.Clear();

					if (n > 0)
					{
						auto region = Extend(offsets, n);

						for (auto & i : region)
						{
							i = tile.origin;

							IncSize(y, Reinterpret<Size>(tile.origin), stride);
						}

						if (GetPoint(y, tile.origin) < (GetPoint(y, inner.origin) + axis_size))
						{
							offsets.Push(tile.origin);
						}
					}
				};
			}

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				struct GraphicImpl : public StandardLayersRenderer
				{
					GraphicImpl(Scratch & scratch, const Size & pixelsize)
						: StandardLayersRenderer(scratch.content_state, pixelsize), 
						scratch(scratch) 
					{
					}

					void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
					{
						auto & ctx = *Core::RenderContext::st_current;

						for (auto & i : scratch.offsets)
						{
							TransformScope scope = { ctx, i };

							DrawLayers(ctx, colour, layers);
						}
					}

					Scratch & scratch;
				};

				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				return REFLEX_CREATE(GraphicImpl, scratch, pixelsize);
			};
		});
	};
};

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_wrapper_layers[]
{
	{ "Colour", &GenericLayer::CreateSchema<ColourImpl>, &GenericLayer::Create<ColourImpl> },

	{ "Align", &GenericLayer::CreateSchema<AlignImpl>, &GenericLayer::Create<AlignImpl> },
	{ "Group", &GenericLayer::CreateSchema<AlignImpl>, &GenericLayer::Create<AlignImpl> },	//alias

	{ "Inline", &GenericLayer::CreateSchema<InlineImpl>, &GenericLayer::Create<InlineImpl> },

	{ "Split", &GenericLayer::CreateSchema<SplitImpl>, &GenericLayer::Create<SplitImpl> },
	{ "Tile", &GenericLayer::CreateSchema<TileImpl>, &GenericLayer::Create<TileImpl> },

	{ "Bar", &GenericLayer::CreateSchema<BarImpl>, &GenericLayer::Create<BarImpl> },
	{ "If", &GenericLayer::CreateSchema<IfImpl>, &GenericLayer::Create<IfImpl> },

	{ "Rotate", &GenericLayer::CreateSchema<RotateImpl>, &GenericLayer::Create<RotateImpl> },
	{ "Translate", &GenericLayer::CreateSchema<TranslateImpl>, &GenericLayer::Create<TranslateImpl> },
	{ "Scale", &GenericLayer::CreateSchema<ScaleImpl>, &GenericLayer::Create<ScaleImpl> },
};
