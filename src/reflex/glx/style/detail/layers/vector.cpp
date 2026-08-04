#include "types.h"
#include "reflex/glx/detail/functions/geometry.h"



//
//vector layers

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

inline Corners MakeCorners(Float left, Float right)
{
	auto left_size = Reflex::MakeSize(left);
	auto right_size = Reflex::MakeSize(right);

	return { left_size, right_size, left_size, right_size };
}

inline Corners MakeCorners(Float tl, Float tr, Float bl, Float br)
{
	return { Reflex::MakeSize(tl), Reflex::MakeSize(tr), Reflex::MakeSize(bl), Reflex::MakeSize(br) };
}

inline Corners MakeCorners(Float tl_x, Float tr_x, Float bl_x, Float br_x, Float tl_y, Float tr_y, Float bl_y, Float br_y)
{
	return { { tl_x, tl_y }, { tr_x, tr_y }, { bl_x, bl_y }, { br_x, br_y } };
}

inline Corners MakeScaledCorners(const Corners & corners, Size pixelsize, Size max)
{
	return
	{
		Min(corners.tl * pixelsize, max),
		Min(corners.tr * pixelsize, max),
		Min(corners.bl * pixelsize, max),
		Min(corners.br * pixelsize, max),
	};
}

void RegisterCornerProperty(GenericPropertiesSchema & schema, UIntNative offset, PropertyGroup flags)
{
	constexpr const char * kCorner = "corner";

	constexpr Key32 id = kCorner;

	auto defs = schema.RegisterProperties(1);

	defs[0] = 
	{ 
		UInt16(offset), 
		flags, 
		kBindStageNone, 
		kCorner,
		MakeAddress<Data::ArrayOfFloat32Property>(id), 
		0, 
		[](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto values = ToView(Cast<Data::ArrayOfFloat32Property>(object)->value);

			switch (values.size)
			{
			case 1:
				*Cast<Corners>(adr) = GLX::Detail::MakeCorners(values.GetFirst());
				break;

			case 2:
				if (stylesheet_flags & 1)
				{
					*Cast<Corners>(adr) = MakeCorners(values[0], values[1], values[1], values[0]);
				}
				else
				{
					*Cast<Corners>(adr) = MakeCorners(values[0], values[1]);
				}
				break;

			case 4:
				*Cast<Corners>(adr) = MakeCorners(values[0], values[1], values[(stylesheet_flags & 1) ? 3 : 2], values[(stylesheet_flags & 1) ? 2 : 3]);
				break;

			case 8:
				*Cast<Corners>(adr) = MakeCorners(values[0], values[1], values[2], values[3], values[4], values[5], values[6], values[7]);
				break;
			}
		} 
	};

	//defs[2] = { UInt16(offset), flags, kBindStageNone, name.data, MakeAddress<Data::Key32Property>(id), object_data, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
	//{
	//	auto id = Cast<Data::Key32Property>(object)->value;

	//	if (auto p = FindResource(st_current_style, Address{ id, Reinterpret<TypeID>(data) }, 0))
	//	{
	//		auto & tuple = Reinterpret<Tuple<UInt32, UInt16, UInt16>>(data);

	//		MemCopy(Reinterpret<UInt8>(p) + tuple.b, adr, tuple.c);
	//	}
	//} };
}

struct FillImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		Corners corners;
	};

	typedef StandardScratch Scratch;


	static void Init(GenericPropertiesSchema & schema)
	{	
		RegisterIndentAndColour(schema);
		
		RegisterCornerProperty(schema, REFLEX_OFFSETOF(Properties,corners), kPropertyGroupCustom0);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = BindStandardAlign(self);

			if (self.propertyflags & kPropertyGroupCustom0)
			{
				vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					colour = properties->colour;

					auto max = scratch->inner.size * Reflex::MakeSize(0.5f);

					AddRoundedFill(points, scratch->inner, MakeScaledCorners(properties->corners, pixelsize, max));
				};
			}
			else
			{
				vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
				{
					auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

					colour = properties->colour;

					AddRectFill(points, scratch->inner);
				};
			}
		});
	}

	REFLEX_INLINE static Size ToWorld(Size pixelsize, Size max, Float rpx)
	{
		Float rx = rpx * pixelsize.w;
		Float ry = rpx * pixelsize.h;

		return Min({ rx, ry }, max);
	}
};

struct BorderImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		Margin width = MakeMargin(1.0f);
		Corners corners;
	};
	
	typedef StandardScratch Scratch;

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);
		
		RegisterMarginProperty(schema, REFLEX_OFFSETOF(Properties,width), "width");
		RegisterCornerProperty(schema, REFLEX_OFFSETOF(Properties, corners), kPropertyGroupCustom0);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = BindStandardAlign(self);

			vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				colour = properties->colour;

				if (self.propertyflags & kPropertyGroupCustom0)
				{
					SIMD::FloatV4 mult = { pixelsize.w, pixelsize.h, pixelsize.w, pixelsize.h };

					auto width = Reinterpret<Margin>(Reinterpret<SIMD::FloatV4>(properties->width) * mult);

					auto max = scratch->inner.size * Reflex::MakeSize(0.5f);
					
					AddRoundedOutline(points, scratch->inner, width, MakeScaledCorners(properties->corners, pixelsize, max));
				}
				else
				{
					AddRectOutline(points, scratch->inner, properties->width, pixelsize);
				}
			};
		});
	}
};

struct CircleFillImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		Range sweep = kNormalRange;
	};

	typedef StandardScratch Scratch;

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterRangeProperty(schema, REFLEX_OFFSETOF(Properties, sweep), "sweep", kPropertyGroupCustom0, kBindStageRealign);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = BindStandardAlign(self);

			vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				colour = properties->colour;

				if (self.propertyflags & kPropertyGroupCustom0)
				{
					auto [start,length] = properties->sweep;

					length = Clip(length, -1.0f, 1.0f) * k2Pif;

					AddEllipseFill(points, scratch->inner, start * k2Pif, length);
				}
				else
				{
					AddEllipseFill(points, scratch->inner);
				}
			};
		});
	};
};

struct CircleImpl
{
	struct Properties : public CircleFillImpl::Properties
	{
		Float width = 1.0f;
		bool round_cap = false;
	};

	typedef StandardScratch Scratch;
	
	static void Init(GenericPropertiesSchema & schema)
	{
		CircleFillImpl::Init(schema);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties,width), "width");
		RegisterBoolProperty(schema, REFLEX_OFFSETOF(Properties, round_cap), "round_cap", kPropertyGroupNone, kBindStageNone);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = BindStandardAlign(self);

			vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				colour = properties->colour;

				Size width = { properties->width * pixelsize.w, properties->width * pixelsize.h };

				if (self.propertyflags & kPropertyGroupCustom0)	//sweep property is set
				{
					auto [start, length] = properties->sweep;

					length = Clip(length, -1.0f, 1.0f) * k2Pif;

					auto add_elllipse_fn = properties->round_cap ? &AddEllipseOutlineRoundCapped : &AddEllipseOutline;

					add_elllipse_fn(points, scratch->inner, width, start * k2Pif, length);
				}
				else
				{
					AddEllipseOutline(points, scratch->inner, width);
				}
			};
		});
	};
};

struct TriangleFillImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		Pair <Orientation> direction = { kOrientationCenter, kOrientationNear };
		Float corner = 0.0f;
	};

	typedef StandardScratch Scratch;

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterAlignmentProperty(schema, REFLEX_OFFSETOF(Properties, direction), "direction");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, corner), "corner", kPropertyGroupCustom0);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = BindStandardAlign(self);

			vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				colour = properties->colour;

				auto direction = OrientationToAlignment(properties->direction);

				if (self.propertyflags & kPropertyGroupCustom0)
				{
					AddRoundedTriangleFill(points, scratch->inner, properties->corner, direction, pixelsize);
				}
				else
				{
					AddTriangleFill(points, scratch->inner, direction);
				}
			};
		});
	}
};

struct TriangleImpl
{
	struct Properties : public TriangleFillImpl::Properties
	{
		Float width = 1.0f;
	};

	typedef StandardScratch Scratch;

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterAlignmentProperty(schema, REFLEX_OFFSETOF(Properties,direction), "direction");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, corner), "corner", kPropertyGroupCustom0);
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties,width), "width");

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = BindStandardAlign(self);

			vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				colour = properties->colour;

				auto direction = OrientationToAlignment(properties->direction);

				if (self.propertyflags & kPropertyGroupCustom0)
				{
					AddRoundedTriangleOutline(points, scratch->inner, properties->width, properties->corner, direction, pixelsize);
				}
				else
				{
					AddTriangleOutline(points, scratch->inner, properties->width, direction, pixelsize);
				}
			};
		});
	};
};

struct LineImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		Pair <Orientation> position = { kOrientationNear, kOrientationNear };
		Float width = 1.0f;
		Array <Float32> pattern;
	};

	struct Scratch : public StandardScratch
	{
		Rect line;
		bool y = false;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterAlignmentProperty(schema, REFLEX_OFFSETOF(Properties,position), "position");
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties,width), "width");

		constexpr auto pattern_offset = UInt16(REFLEX_OFFSETOF(Properties, pattern));
		constexpr auto pattern_name = "pattern";
		constexpr auto pattern_id = K32("pattern");

		schema.RegisterProperty({ pattern_offset, kPropertyGroupCustom0, kBindStageRealign, pattern_name, MakeAddress<Data::Key32Property>(pattern_id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			auto & pattern = *Cast<Array<Float>>(adr);

			switch (Cast<Data::Key32Property>(object)->value.value)
			{
			case K32("dotted"):
				pattern = { 1.0f, 1.0f };
				break;

			case K32("dashed"):
				pattern = { 5.0f, 2.0f };
				break;
			}
		} });

		schema.RegisterProperty({ pattern_offset, kPropertyGroupCustom0, kBindStageRealign, pattern_name, MakeAddress<Data::ArrayOfFloat32Property>(pattern_id), 0, [](const Reflex::Object & object, void * adr, UInt64 data, UInt16 stylesheet_flags)
		{
			*Cast<Array<Float>>(adr) = Cast<Data::ArrayOfFloat32Property>(object)->value;
		} });

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				StandardIndent(layer, data, size, contenth);

				auto & inner = scratch->inner;

				auto & line = scratch->line;

				auto position = properties->position;

				auto width = properties->width;

				auto alignmentex = OrientationToAlignmentEx(position);

				scratch->y = kAlignToAxis[alignmentex];

				auto ortho = !scratch->y;

				auto orientation = (&position.a)[ortho];

				line.size = MakeSize(scratch->y, GetSize(scratch->y, inner.size), width);

				auto pos = GetPoint(ortho, inner.origin) + Align1D(GetSize(ortho, inner.size), width, orientation);

				line.origin = MakePoint(ortho, pos, GetPoint(scratch->y, inner.origin));
			};

			vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				colour = properties->colour;

				auto & line = scratch->line;

				auto end = line.origin + line.size;

				if (properties->pattern)
				{
					AddDottedLine(points, properties->pattern, line, end, scratch->y);
				}
				else
				{
					AddQuad<kAllocateOver>(points, line.origin.x, line.origin.y, end.x, end.y);
				}
			};
		});
	}

	static constexpr bool kAlignToAxis[] =
	{
		false,   // Near, Horizontal
		false, // Center, Horizontal
		false,    // Far, Horizontal
		false,    // Fit, Horizontal

		true,    // Near, Vertical
		true,  // Center, Vertical
		true,     // Far, Vertical
		false,     // Fit, Vertical

		false,   // Near, Horizontal
		false, // Center, Horizontal
		false,    // Far, Horizontal
		false,    // Fit, Horizontal

		true,    // Near, Vertical
		true,  // Center, Vertical
		true,     // Far, Vertical
		true      // Fit, Vertical
	};

	static void AddDottedLine(Array <Point> & points, const Array <Float> & pattern, const Rect & line, Point end, bool y)
	{
		bool ortho = !y;

		auto origin = line.origin;

		UInt idx = 0;

		auto n = pattern.GetSize();
		
		auto axispos = GetPoint(y, line.origin);

		Float length = GetSize(y, line.size);

		auto axisend = axispos + length;

		while (axispos < axisend)
		{
			Float segmentLength = pattern[idx++ % n];

			auto gap = pattern[idx++ % n];

			segmentLength = Reflex::Min(segmentLength, axisend - axispos);

			auto from = MakePoint(y, axispos, GetPoint(ortho, origin));
					
			auto to = MakePoint(y, axispos + segmentLength, GetPoint(ortho, end));

			AddQuad<kAllocateOver>(points, from.x, from.y, to.x, to.y);

			axispos += segmentLength + gap;
		}
	}
};

struct AbstractPolygonImpl
{
	struct Properties : public StandardPropertiesWithColourAndUnit
	{
		static const bool kPath = false;

		bool normalized = false;

		ConstReference <Data::ArrayOfFloat32Property> block;
	};

	struct Scratch : public StandardScratch
	{
		Size size_z;
		ArrayView <Point> points;
	};

	template <class PROPERTIES> static void VectoriseMonochrome(const GenericLayer & self, const GenericLayer::ObjectState & data, Size pixelsize, Colour & colour, Points & points)
	{
		auto [properties,scratch] = data.GetPropertiesAndScratch<PROPERTIES,Scratch>();

		colour = properties->colour;

		auto path = scratch->points;

		if (properties->normalized)
		{
			GLX_GET_POINT_WORKSPACE(workspace);

			workspace = path;

			Rescale(workspace, Reinterpret<Scale>(scratch->inner.size));

			path = workspace;
		}

		if constexpr (PROPERTIES::kPath)
		{
			auto join_cap = properties->join_cap;

			if (Reinterpret<UInt16>(join_cap))
			{
				AddPath(points, path, properties->closed, properties->width, PathJoin(join_cap.a), PathCap(join_cap.b), 4.0f, pixelsize);
			}
			else
			{
				AddPath(points, path, properties->closed, properties->width, pixelsize);
			}
		}
		else
		{
			AddPolygonFill(points, path);
		}

		if (Reinterpret<UInt64>(scratch->inner.origin))
		{
			Translate(points, scratch->inner.origin);
		}
	}

	static void InitCommon(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterBoolProperty(schema, REFLEX_OFFSETOF(Properties,normalized), "normalized", kPropertyGroupCustom0, kBindStageNone);
		RegisterFloatArrayProperty(schema, REFLEX_OFFSETOF(Properties, block), "points", kPropertyGroupNone, kBindStageRealign);
	}
	
	static void OnAlign(const GenericLayer & layer, GenericLayer::ObjectState & data, Size size, Float & contenth)
	{
		StandardIndent(layer, data, size, contenth);

		auto properties = data.GetProperties<Properties>();

		auto scratch = data.GetScratch<Scratch>(layer);

		auto & points = properties->block->value;

		scratch->points = { Reinterpret<Point>(points.GetData()), points.GetSize() / 2};
	}
};

struct PolygonImpl : public AbstractPolygonImpl
{
	static void Init(GenericPropertiesSchema & schema)
	{
		InitCommon(schema);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = &OnAlign;

			vtable.OnVectoriseMonochrome = &AbstractPolygonImpl::VectoriseMonochrome<Properties>;
		});
	};
};

struct PathImpl : public AbstractPolygonImpl
{
	GLX_DECLARE_ENUM(Joins, "miter", "round", "bevel");
	GLX_DECLARE_ENUM(Caps, "butt", "round", "square");

	struct Properties : public AbstractPolygonImpl::Properties
	{
		static const bool kPath = true;
		Float32 width = 1.0f;
		bool closed = false;

		Pair <UInt8> join_cap = { kPathJoinMiter, kPathCapButt };
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		InitCommon(schema);

		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, width), "width");
		RegisterBoolProperty(schema, REFLEX_OFFSETOF(Properties, closed), "closed");
		RegisterEnumProperty(schema, REFLEX_OFFSETOF(Properties, join_cap.a), "join", &kJoins, kPropertyGroupCustom0);
		RegisterEnumProperty(schema, REFLEX_OFFSETOF(Properties, join_cap.b), "cap", &kCaps, kPropertyGroupCustom0);

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive | Layer::kOptimisationFlagVector;

			vtable.OnAlign = &OnAlign;

			vtable.OnVectoriseMonochrome = &AbstractPolygonImpl::VectoriseMonochrome<Properties>;
		});
	};
};

struct BarGraphImpl
{
	struct Properties : public StandardPropertiesWithColour
	{
		RangeProperty::ValueType range = kNormalRange;

		ConstReference <Data::ArrayOfFloat32Property> values;

		Float gap = 0.0f;
	};

	struct Scratch : public StandardScratch
	{
		bool invert;
		Float nrcp;
	};

	static void Init(GenericPropertiesSchema & schema)
	{
		RegisterIndentAndColour(schema);

		RegisterRangeProperty(schema, REFLEX_OFFSETOF(Properties, range), "range", kPropertyGroupNone, kBindStageRealign);
		RegisterFloat32Property(schema, REFLEX_OFFSETOF(Properties, gap), "gap");
		RegisterFloatArrayProperty(schema, REFLEX_OFFSETOF(Properties,values), "values", kPropertyGroupNone, kBindStageAccommodate);

		typedef XAxis Axis;
		typedef Axis::Ortho Ortho;

		SetLayerInitFn(schema, [](GenericLayer & self, const void * pproperties, void * pscratch, GenericLayer::VTable & vtable)
		{
			RemoveConst(self.flags) = Layer::kOptimisationFlagNotResponsive /*| Layer::kOptimisationFlagVector*/;

			vtable.OnAccommodate = [](const GenericLayer & layer, GenericLayer::ObjectState & data, Size & contentsize)
			{
				auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				scratch->invert = True(data.owner->GetLayoutFlags() & kFlowInvert);	//move to OnBind

				auto & values = properties->values->value;

				Float n = Float(values.GetSize());

				scratch->nrcp = 1.0f / Reflex::Max(1.0f, n);

				//if (properties->autofit)
				//{
				//	Axis::SetSize(contentsize, Reflex::Max(((properties->stride + properties->gap) * n) - properties->gap, Axis::GetSize(properties->size)));

				//	Ortho::SetSize(contentsize, Ortho::GetSize(properties->size));

				//	contentsize += Sum(properties->indent);
				//}
			};

			vtable.OnAlign = BindStandardAlign(self);
	
			//vtable.OnVectoriseMonochrome = [](const GenericLayer & self, const AbstractObjectData & data, const Size & pixelsize, Colour & colour, Points & points)
			//{
			//	auto [properties,scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

			//	colour = properties->colour;

			//	auto & values = properties->values->value;

			//	auto gap = properties->gap;

			//	Float inc = Reflex::RoundDown((Axis::GetSize(scratch->inner.size) + gap) * scratch->nrcp);

			//	auto max = Ortho::GetSize(scratch->inner.size);

			//	Rect rect = { scratch->inner.point, Axis::MakeSize(inc - gap) };

			//	auto range = properties->range;

			//	if (scratch->invert)
			//	{
			//		Ortho::IncPoint(rect.point, max);

			//		max *= -1.0f;
			//	}

			//	for (auto & i : values)
			//	{
			//		auto h = Reflex::Normalise(i, range.a, range.b) * max;

			//		Ortho::SetSize(rect.size, h);

			//		AddRectFill(points, rect);

			//		Axis::IncPoint(rect.point, inc);
			//	}
			//};

			vtable.OnRedraw = [](const GenericLayer & self, GenericLayer::ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				struct DynamicGraphic : public Graphic
				{
					DynamicGraphic(const Properties & properties, const Rect & bar_rect, Float inc, Float max)
						: values(properties.values),
						range(properties.range),
						bar_rect(bar_rect),
						inc(inc), max(max),
						colour(properties.colour)
					{
					}

					virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override
					{
						auto rect = bar_rect;

						auto mult = 1.0f / range.length;

						for (auto & i : values->value)
						{
							auto h = ((i - range.start) * mult) * max;

							Ortho::SetSize(rect.size, h);

							Detail::DrawFill(transform, DynamicGraphic::colour, rect);

							Axis::IncPoint(rect.origin, inc);
						}
					}

					ConstReference <Data::ArrayOfFloat32Property> values;

					const RangeProperty::ValueType range;

					const Rect bar_rect;

					const Float inc, max;

					const Colour colour;
				};

				auto [properties, scratch] = data.GetPropertiesAndScratch<Properties,Scratch>();

				if (properties->range.length)
				{
					auto gap = properties->gap;

					Float inc = Reflex::RoundDown((Axis::GetSize(scratch->inner.size) + gap) * scratch->nrcp);

					auto max = Ortho::GetSize(scratch->inner.size);

					Rect rect = { scratch->inner.origin, Axis::MakeSize(inc - gap) };

					if (scratch->invert)
					{
						Ortho::IncPoint(rect.origin, max);

						max *= -1.0f;
					}

					return REFLEX_CREATE(DynamicGraphic, properties, rect, inc, max);
				}
				else
				{
					return {};
				}
			};
		});
	};
};

REFLEX_END_INTERNAL

const Reflex::GLX::Detail::Layer::Class Reflex::GLX::Detail::g_vector_layers[10]
{
	{ "Fill", &GenericLayer::CreateSchema<FillImpl>, &GenericLayer::Create<FillImpl> },
	{ "Border", &GenericLayer::CreateSchema<BorderImpl>, &GenericLayer::Create<BorderImpl> },

	{ "CircleFill", &GenericLayer::CreateSchema<CircleFillImpl>, &GenericLayer::Create<CircleFillImpl> },
	{ "Circle", &GenericLayer::CreateSchema<CircleImpl>, &GenericLayer::Create<CircleImpl> },

	{ "TriangleFill", &GenericLayer::CreateSchema<TriangleFillImpl>, &GenericLayer::Create<TriangleFillImpl> },
	{ "Triangle", &GenericLayer::CreateSchema<TriangleImpl>, &GenericLayer::Create<TriangleImpl> },

	{ "Line", &GenericLayer::CreateSchema<LineImpl>, &GenericLayer::Create<LineImpl> },

	{ "Polygon", &GenericLayer::CreateSchema<PolygonImpl>, &GenericLayer::Create<PolygonImpl> },
	{ "Path", &GenericLayer::CreateSchema<PathImpl>, &GenericLayer::Create<PathImpl> },

	{ "BarGraph", &GenericLayer::CreateSchema<BarGraphImpl>, &GenericLayer::Create<BarGraphImpl> },
};
