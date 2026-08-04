#include "global.h"




REFLEX_NS(Reflex::GLXVM)

typedef VM_TEMPLATE_TARG("Array@GLX::Point", VM::ValueArray) ArrayOfPointsTarg;

typedef VM_TEMPLATE_TARG("Array@GLX::ColourPoint", VM::ValueArray) ArrayOfColourPointsTarg;

struct iColourPoint
{
	GLX::Point point;
	GLX::Colour colour;
};

//no colour

REFLEX_NOINLINE Pair <GLX::Point> LookupBoundsImpl(const Data::Archive::Region & view, UInt stride, const GLX::Point & size)
{
	auto ptr = view.data;

	auto end = ptr + view.size;

	Pair <GLX::Point> minmax = { size, -size };

	while (ptr < end)
	{
		auto & i = *Reinterpret<GLX::Point>(ptr);

		minmax.a = Min(minmax.a, i);
		minmax.b = Max(minmax.b, i);

		ptr += stride;
	}

	minmax.b -= minmax.a;

	return minmax;
}

REFLEX_NOINLINE void ApplyBoundsImpl(const Data::Archive::Region & region, UInt stride, const GLX::Rect & rect)
{
	auto ptr = region.data;

	auto end = ptr + region.size;

	auto min = rect.origin;
	auto max = rect.origin + Reinterpret<GLX::Point>(rect.size);

	while (ptr < end)
	{
		auto & i = *Reinterpret<GLX::Point>(ptr);

		i = Min(max, i);
		i = Max(min, i);

		ptr += stride;
	}
}

REFLEX_INLINE void AddPoints(const Array <GLX::Point> & input, ArrayOfPointsTarg & data)
{
	auto n = input.GetSize();

	if (n < kMaxUInt16)
	{
		auto output = data.Extend<GLX::Point>(n);

		MemCopy(input.GetData(), output, n * sizeof(GLX::Point));
	}
}

REFLEX_INLINE void AddPoints(const Array <System::ColourPoint> & input, ArrayOfColourPointsTarg & data)
{
	auto n = input.GetSize();

	if (n < kMaxUInt16)
	{
		auto output = data.Extend<System::ColourPoint>(n);

		MemCopy(input.GetData(), output, n * sizeof(System::ColourPoint));
	}
}

REFLEX_INLINE void AddColourPoints(const Array <GLX::Point>::View & input, const GLX::Colour & colour, ArrayOfColourPointsTarg & output)
{
	if (input.size < kMaxUInt16)
	{
		auto pout = output.Extend<System::ColourPoint>(input.size);

		REFLEX_LOOP_PTR(input.data, ppoint, input.size)
		{
			(*pout++) = { *ppoint, colour };
		}
	}
}

template <class TYPE> REFLEX_INLINE void Rotate(VM::Context & context)
{
	VM_POP(VM::ValueArray&,GLX::Point,Float32);

	GLX::Rotate(args.a.GetRegion<TYPE>(), args.b, args.c);
}

REFLEX_END

const Reflex::VM::Module Reflex::GLXVM::gGeometry("GLX > Geometry", {}, kMaxUInt8, [](VM::Compiler::State & cstate, UInt8 context, Reflex::Object & object)
{
	cstate.RegisterStaticString(kGLX, "GLX");


	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto float32_t = bindings->float32_t;

	auto uint8_t = bindings->uint8_t;


	auto point_t = RegisterValue<GLX::Point>(bindings, kGLX, "Point");

	point_t->members = { VM_BIND_MEMBER(GLX::Point, x, cstate, float32_t), VM_BIND_MEMBER(GLX::Point, y, cstate, float32_t) };

	auto fsize_t = RegisterValue<GLX::Size>(bindings, kGLX, "Size");

	fsize_t->members = { VM_BIND_MEMBER(GLX::Size, w, cstate, float32_t), VM_BIND_MEMBER(GLX::Size, h, cstate, float32_t) };

	VM_BIND_CONST(GLX::kLarge, cstate, fsize_t, kGLX, "kLarge");

	auto rect_t = RegisterValue<GLX::Rect>(bindings, kGLX, "Rect");

	rect_t->members = { VM_BIND_MEMBER(GLX::Rect, origin, cstate, point_t), VM_BIND_MEMBER(GLX::Rect, size, cstate, fsize_t) };

	auto colour_t = VM::RegisterValue<GLX::Colour>(bindings, kGLX, "Colour");

	colour_t->members =
	{
		VM_BIND_MEMBER(GLX::Colour, r, cstate, float32_t),
		VM_BIND_MEMBER(GLX::Colour, g, cstate, float32_t),
		VM_BIND_MEMBER(GLX::Colour, b, cstate, float32_t),
		VM_BIND_MEMBER(GLX::Colour, a, cstate, float32_t),
	};


	auto margin_t = RegisterValue<GLX::Margin>(bindings, kGLX, "Margin");

	margin_t->members = { VM_BIND_MEMBER(GLX::Margin, near, cstate, fsize_t), VM_BIND_MEMBER(GLX::Margin, far, cstate, fsize_t) };

	
	VM::InstantiateObjectOf<GLX::Margin>(cstate);

	VM::InstantiateObjectOf<GLX::Size>(cstate);

	VM::InstantiateObjectOf<GLX::Point>(cstate);


	VM::AddFunction(bindings, kGLX, "HSV", colour_t, { float32_t, float32_t, float32_t }, [](VM::Context & context)
	{
		VM_POP(Float32, Float32, Float32);

		GLX::Colour rgba;

		auto hue6 = args.a * 6.0f;

		Int i = ToInt32(hue6);
		Float f = hue6 - i;
		Float p = args.c * (1.0f - args.b);
		Float q = args.c * (1.0f - f * args.b);
		Float t = args.c * (1.0f - (1.0f - f) * args.b);

		switch (i % 6)
		{
		case 0: rgba.r = args.c; rgba.g = t; rgba.b = p; break;
		case 1: rgba.r = q; rgba.g = args.c; rgba.b = p; break;
		case 2: rgba.r = p; rgba.g = args.c; rgba.b = t; break;
		case 3: rgba.r = p; rgba.g = q; rgba.b = args.c; break;
		case 4: rgba.r = t; rgba.g = p; rgba.b = args.c; break;
		case 5: rgba.r = args.c; rgba.g = p; rgba.b = q; break;
		}

		VM_RTN(GLX::Colour(rgba));
	});


	auto colourpoint_t = VM::RegisterValue<System::ColourPoint>(bindings, kGLX, "ColourPoint");

	colourpoint_t->members =
	{
		VM_BIND_MEMBER(iColourPoint, point, cstate, point_t),
		VM_BIND_MEMBER(iColourPoint, colour, cstate, colour_t)
	};


	auto array_point_t = cstate.InstantiateTemplateType(VM::kArray, { point_t });

	auto array_colourpoint_t = cstate.InstantiateTemplateType(VM::kArray, { colourpoint_t });


	//auto transform_t = VM::RegisterValue<System::Renderer::Transform>(bindings, glx, "Transform");

	//transform_t->members =
	//{
	//	VM_BIND_MEMBER(System::Renderer::Transform, origin, cstate, point_t),
	//	VM_BIND_MEMBER(System::Renderer::Transform, scale, cstate, fsize_t),
	//	VM_BIND_MEMBER(System::Renderer::Transform, opacity, cstate, float32_t)
	//};

	//auto array_graphic_t = cstate.InstantiateTemplateType(VM::kArray, { graphic_t });

	//auto pair_transform_graphic_t = cstate.InstantiateTemplateType(VM::kTuple, { transform_t, graphic_t });

	//auto array_pair_transform_graphic_t = cstate.InstantiateTemplateType(VM::kArray, { pair_transform_graphic_t });

	//AddConstructor(bindings, graphic_t, { array_pair_transform_graphic_t }, VM::PackTupleAdrs(pair_transform_graphic_t), [](Context)
	//{
	//	auto addresses = Data::Unpack<Pair<UInt16>>(fn.clientdata);

	//	auto rtn = New<MultiGraphic>();

	//	auto & p0 = Pop<VM::ArrayOfNonCircularObjects&>(context.stack);

	//	auto view = p0.GetConstView();

	//	rtn->m_elements.Allocate(view.b);

	//	for (auto & i : view)
	//	{
	//		auto & object = *i;

	//		rtn->m_elements.Push<kAllocateNone>({ VM::GetMemberAtAdr<System::Renderer::Transform>(object, addresses.a), VM::GetMemberAtAdr<System::Renderer::Graphic>(object, addresses.b) });
	//	}

	//	VM_RTN(rtn);
	//});


	auto rect_ref = VM::ByRef(rect_t);

	auto margin_ref = VM::ByRef(margin_t);

	VM::AddFunction(bindings, kGLX, "Indent", rect_t, { rect_ref, margin_ref }, [](VM::Context & context)
	{
		VM_POP(GLX::Rect&, GLX::Margin&);

		auto inner = GLX::Indent(args.a.size, args.b);

		inner.origin += args.a.origin;

		VM_RTN(inner);
	});

	VM::AddFunction(bindings, kGLX, "Align", rect_t, { rect_ref, fsize_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(GLX::Rect&, GLX::Size, UInt8);

		GLX::Rect inner = { GLX::Detail::Align(args.a.size, args.b, GLX::Alignment(args.c % GLX::kNumAlignment)), args.b };

		inner.origin += args.a.origin;

		VM_RTN(inner);
	});



	//no colour

	VM::AddFunction(bindings, kGLX, "AddRect", void_t, { array_point_t, margin_ref, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Margin&, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRectOutline(points, args.c, args.b, GLX::kNormal);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddRect", void_t, { array_point_t, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRectFill(points, args.b);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddRoundedRect", void_t, { array_point_t, margin_ref, rect_ref, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Margin&, GLX::Rect&, Float);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRoundedOutline(points, args.c, args.b, args.d);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddRoundedRect", void_t, { array_point_t, rect_ref, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Rect&, Float);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRoundedFill(points, args.b, args.c);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddEllipse", void_t, { array_point_t, float32_t, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, Float32, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseOutline(points, args.c, { args.b, args.b });

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddEllipse", void_t, { array_point_t, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseFill(points, args.b);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddPie", void_t, { array_point_t, float32_t, rect_ref, float32_t, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, Float32, GLX::Rect&, Float32, Float32);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseOutline(points, args.c, { args.b, args.b }, args.d * k2Pif, args.e * k2Pif);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddPie", void_t, { array_point_t, rect_ref, float32_t, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Rect&, Float32, Float32);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseFill(points, args.b, args.c * k2Pif, args.d * k2Pif);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddPolygon", void_t, { array_point_t, array_point_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, ArrayOfPointsTarg&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddPolygonFill(points, args.b.GetView<GLX::Point>());

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddPath", void_t, { array_point_t, array_point_t, bool_t, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, ArrayOfPointsTarg&, bool, Float32);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddPath(points, args.b.GetView<GLX::Point>(), args.c, args.d, GLX::kNormal);

		AddPoints(points, args.a);
	});

	VM::AddFunction(bindings, kGLX, "LookupBounds", rect_t, { array_point_t, fsize_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Point);

		auto minmax = LookupBoundsImpl(args.a.GetRawRegion(), sizeof(GLX::Point), args.b);

		VM_RTN(minmax);
	});

	VM::AddFunction(bindings, kGLX, "ApplyBounds", void_t, { array_point_t, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfPointsTarg&, GLX::Rect&);

		ApplyBoundsImpl(args.a.GetRawRegion(), sizeof(GLX::Point), args.b);
	});

	VM::AddFunction(bindings, kGLX, "LookupBounds", rect_t, { array_colourpoint_t, fsize_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Point);

		auto minmax = LookupBoundsImpl(args.a.GetRawRegion(), sizeof(System::ColourPoint), args.b);

		VM_RTN(minmax);
	});

	VM::AddFunction(bindings, kGLX, "ApplyBounds", void_t, { array_colourpoint_t, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Rect&);

		ApplyBoundsImpl(args.a.GetRawRegion(), sizeof(System::ColourPoint), args.b);
	});


	//colour

	auto colour_ref = VM::ByRef(colour_t);

	VM::AddFunction(bindings, kGLX, "AddRect", void_t, { array_colourpoint_t, colour_ref, float32_t, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, Float32, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRectOutline(points, args.d, GLX::MakeMargin(args.c), GLX::kNormal);

		AddColourPoints(points, args.b, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddRect", void_t, { array_colourpoint_t, colour_ref, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRectFill(points, args.c);

		AddColourPoints(points, args.b, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddRoundedRect", void_t, { array_colourpoint_t, colour_ref, margin_ref, rect_ref, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, GLX::Margin&, GLX::Rect&, Float32);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRoundedOutline(points, args.d, args.c, args.e);

		AddColourPoints(points, args.b, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddRoundedRect", void_t, { array_colourpoint_t, colour_ref, rect_ref, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, GLX::Rect&, Float32);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddRoundedFill(points, args.c, args.d);

		AddColourPoints(points, args.b, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddEllipse", void_t, { array_colourpoint_t, colour_ref, float32_t, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, Float32, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseOutline(points, args.d, { args.c, args.c });

		AddColourPoints(points, args.b, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddEllipse", void_t, { array_colourpoint_t, colour_ref, rect_ref }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, GLX::Rect&);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseFill(points, args.c);

		AddColourPoints(points, args.b, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddPie", void_t, { array_colourpoint_t, colour_ref, float32_t, rect_ref, float32_t, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, Float32, GLX::Rect&, Float32, Float32);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseOutline(points, args.d, { args.c, args.c }, args.e * k2Pif, args.f * k2Pif);

		AddColourPoints(points, args.b, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddPie", void_t, { array_colourpoint_t, colour_ref, rect_ref, float32_t, float32_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, GLX::Rect&, Float32, Float32);

		GLX_GET_POINT_WORKSPACE(points);

		GLX::AddEllipseFill(points, args.c, args.d * k2Pif, args.e * k2Pif);

		AddColourPoints(points, args.b, args.a);
	});

	//AddFunction(bindings, glx, "AddPath", void_t, { array_colourpoint_t, VM::ByRef(colour_t), float32_t, bool_t, array_point_t }, [](VM::Context & context)
	//{
	//	GLX_GET_POINT_WORKSPACE(points);

	//	VM_POP(ArrayOfColourPointsTarg&,GLX::Colour&,Float32,bool,ArrayOfPointsTarg&);

	//	GLX::Detail::AddPath(points, args.e, args.d, args.c);

	//	AddColourPoints(points, args.b, args.a);
	//});

	//AddFunction(bindings, glx, "AddPath", void_t, { array_colourpoint_t, float32_t, bool_t, array_point_t }, [](VM::Context & context)
	//{
	//	GLX_GET_COLOURPOINT_WORKSPACE(points);

	//	VM_POP(ArrayOfColourPointsTarg&,Float32,bool,ArrayOfColourPointsTarg&);

	//	GLX::Detail::AddPath(points, args.d, args.c, args.b);

	//	auto pout = args.a.Extend(points.GetSize());

	//	MemCopy(points.GetData(), pout, points.GetSize() * sizeof(System::ColourPoint));
	//});

	VM::AddFunction(bindings, kGLX, "AddPolygon", void_t, { array_colourpoint_t, array_point_t, colour_ref }, [](VM::Context & context)
	{
		GLX_GET_POINT_WORKSPACE(points);

		VM_POP(ArrayOfColourPointsTarg&, ArrayOfPointsTarg&, GLX::Colour&);

		GLX::AddPolygonFill(points, args.b.GetView<GLX::Point>());

		AddColourPoints(points, args.c, args.a);
	});

	VM::AddFunction(bindings, kGLX, "AddColour", void_t, { array_colourpoint_t, colour_ref, array_point_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, ArrayOfPointsTarg&);

		AddColourPoints(args.c.GetView<GLX::Point>(), args.b, args.a);
	});

	//VM::AddFunction(bindings, kGLX, "AddGradient", void_t, { array_colourpoint_t, colour_ref, colour_ref, rect_ref, bool_t }, [](VM::Context & context)
	//{
	//	VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, GLX::Colour&, GLX::Rect&, UInt8);

	//	GLX_GET_COLOURPOINT_WORKSPACE(t);

	//	GLX::AddGradientFill(t, args.b, args.c, args.d, args.e);

	//	MemCopy(t.GetData(), args.a.Extend<System::ColourPoint>(6), sizeof(System::ColourPoint) * 6);
	//});

	VM::AddFunction(bindings, kGLX, "ApplyGradient", void_t, { array_colourpoint_t, colour_ref, colour_ref, rect_ref, bool_t }, [](VM::Context & context)
	{
		VM_POP(ArrayOfColourPointsTarg&, GLX::Colour&, GLX::Colour&, GLX::Rect&, UInt8);

		auto y = args.e & 1;

		auto origin = (&args.d.origin.x)[y];

		Float mult = Reciprocal(Max((&args.d.size.w)[y], 1.0f));

		auto inout = args.a.GetRegion<System::ColourPoint>();

		Float * pf = &inout.data->a.x + y;

		REFLEX_LOOP_PTR(inout.data, pcp, inout.size)
		{
			auto & cp = *pcp;

			auto mix = Clip((*pf - origin) * mult, 0.0f, 1.0f);

			pf += 6;

			cp.b = LinearInterpolate(mix, args.b, args.c);
		}
	});

	VM::AddFunction(bindings, kGLX, "Rotate", void_t, { array_point_t, point_t, float32_t }, &Rotate<GLX::Point>);

	//VM::AddFunction(bindings, kGLX, "ApplyRotate", void_t, { array_colourpoint_t, point_t, float32_t }, &Rotate<System::ColourPoint>);


	VM::AddFunction(bindings, kGLX, "Intersects", bool_t, { rect_ref, rect_ref }, [](VM::Context & context)
	{
		VM_POP(GLX::Rect&, GLX::Rect&);

		VM_RTN(GLX::Intersects(args.a, args.b));
	});
});

const Reflex::VM::Module Reflex::GLXVM::gGraphic("GLX > Graphic", { VM::gDataPropertySet, gGeometry }, kMaxUInt8, [](VM::Compiler::State & cstate, UInt8 context, Reflex::Object & object)
{
	auto bindings = cstate.bindings;

	auto uint8_t = bindings->uint8_t;

	auto point_t = VM::GetType<GLX::Point>(bindings);

	auto dynamic_t = VM::GetType<Data::PropertySet>(bindings);

	auto colourpoint_t = VM::GetType<System::ColourPoint>(bindings);

	auto array_point_t = cstate.InstantiateTemplateType(VM::kArray, { point_t });

	auto array_colourpoint_t = cstate.InstantiateTemplateType(VM::kArray, { colourpoint_t });

	VM::BindConstant(cstate, kGLX, "kPrimitiveTypeTriangles", System::Renderer::kPrimitiveTypeTriangles);
	VM::BindConstant(cstate, kGLX, "kPrimitiveTypeTriangleStrip", System::Renderer::kPrimitiveTypeTriangleStrip);
	VM::BindConstant(cstate, kGLX, "kPrimitiveTypeLines", System::Renderer::kPrimitiveTypeLines);
	VM::BindConstant(cstate, kGLX, "kPrimitiveTypeLineStrip", System::Renderer::kPrimitiveTypeLineStrip);

	auto graphic_t = VM::RegisterObject<System::Renderer::Graphic>(bindings, kGLX, "Graphic");

	VM::AddConstructor(bindings, graphic_t, { uint8_t, array_point_t }, [](VM::Context & context)
	{
		VM_POP(UInt8, ArrayOfPointsTarg&);

		VM_RTN(GLX::Core::g_renderer->CreatePrimitives(System::Renderer::PrimitiveType(args.a & 3), args.b.GetView<System::fPoint>()));
	});

	VM::AddConstructor(bindings, graphic_t, { uint8_t, array_colourpoint_t }, [](VM::Context & context)
	{
		VM_POP(UInt8, ArrayOfColourPointsTarg&);

		VM_RTN(GLX::Core::g_renderer->CreatePrimitives(System::Renderer::PrimitiveType(args.a & 3), args.b.GetView<System::ColourPoint>()));
	});


	auto font_t = VM::RegisterObject<GLX::Detail::Font>(bindings, kGLX, "Font");

	//Key32 font_ns = VM::AcquireStaticString(compilestate, "GLX::Font");

	//auto facedesc_t = VM::RegisterObject<FaceDesc>(bindings, font_ns, "FaceDesc");

	//facedesc_t->members =
	//{
	//	VM_BIND_MEMBER(FaceDesc, path, compilestate, string_t),
	//	VM_BIND_MEMBER(FaceDesc, size, compilestate, fsize_t),
	//	VM_BIND_MEMBER(FaceDesc, spacing, compilestate, float32_t),
	//	VM_BIND_MEMBER(FaceDesc, mode, compilestate, uint8_t),
	//	VM_BIND_MEMBER(FaceDesc, antialias, compilestate, bool_t),
	//};

	//auto array_facedesc_t = compilestate.InstantiateTemplateType(VM::kArray, { facedesc_t });

	VM::AddConstructor(bindings, font_t, { dynamic_t }, [](VM::Context & context)
	{
		//TODO convert from VM supported types

		auto params = VM::Detail::Pop<Data::PropertySet&>(context.stack);

		//Array <GLX::Detail::Font::FaceDesc> t;

		//t.Allocate(params.b);

		//for (auto & i : params)
		//{
		//	auto & to = t.Push<kAllocateNone>();

		//	auto & from = Cast<FaceDesc>(*i);

		//	to.fontfile = GLX::Detail::RetrieveFontFile(from.path->value);
		//	to.size = from.size;
		//	to.spacing = from.spacing;
		//	to.mode = GLX::Detail::Font::RenderMode(from.mode % GLX::Detail::Font::kNumRenderMode);
		//	to.aa = from.antialias;
		//}

		VM_RTN(GLX::Detail::Font::Create(params));
	});

	auto pair_float32_t = VM::GetType<Pair<Float32>>(bindings);

	VM::AddMethod(bindings, "GetHeightAndTail", pair_float32_t, { font_t }, [](VM::Context & context)
	{
		auto & font = VM::Detail::Pop<GLX::Detail::Font&>(context.stack);

		VM_RTN(font.GetHeightAndTail());
	});

	auto pair_graphic_float_t = cstate.InstantiateTemplateType(VM::kTuple, { graphic_t, bindings->float32_t });

	VM::AddMethod(bindings, "CreateText", pair_graphic_float_t, { font_t, bindings->string_t }, {}, Join(VM::PackTupleAdrs(pair_graphic_float_t), Data::Pack(pair_graphic_float_t)), [](VM::Context & context)
	{
		VM_POP(GLX::Detail::Font&, VM::String&);

		auto data = Splice(VM::GetFunctionData(context), 2);

		auto text = args.a.CreateText(args.b.GetView());

		auto tuple = VM::Detail::CreateObject(context, Data::Unpack<VM::TypeRef>(data.b));

		auto adrs = Data::Unpack<Pair<UInt16>>(data.a);

		VM::SetMemberObjectAtAdr(tuple, adrs.a, text.a);

		VM::SetMemberValueAtAdr(tuple, adrs.b, text.b);

		VM_RTN(tuple);
	});

});