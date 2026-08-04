#include "types.h"
#include "../../../functions/drawing.h"



//
//boxlayer

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct GradientProperties : public StandardProperties
{
	static void Register(const char * name, GenericPropertiesSchema & schema)
	{
		RegisterIndent(schema);

		RegisterColourProperty(schema, REFLEX_OFFSETOF(GradientProperties, from), "from");
		RegisterColourProperty(schema, REFLEX_OFFSETOF(GradientProperties, to), "to");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(GradientProperties, dither), "dither", kPropertyGroupCustom0, kBindStageNone);
	}

	Colour from = kWhite;
	Colour to = kTransparent;
	Float dither = 0.0f;
};

struct DitheredRenderer : public Graphic
{
	DitheredRenderer(const Graphic & graphic, Float amount)
		: m_graphic(graphic),
		m_amount(amount)
	{
	}

	void Render(const System::Renderer::Transform & transform, const Colour & colour) const
	{
		Core::g_renderer->SetDitheringAmount(m_amount);

		m_graphic->Render(transform, colour);

		Core::g_renderer->SetDitheringAmount(0.0f);
	}

	const ConstReference <Graphic> m_graphic;

	const Float m_amount;
}; 

struct GradientImpl
{
	struct Properties : public GradientProperties
	{
		Float angle = 0.0f;
	};

	typedef StandardScratch Scratch;

	static TRef <Graphic> CreateDitheredRenderer(const GenericLayer & layer, GenericLayer::ObjectState & state, Size pixelsize, FunctionPointer <void(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)> generate)
	{
		auto amount = state.GetProperties<GradientProperties>()->dither;

		ColourPoints points;

		generate(layer, state, pixelsize, points);

		return New<DitheredRenderer>(CreateGraphic(points), amount);
	}

	static void Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points);

	static void Init(GenericPropertiesSchema & schema)
	{
		GradientProperties::Register("Gradient", schema);

		RegisterAngleProperty(schema, REFLEX_OFFSETOF(Properties, angle));

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			vtable.OnAlign = BindStandardAlign(self);

			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive;

			if (self.propertyflags & kPropertyGroupCustom0)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags)
				{
					return CreateDitheredRenderer(self, data, pixelsize, &GradientImpl::Generate);
				};
			}
			else
			{
				RemoveConst(self.flags) |= Layer::kOptimisationFlagVector;

				vtable.OnVectoriseColour = &GradientImpl::Generate;
			}
		});
	};
};

REFLEX_NOINLINE void GradientImpl::Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
{
	auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

	auto & rect = scratch->inner;

	auto x2 = rect.origin.x + rect.size.w;
	auto y2 = rect.origin.y + rect.size.h;

	System::ColourPoint vertices[] =
	{
		{ rect.origin, {} },
		{ { x2, rect.origin.y }, {} },
		{ { rect.origin.x, y2 }, {} },
		{ { x2, y2 }, {} },
	};

	Float32 radians = properties->angle * k2Pif;

	auto direction = Normalise({ Cos(radians), Sin(radians) });

	Float32 minProj = DotProduct(vertices[0].a, direction);

	Float32 maxProj = minProj;

	for (auto & i : vertices)
	{
		Float32 proj = DotProduct(i.a, direction);

		if (proj < minProj) minProj = proj;

		if (proj > maxProj) maxProj = proj;
	}

	auto mult = 1.0f / (maxProj - minProj);

	auto & from = properties->from;

	auto & to = properties->to;

	for (auto & i : vertices)
	{
		Float32 proj = DotProduct(i.a, direction);

		Float32 t = (proj - minProj) * mult;// / (maxProj - minProj);

		i.b = LinearInterpolate(t, from, to);
	}

	AllocateExtra(points, 4 * 3);

	REFLEX_LOOP(idx, 2)
	{
		auto tri = Extend<kAllocateNone>(points, 3);

		if (idx & 1)
		{
			tri[0] = vertices[idx];
			tri[1] = vertices[idx + 2];
			tri[2] = vertices[idx + 1];
		}
		else
		{
			tri[0] = vertices[idx];
			tri[1] = vertices[idx + 1];
			tri[2] = vertices[idx + 2];
		}
	}
}

struct CircleGradientImpl
{
	using Properties = GradientProperties;

	using Scratch = StandardScratch;

	static void Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points);

	static void Init(GenericPropertiesSchema & schema)
	{
		GradientProperties::Register("CircleGradient", schema);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			vtable.OnAlign = BindStandardAlign(self);

			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive;

			if (self.propertyflags & kPropertyGroupCustom0)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags)
				{
					return GradientImpl::CreateDitheredRenderer(self, data, pixelsize, &CircleGradientImpl::Generate);
				};
			}
			else
			{
				RemoveConst(self.flags) |= Layer::kOptimisationFlagVector;

				vtable.OnVectoriseColour = &CircleGradientImpl::Generate;
			}
		});
	};
};

REFLEX_NOINLINE void CircleGradientImpl::Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
{
	auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

	auto & from = properties->from;
	auto & to = properties->to;
	auto & rect = scratch->inner;
	auto & size = rect.size;

	if ((size.w * size.h) != 0.0f)
	{
		Size radius = size * 0.5f;

		Point center = rect.origin + Reinterpret<Point>(radius);

		auto step = CalculateCircleStep(rect.size);

		auto segments = Truncate(k2Pif / step) + 1;

		AllocateExtra(points, segments * 3);

		Float angle = 0.0f;
		Point previous = center + CalculateCirclePoint(radius, angle);

		REFLEX_LOOP(i, segments)
		{
			Float next_angle = Min(angle + step, k2Pif);

			Point next = center + CalculateCirclePoint(radius, next_angle);

			points.Push<kAllocateNone>({ center, from });
			points.Push<kAllocateNone>({ previous, to });
			points.Push<kAllocateNone>({ next, to });

			previous = next;
			angle = next_angle;
		}
	}
}

struct ShadowImpl
{
	struct Properties : public StandardProperties
	{
		static constexpr Colour kDefaultColour = RGB(0, 0, 0, 64);

		Colour colour = kDefaultColour;
		Float blur = 8.0f;
		Float corner = 0.0f;
		Float dither = 0.0f;
	};

	struct Scratch : public StandardScratch
	{
		Float32 corner;
		Float32 actual_corner;
		Colour colour_adjusted;
		Colour opaque;
	};

	//static Point MorphCornerPoint(Float roundness, const Size & radius, Float angle)
	//{
	//	Float sin = Sin(angle);
	//	Float cos = Cos(angle);

	//	Float mult = 1.414f;

	//	Float x = Clip(LinearInterpolate(roundness, mult * cos, cos), -1.0f, 1.0f) * radius.w;
	//	Float y = Clip(LinearInterpolate(roundness, mult * sin, sin), -1.0f, 1.0f) * radius.h;

	//	return { x, y };
	//}

	//REFLEX_INLINE static void MakePie(ColourPoints & points, const Colour & inner, const Colour & outer, const Rect & rect, Float roundness, Float angle, Float sweep, Float step)
	//{
	//	auto & size = rect.size;

	//	Size radius = size * 0.5f;

	//	System::ColourPoint center = { rect.origin + Reinterpret<Point>(radius), inner };

	//	System::ColourPoint p0 = { {}, outer };

	//	System::ColourPoint p1 = { {}, outer };

	//	auto angle_end = angle + sweep;

	//	p0.a = center.a + MorphCornerPoint(roundness, radius, angle);

	//	while (angle < angle_end)
	//	{
	//		angle += step;

	//		auto angle1 = Min(angle, angle_end);

	//		p1.a = center.a + MorphCornerPoint(roundness, radius, angle1);

	//		auto region = Extend(points, 3);

	//		region[0] = p0;
	//		region[1] = center;
	//		region[2] = p1;

	//		p0.a = p1.a;
	//	}
	//}

	static void Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points);

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, blur), "blur");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, corner), "corner");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, dither), "dither", kPropertyGroupCustom0, kBindStageNone);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				StandardIndent(layer, data, size, contenth);

				auto radius = properties->blur;

				scratch->corner = Max(properties->corner, radius * 0.5f);
				scratch->actual_corner = radius + scratch->corner;

				auto colour_adjusted = properties->colour;
				colour_adjusted.a *= 0.5f;
				scratch->colour_adjusted = colour_adjusted;
				colour_adjusted.a = 0.0f;
				scratch->opaque = colour_adjusted;
			};

			if (self.propertyflags & kPropertyGroupCustom0)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					GLX_GET_COLOURPOINT_WORKSPACE(points);

					Generate(self, data, pixelsize, points);

					return New<DitheredRenderer>(CreateGraphic(points), data.GetProperties<Properties>()->dither);
				};
			}
			else
			{
				RemoveConst(self.flags) |= Layer::kOptimisationFlagVector;

				vtable.OnVectoriseColour = &Generate;
			}
		});
	}

	REFLEX_INLINE static void AddFeatherCornerFan(ColourPoints & points, Point centre, Size radius, Float32 start_angle, Float32 end_angle, const Colour & inner_colour, const Colour & outer_colour, UInt segments)
	{
		Float32 step = (end_angle - start_angle) / Float32(segments);
		Float32 step_cos = Cos(step);
		Float32 step_sin = Sin(step);
		Float32 cos0 = Cos(start_angle);
		Float32 sin0 = Sin(start_angle);
		
		Point p0 = { centre.x + (cos0 * radius.w), centre.y + (sin0 * radius.h) };

		auto ptr = Extend(points, segments * 3).data;

		REFLEX_LOOP(i, segments)
		{
			Float32 cos1 = (cos0 * step_cos) - (sin0 * step_sin);
			Float32 sin1 = (sin0 * step_cos) + (cos0 * step_sin);
			
			Point p1 = { centre.x + (cos1 * radius.w), centre.y + (sin1 * radius.h) };

			*ptr++ = { centre, inner_colour };
			*ptr++ = { p0, outer_colour };
			*ptr++ = { p1, outer_colour };

			cos0 = cos1;
			sin0 = sin1;
			p0 = p1;
		}
	}

	REFLEX_INLINE static void AddRoundedFeatheredRect(ColourPoints & points, const Rect & rect, Float feather_width, const Colour & inner_colour, const Colour & outer_colour, UInt corner_segments = 5)
	{
		Float32 x1 = rect.origin.x;
		Float32 y1 = rect.origin.y;
		Float32 x2 = x1 + rect.size.w;
		Float32 y2 = y1 + rect.size.h;

		Float32 ox1 = x1 - feather_width;
		Float32 oy1 = y1 - feather_width;
		Float32 ox2 = x2 + feather_width;
		Float32 oy2 = y2 + feather_width;

		AddGradientImpl(points, x1, oy1, x2, y1, true, outer_colour, inner_colour);
		AddGradientImpl(points, x1, y2, x2, oy2, true, inner_colour, outer_colour);
		AddGradientImpl(points, ox1, y1, x1, y2, false, outer_colour, inner_colour);
		AddGradientImpl(points, x2, y1, ox2, y2, false, inner_colour, outer_colour);

		auto corner_size = Reflex::MakeSize(feather_width);

		AddFeatherCornerFan(points, { x1, y1 }, corner_size, kPif, kPif * 1.5f, inner_colour, outer_colour, corner_segments);
		AddFeatherCornerFan(points, { x2, y1 }, corner_size, kPif * 1.5f, kPif * 2.0f, inner_colour, outer_colour, corner_segments);
		AddFeatherCornerFan(points, { x2, y2 }, corner_size, 0.0f, kPif * 0.5f, inner_colour, outer_colour, corner_segments);
		AddFeatherCornerFan(points, { x1, y2 }, corner_size, kPif * 0.5f, kPif, inner_colour, outer_colour, corner_segments);

		AddRectFill(points, inner_colour, rect);
	}
};

REFLEX_NOINLINE void ShadowImpl::Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
{
	auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

	auto radius = properties->blur;

	auto colour_adjusted = scratch->colour_adjusted;
	auto & opaque = scratch->opaque;

	const auto compensated_rect = Indent(scratch->inner, scratch->corner);

	REFLEX_LOOP(pass, 3)
	{
		auto resize = Indent(compensated_rect, radius * 0.5f);	//AddRoundedFeatheredRect extends rect by actual_corner, so need to compensate

		AddRoundedFeatheredRect(points, resize, scratch->actual_corner, colour_adjusted, opaque);

		radius *= 0.75f;

		colour_adjusted.a *= 0.75f;
	}
}

struct InnerShadowImpl
{
	struct Properties : public StandardProperties
	{
		Colour colour = kBlack;
		Margin width = MakeMargin(4.0f);
		Float dither = 0.0f;
	};

	struct Scratch : public StandardScratch
	{
		Margin width_adjusted;
		Colour colour_adjusted, opaque;
	};

	static void Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points);

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);
		RegisterMarginProperty(schema, REFLEX_OFFSETOF(Properties, width), "width");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, dither), "dither", kPropertyGroupCustom0, kBindStageNone);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

				StandardIndent(layer, data, size, contenth);

				auto width_adjusted = properties->width;
				width_adjusted.near *= 1.125f;
				width_adjusted.far *= 1.125f;
				scratch->width_adjusted = width_adjusted;

				scratch->colour_adjusted = properties->colour;
				scratch->colour_adjusted.a = 1.0f - cbrtf(1.0f - properties->colour.a);
				scratch->opaque = scratch->colour_adjusted;
				scratch->opaque.a = 0.0f;
			};

			if (self.propertyflags & kPropertyGroupCustom0)
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					GLX_GET_COLOURPOINT_WORKSPACE(points);

					Generate(self, data, pixelsize, points);

					return New<DitheredRenderer>(CreateGraphic(points), data.GetProperties<Properties>()->dither);
				};
			}
			else
			{
				RemoveConst(self.flags) |= Layer::kOptimisationFlagVector;

				vtable.OnVectoriseColour = &Generate;
			}
		});
	}
};

REFLEX_NOINLINE void InnerShadowImpl::Generate(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, ColourPoints & points)
{
	auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();
	auto & rect = scratch->inner;
	auto & width = scratch->width_adjusted;
	auto & colour = scratch->colour_adjusted;
	auto & opaque = scratch->opaque;

	Float x1 = rect.origin.x;
	Float y1 = rect.origin.y;
	Float x2 = x1 + rect.size.w;
	Float y2 = y1 + rect.size.h;

	Float left = width.near.w;
	Float right = width.far.w;
	Float top = width.near.h;
	Float bottom = width.far.h;

	bool has_left = True(left);
	bool has_right = True(right);
	bool has_top = True(top);
	bool has_bottom = True(bottom);

	REFLEX_LOOP(idx, 3)
	{
		Float x1i = x1 + left;
		Float y1i = y1 + top;
		Float x2i = x2 - right;
		Float y2i = y2 - bottom;

		if (has_left) AddGradientImpl(points, x1, y1i, x1i, y2i, false, colour, opaque);
		if (has_right) AddGradientImpl(points, x2i, y1i, x2, y2i, false, opaque, colour);
		if (has_top) AddGradientImpl(points, x1i, y1, x2i, y1i, true, colour, opaque);
		if (has_bottom) AddGradientImpl(points, x1i, y2i, x2i, y2, true, opaque, colour);

		if (has_left || has_top) AddGradientImpl(points, x1, y1, x1i, y1i, colour, colour, colour, opaque); // TL
		if (has_right || has_top) AddGradientImpl<true>(points, x2i, y1, x2, y1i, colour, colour, opaque, colour); // TR
		if (has_left || has_bottom) AddGradientImpl<true>(points, x1, y2i, x1i, y2, colour, opaque, colour, colour); // BL
		if (has_right || has_bottom) AddGradientImpl(points, x2i, y2i, x2, y2, opaque, colour, colour, colour); // BR

		left *= 0.75f;
		right *= 0.75f;
		top *= 0.75f;
		bottom *= 0.75f;
	}
}

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_gradient_layers[4]
{
	{ "Gradient", &GenericLayer::CreateSchema<GradientImpl>, &GenericLayer::Create<GradientImpl> },
	{ "CircleGradient", &GenericLayer::CreateSchema<CircleGradientImpl>, &GenericLayer::Create<CircleGradientImpl> },

	{ "Shadow", &GenericLayer::CreateSchema<ShadowImpl>, &GenericLayer::Create<ShadowImpl> },
	{ "InnerShadow", &GenericLayer::CreateSchema<InnerShadowImpl>, &GenericLayer::Create<InnerShadowImpl> },
};
