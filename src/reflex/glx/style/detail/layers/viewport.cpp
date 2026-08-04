#include "types.h"

#include "reflex/glx/detail/font.h"
#include "reflex/glx/detail/axis.h"




//
//layer

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

inline void AddLinePrimitive(ColourPoints & points, const Colour & colour, const Point & from, const Point & to)
{
	auto region = Extend(points, 2);

	region[0] = { from, colour };
	region[1] = { to, colour };
}

struct ViewPortMarkerImpl : public LayerImpl <ViewPortMarkerImpl>
{
	struct State : public System::Renderer::Graphic
	{
		State(GLX::Object & owner) : object(owner) {}

		virtual void Render(const System::Renderer::Transform & t, const Colour & colour) const override
		{
			Detail::SetPoint(m_yaxis, m_rect.origin, Quantise(m_position->value, m_axis_pix_size));

			DrawFill(t, m_colour * colour, m_rect);
		}

		const TRef <GLX::Object> object;

		bool m_yaxis;

		Float m_axis_pix_size;

		Colour m_colour;

		mutable Rect m_rect;

		ConstReference <Data::Float32Property> m_position;

		ConstReference <AbstractViewPort::ViewState> m_viewport;
	};

	static TRef <GLX::Detail::Layer> Create(const GLX::Style & stylesheet, const Object & global, const Data::PropertySet & params)
	{
		Key32 dataid = Data::GetKey32(params, K32("position_id"), GLX::kdata);

		auto width = GetNumber(params, K32("width"), 1.0f);

		auto indent = GetNumber(params, kindent);

		auto colour = ToColour(Data::GetFloat32Array(params, K32("colour")));

		return REFLEX_CREATE(ViewPortMarkerImpl, dataid, width, indent, colour);
	}

	ViewPortMarkerImpl(Key32 dataid, Float32 width, Float indent, const Colour & colour)
		: LayerImpl<ViewPortMarkerImpl>(Layer::kOptimisationFlagNotResponsive)
		, m_dataid(dataid)
		, m_width(width)
		, m_indent(indent)
		, m_colour(colour)
	{
		SetOnCreateState(&ViewPortMarkerImpl::CreateState);

		SetOnAccommodate<State>(&ViewPortMarkerImpl::Accommodate);

		SetOnAlign<State>(&ViewPortMarkerImpl::Align);

		SetOnRedraw<State>(&ViewPortMarkerImpl::Redraw);
	}

	static TRef <Reflex::Object> CreateState(const ViewPortMarkerImpl & self, GLX::Object & object)
	{
		auto state = Data::Detail::AcquireProperty<State>(object, self.m_dataid, object);

		state->m_colour = self.m_colour;

		return state;
	}

	static void Accommodate(const ViewPortMarkerImpl & self, State & state, Size & contentsize)
	{
		auto object = state.object;

		state.m_viewport = GetContainingViewPort(object)->view_state;

		state.m_position = Data::Detail::AcquireProperty<Data::Float32Property>(object, self.m_dataid);

		state.m_yaxis = GetAxis(object);
	}

	static void Align(const ViewPortMarkerImpl & self, State & state, Size size, Float & height)
	{
		bool yaxis = state.m_yaxis;

		bool ortho = !yaxis;

		auto pix = state.m_viewport->GetPixelsPerUnit();

		auto ortho_pix_size = Detail::GetSize(ortho, pix);

		state.m_axis_pix_size = Detail::GetSize(yaxis, pix);

		auto indent = self.m_indent * ortho_pix_size;

		Detail::SetPoint(ortho, state.m_rect.origin, indent);

		state.m_rect.size = Detail::MakeSize(yaxis, state.m_axis_pix_size * self.m_width, Detail::GetSize(ortho, size) - (indent + indent));
	}

	static TRef <System::Renderer::Graphic> Redraw(const ViewPortMarkerImpl & self, State & data, Size pixelsize, UInt8 flags)
	{
		return data;
	}


	Key32 m_dataid;

	Float32 m_width, m_indent;

	Colour m_colour;
};

struct ViewPortGridImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		AxisProperty axis = kAxisPropertyX;

		bool dotted = false;

		Float origin = 0.0f;
		Float unit = 1.0f;
		Float minpx = 32.0f;
		Float indent = 0.0f;

		ConstReference <Font> font;
		
		ConstReference <Reflex::Object> stringfn;

		Colour text_colour = kWhite;
		Point text_offset;
	};

	struct Scratch
	{
		Size size_z;

		Float32 unit_rcp = 1.0f;

		Reference <Graphic> lines;

		Function <WString(Float)> stringfn;

		Array <Pair < Colour, ConstReference <Graphic> > > labels;
	};

	template <bool YAXIS, bool LABELS, bool DOTTED> static void OnAlign(const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
	{
		typedef ConditionalType<YAXIS,YAxis,XAxis> AXIS;

		typedef typename AXIS::Ortho Ortho;

		auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

		auto object = data.owner;

		auto viewport = GetContainingViewPort(object);

		auto pix = viewport->GetPixelsPerUnit();

		if (SetFiltered(scratch->size_z, size * pix))
		{
			auto [vo,vr] = viewport->GetView();
			
			Pair <bool> inversion = viewport->inverted;
			
			
			auto step_px = properties->unit / AXIS::GetSize(pix);
			
			if (step_px <= 0.0) return;

			while (step_px < properties->minpx) step_px = step_px * 2.0f;
			
			auto dbl_minpx = properties->minpx * 2.0f;
			
			while (step_px > dbl_minpx) step_px *= 0.5f;
			
			auto alpha = step_px / properties->minpx;
			
			alpha = Modulo(alpha, 1.0f);
			
			alpha = Square(alpha);
			
			
			auto step = step_px * AXIS::GetSize(pix);
			
			Float s = AXIS::GetPoint(vo);
			
			Float start_value = s - Modulo(s, step * 2.0f);
			
			Float end = s + AXIS::GetSize(vr);
			
			Float pixw = AXIS::GetSize(pix);
			
			Float halfpixw = pixw * 0.5f;
			
			
			
			//generate lines
			
			Colour colours[2] = { properties->colour, properties->colour };
			
			Colour text_colours[2] = { properties->text_colour, properties->text_colour };
			
			colours[1].a *= alpha;
			
			text_colours[1].a *= alpha;
			
			
			Float indent = properties->indent * Ortho::GetSize(pix);
			
			Float line_start = indent;
			
			Float line_end = AXIS::Ortho::GetSize(size) - indent;
			
			if ((&inversion.a)[Ortho::kY])
			{
				line_start += Ortho::GetSize(pix);
				
				line_end += Ortho::GetSize(pix);
			}
			
			
			auto & stringfn = scratch->stringfn;
			
			auto & labels = scratch->labels;
			
			GLX_GET_COLOURPOINT_WORKSPACE(lines);
			
			UInt idx = 0;
			
			Float origin = properties->origin;
			
			Float pos = start_value - origin;
			
			
			//label positions
			
			auto & font = *properties->font;
			
			Point text_offset;
			
			Scale text_scale = { 1.0f, 1.0f };
			
			if constexpr (AXIS::kX & LABELS)
			{
				labels.Clear();
				
				text_scale = Reinterpret<Scale>(pix);
				
				text_offset.x = properties->text_offset.x * text_scale.w;
				
				if (inversion.b)
				{
					text_offset.y = (properties->text_offset.y * text_scale.h);
					
					text_offset.y += vo.y;
					
					text_scale.h *= -1.0f;
				}
				else
				{
					text_offset.y = (properties->text_offset.y * -1.0f * text_scale.h);
					
					text_offset.y += (vo.y + vr.h);
				}
			}
			
			REFLEX_ASSERT(((end - pos) / step) < 1024.0f);
			
			GLX_GET_POINT_WORKSPACE(points);
			
			end += Core::kRoundingTolerance;

			while (pos < end)
			{
				Float qpos = Quantise(pos, pixw);
				
				Float linepos = qpos += halfpixw;

				auto & colour = colours[idx];
				
				if constexpr (DOTTED)
				{
					points.Clear();
					
					AddDottedLine(points, AXIS::MakePoint(linepos, line_start), AXIS::MakePoint(linepos, line_end), pix);
					
					AddPointsWithColour(lines, points, colour);
				}
				else
				{
					//linepos += halfpixw;

					AddLinePrimitive(lines, colour, AXIS::MakePoint(linepos, line_start), AXIS::MakePoint(linepos, line_end));
				}
				
				if constexpr (AXIS::kX & LABELS)
				{
					Point position = { qpos + text_offset.x, text_offset.y };
					
					labels.Push({ text_colours[idx], font.CreateText(stringfn(pos + origin), position, text_scale).a });
				}
				
				idx ^= 1;
				
				pos += step;
			}
						
			scratch->lines = Core::g_renderer->CreatePrimitives(DOTTED ? System::Renderer::kPrimitiveTypePoints : System::Renderer::kPrimitiveTypeLines, lines);
		}
	}

	REFLEX_TBINDER_3P(OnAlign);

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, colour), "color", kPropertyGroupColour, kBindStageRedraw);
		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, colour), "colour", kPropertyGroupColour, kBindStageRedraw);

		RegisterAxisProperty(schema, REFLEX_OFFSETOF(Properties, axis), "axis", kPropertyGroupNone, kBindStageNone);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, origin), "origin", kPropertyGroupNone, kBindStageAccommodate);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, unit), "unit", kPropertyGroupNone, kBindStageAccommodate);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, minpx), "minpx", kPropertyGroupNone, kBindStageNone);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, indent), "indent", kPropertyGroupNone, kBindStageNone);
		
		schema.Pop();

		RegisterBoolProperty(schema, REFLEX_OFFSETOF(Properties, dotted), "dotted", kPropertyGroupNone, kBindStageNone);

		RegisterReferenceProperty<Font>(schema, REFLEX_OFFSETOF(Properties, font), "font", kPropertyGroupCustom0);

		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, text_colour), "text_colour", kPropertyGroupColour, kBindStageAccommodate);

		RegisterColourProperty(schema, REFLEX_OFFSETOF(Properties, text_colour), "text_color", kPropertyGroupColour, kBindStageAccommodate);

		RegisterPointProperty(schema, REFLEX_OFFSETOF(Properties, text_offset), "text_offset", kPropertyGroupNone, kBindStageNone);


		REFLEX_STATIC_ASSERT((IsType<decltype(Properties::origin),Float>::value));

		using StringFnObject = ObjectOf < Function<WString(Float)> >;


		schema.RegisterProperty({ REFLEX_OFFSETOF(Properties, stringfn), 0, kBindStageAccommodate, "label", MakeAddress<StringFnObject>(K32("label")), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			*Cast<ConstReference<Reflex::Object>>(adr) = object;
		} });

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive;

			auto properties = Cast<Properties>(pproperties);
			
			vtable.OnAccommodate = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size & contentsize)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				scratch->unit_rcp = 1.0f / Reflex::Max(properties->unit, Core::kRoundingTolerance);

				if (self.propertyflags & kPropertyGroupCustom0)
				{
					if (auto & stringfnref = properties->stringfn)
					{
						scratch->stringfn = Cast<StringFnObject>(stringfnref)->value;
					}
					else
					{
						scratch->stringfn = [](Float32)
						{
							return WString();
						};
					}
				}

				scratch->size_z = {};
			};

			vtable.OnAlign = OnAlignBinder::Bind(MakeBits(properties->axis == Bits<false,true>::value, True(self.propertyflags & kPropertyGroupCustom0), properties->dotted));

			if (self.propertyflags & kPropertyGroupCustom0)	//font
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					struct GraphicImpl : public Graphic
					{
						GraphicImpl(const Graphic & lines, const Array < Pair <Colour, ConstReference <Graphic> > > & labels) : lines(lines), labels(labels) {}

						virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
						{
							lines.Render(transform, colour);

							for (auto & i : labels)
							{
								i.b->Render(transform, colour * i.a);
							}
						}

						const Graphic & lines;

						const Array < Pair <Colour, ConstReference <System::Renderer::Graphic> > > & labels;

						Colour colour;
					};

					return REFLEX_CREATE(GraphicImpl, scratch->lines, scratch->labels);
				};
			}
			else
			{
				vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
				{
					auto [properties, scratch] = data.GetPropertiesAndScratch<Properties, Scratch>();

					return scratch->lines;
				};
			}
		});
	}
};

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_viewport_layers[]
{
	{ "ViewPortMarker", [](Key32 uid) { return Null<Reflex::Object>(); }, &ViewPortMarkerImpl::Create },
	{ "ViewPortGrid", &GenericLayer::CreateSchema<ViewPortGridImpl>, &GenericLayer::Create<ViewPortGridImpl> }
};
