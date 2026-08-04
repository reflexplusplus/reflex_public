
REFLEX_BEGIN_INTERNAL(Reflex::GLXVM)

//const SIMD::FloatV4 f_rcp_two = 0.5;
//const SIMD::FloatV4 kf255 = 255.0f;

//REFLEX_INLINE SIMD::FloatV4 Interpolate4P_RT(const SIMD::FloatV4 & x, const SIMD::FloatV4 & t_1, const SIMD::FloatV4 & t0, const SIMD::FloatV4 & t1, const SIMD::FloatV4 & t2)
//{
//	SIMD::AssertAlignment(&f_rcp_two);
//	SIMD::AssertAlignment(&x);
//	SIMD::AssertAlignment(&t_1);
//	SIMD::AssertAlignment(&t0);
//	SIMD::AssertAlignment(&t1);
//	SIMD::AssertAlignment(&t2);
//	REFLEX_USE(SIMD);
//
//	FloatV4 a = (t1 - t_1) * f_rcp_two;
//	FloatV4 b = (t2 - t0) * f_rcp_two;
//	FloatV4 c = t0 - t1;
//	FloatV4 d = a + c;
//	FloatV4 e = b + d;
//	FloatV4 f = e + c;
//	FloatV4 g = x * f;
//	FloatV4 h = d + f;
//	FloatV4 i = g - h;
//	FloatV4 j = x * i;
//	FloatV4 k = a + j;
//	FloatV4 l = x * k;
//
//	return t0 + l;
//}

//typedef Quad <UInt8> RGBA8;
//
//REFLEX_INLINE SIMD::FloatV4 Unpack(const RGBA8 & rgba)
//{
//	SIMD::IntV4 t(rgba.a, rgba.b, rgba.c, rgba.d);
//
//	return SIMD::IntToFloat(t) / kf255;
//}
//
//REFLEX_INLINE RGBA8 Pack(const SIMD::FloatV4 & rgba)
//{
//	auto t = Truncate(rgba * kf255);
//
//	return { UInt8(t[0]),UInt8(t[1]),UInt8(t[2]),UInt8(t[3]) };
//}

//REFLEX_INLINE SIMD::FloatV4 GetPixel(const RGBA8 * rgba, System::iSize size, System::iPoint point, RGBA8 bgcolour)
//{
//	if (point.x >= 0 && point.x < size.w && point.y >= 0 && point.y < size.h)
//	{
//		return Unpack(rgba[(point.y * size.w) + point.x]);
//	}
//	else
//	{
//		return Unpack(bgcolour);
//	}
//}

//REFLEX_INLINE SIMD::FloatV4 BicubicInterpolation(const RGBA8 * rgba, System::iSize size, System::fPoint point, RGBA8 bgcolour)
//{
//	auto x0 = Truncate(RoundDown(point.x));
//
//	auto x_1 = x0 - 1;
//
//	auto x1 = x0 + 1;
//
//	auto x2 = x0 + 2;
//
//
//	auto y0 = Truncate(RoundDown(point.y));
//
//	auto y_1 = y0 - 1;
//
//	auto y1 = y0 + 1;
//
//	auto y2 = y0 + 2;
//
//
//	auto x = point.x - Float(x0);
//
//	auto t_1 = Interpolate4P_RT(x, GetPixel(rgba, size, { x_1, y_1 }, bgcolour), GetPixel(rgba, size, { x0, y_1 }, bgcolour), GetPixel(rgba, size, { x1, y_1 }, bgcolour), GetPixel(rgba, size, { x2, y_1 }, bgcolour));
//
//	auto t0 = Interpolate4P_RT(x, GetPixel(rgba, size, { x_1, y0 }, bgcolour), GetPixel(rgba, size, { x0, y0 }, bgcolour), GetPixel(rgba, size, { x1, y0 }, bgcolour), GetPixel(rgba, size, { x2, y0 }, bgcolour));
//
//	auto t2 = Interpolate4P_RT(x, GetPixel(rgba, size, { x_1, y1 }, bgcolour), GetPixel(rgba, size, { x0, y1 }, bgcolour), GetPixel(rgba, size, { x1, y1 }, bgcolour), GetPixel(rgba, size, { x2, y1 }, bgcolour));
//
//	auto t3 = Interpolate4P_RT(x, GetPixel(rgba, size, { x_1, y2 }, bgcolour), GetPixel(rgba, size, { x0, y2 }, bgcolour), GetPixel(rgba, size, { x1, y2 }, bgcolour), GetPixel(rgba, size, { x2, y2 }, bgcolour));
//
//	return Clip(Interpolate4P_RT(point.y - Float(y0), t_1, t0, t2, t3), SIMD::kfZero, SIMD::kfOne);
//}

const Pair <Key32, decltype(&GLX::Detail::EncodePNG)> kEncodeBitmapFns[] = { { K32("png"), &GLX::Detail::EncodePNG }, {K32("jpg"), &GLX::Detail::EncodeJPG}, {K32("bmp"), &GLX::Detail::EncodeBMP}, {kGLX, &GLX::Detail::EncodeGLX} };

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::GLXVM::gBitmap("GLX > Bitmap", { VM::gDataFormat, gGeometry }, VM::kContextFlagMain | VM::kContextFlagMainBg | VM::kContextFlagUi | VM::kContextFlagUiBg, [](VM::Compiler::State & cstate, UInt8 context, Reflex::Object &)
{
	static constexpr auto ReturnBitmap = [](VM::Context & context, System::RawBitmap && bitmapinfo)
	{
		auto [type, adrdata] = Splice(VM::GetFunctionData(context), sizeof(VM::TypeRef));

		auto tuple_t = Data::Unpack<VM::TypeRef>(type);

		auto tuple = VM::Detail::CreateObject(context, tuple_t);

		auto adrs = Data::Unpack<Pair<UInt16>>(adrdata);

		VM::GetMemberAtAdr<System::BitmapInfo>(tuple, adrs.a) = bitmapinfo.a;

		VM::SetMemberObjectAtAdr(tuple, adrs.b, REFLEX_CREATE(Data::ArchiveObject, std::move(bitmapinfo.b)));

		VM_RTN(tuple);
	};
		
	cstate.RegisterStaticString(kGLX, "GLX");

	auto bindings = cstate.bindings;

	auto uint8_t = bindings->uint8_t;

	auto int32_t = bindings->int32_t;

	auto key32_t = bindings->key32_t;

	auto bitmapinfo_t = VM::RegisterValue<System::BitmapInfo>(bindings, kGLX, "BitmapInfo");

	auto members = Extend(bitmapinfo_t->members, 4);

	members[0] = { VM::AcquireStaticString(cstate, "w"), VM::MakeMember(int32_t, 0, false) };
	members[1] = { VM::AcquireStaticString(cstate, "h"), VM::MakeMember(int32_t,4,false) };
	members[2] = { VM::AcquireStaticString(cstate, "density"), VM::MakeMember(int32_t,8,false) };
	members[3] = { VM::AcquireStaticString(cstate, "format"), VM::MakeMember(uint8_t,12,false) };

	auto archiveobject_t = VM::GetType<Data::ArchiveObject>(bindings);

	auto bitmapinfo_byref = VM::ByRef(bitmapinfo_t);

	auto rawbitmap_t = cstate.InstantiateTemplateType(VM::kTuple, { bitmapinfo_t, archiveobject_t });

	Data::Archive data = Join(Data::Pack(rawbitmap_t), VM::PackTupleAdrs(rawbitmap_t));

	VM_BIND_CONST(System::kImageFormatRGBA, cstate, uint8_t, kGLX, "kImageFormatRGBA");
	VM_BIND_CONST(System::kImageFormatRGB, cstate, uint8_t, kGLX, "kImageFormatRGB");

	//TODO needs to be seperate module

	//if (context & VM::kContextFlagUi)
	//{
	//	cstate.Instantiate(gGLX);

	//	auto fsize_t = VM::GetType<GLX::Size>(bindings);

	//	auto rect_t = VM::GetType<GLX::Rect>(bindings);

	//	auto bitmap_t = VM::RegisterObject<System::Renderer::Canvas>(bindings, kGLX, "Bitmap");

	//	cstate.RegisterResourceType(K32("bitmap"), bitmap_t, GLX::Detail::kOpenBitmap);

	//	VM::AddConstructor(bindings, bitmap_t, { string_t, int32_t, bool_t }, [](VM::Context & context)
	//	{
	//		VM_POP(VM::String&,UInt32,bool);

	//		VM_RTN(GLX::Detail::RetrieveBitmap(args.a.GetView(), args.b & 7, args.c));
	//	});

	//	VM::AddConstructor(bindings, bitmap_t, { bitmapinfo_byref, archiveobject_t, bool_t }, [](VM::Context & context)
	//	{
	//		VM_POP(System::BitmapInfo&,Data::ArchiveObject&,bool);

	//		VM_RTN(GLX::Detail::OpenBitmap(args.a, args.b.value, args.c));
	//	});

	//	VM::AddMethod(bindings, "GetSize", fsize_t, { bitmap_t }, [](VM::Context & context)
	//	{
	//		VM_POP1(System::Renderer::Canvas&);

	//		auto isize = arg.GetSize();

	//		VM_RTN(MakeTuple(Float32(isize.w), Float32(isize.h)));
	//	});

	//	auto graphic_t = VM::GetType<System::Renderer::Graphic>(bindings);


	//	auto pair_rect_t = cstate.InstantiateTemplateType(VM::kTuple, { rect_t, rect_t });

	//	auto array_pair_rect_t = cstate.InstantiateTemplateType(VM::kArray, { pair_rect_t });

	//	VM::AddMethod(bindings, "CreateTextures", graphic_t, { bitmap_t, array_pair_rect_t }, [](VM::Context & context)
	//	{
	//		VM_POP(System::Renderer::Canvas&,VM::ValueArray&);

	//		auto rects = args.b.GetView<Pair<System::fRect>>();

	//		VM_RTN(args.a.CreateTextures(rects));
	//	});


	//	auto image_t = VM::RegisterObject<GLX::Detail::Image>(bindings, kGLX, "Image");

	//	VM::AddConstructor(bindings, image_t, { graphic_t, fsize_t }, [](VM::Context & context)
	//	{
	//		VM_POP(System::Renderer::Graphic&,GLX::Size);

	//		VM_RTN(REFLEX_CREATE(GLX::Detail::Image, args.a, args.b));
	//	});

	//	image_t->members = { VM_BIND_MEMBER(GLX::Detail::Image,graphic,cstate,graphic_t), VM_BIND_MEMBER(GLX::Detail::Image,size,cstate,fsize_t) };


	//	VM::AddFunction(bindings, kGLX, "CropBitmap", rawbitmap_t, { bitmapinfo_byref, archiveobject_t, VM::ByRef(rect_t) }, {}, data, [](VM::Context & context)
	//	{
	//		VM_POP(System::BitmapInfo&, Data::ArchiveObject&, GLX::Rect&);

	//		auto frect = SIMD::LoadUnaligned(&args.c.origin.x);

	//		auto irect = Reinterpret<System::iRect>(SIMD::Truncate(frect));

	//		ReturnBitmap(context, GLX::Detail::CropBitmap(args.a, args.b.value, irect));
	//	});
	//}

	VM::AddFunction(bindings, kGLX, "DecodeBitmap", rawbitmap_t, { archiveobject_t, int32_t }, {}, data, [](VM::Context & context)
	{
		VM_POP(Data::ArchiveObject&, Int32);

		ReturnBitmap(context, GLX::Detail::DecodeBitmap(args.a.value, args.b));
	});

	VM::AddFunction(bindings, kGLX, "HalveBitmap", rawbitmap_t, { bitmapinfo_byref, archiveobject_t }, {}, data, [](VM::Context & context)
	{
		VM_POP(System::BitmapInfo&, Data::ArchiveObject&);

		ReturnBitmap(context, GLX::Detail::HalveBitmap(args.a, args.b.value));
	});

	VM::AddFunction(bindings, kGLX, "EncodeBitmap", archiveobject_t, { bitmapinfo_byref, archiveobject_t, key32_t, uint8_t }, [](VM::Context & context)
	{
		VM_POP(System::BitmapInfo&, Data::ArchiveObject&, Key32, UInt8);

		auto encoder = SearchValue<KeyCompare>(ToView(kEncodeBitmapFns), args.c, kEncodeBitmapFns + 0);

		auto temp = encoder->b(args.a, args.b.value, args.d);

		VM_RTN(REFLEX_CREATE(Data::ArchiveObject, std::move(temp)));
	});

	//VM::AddFunction(bindings, kGLX, "RotateBitmap", archiveobject_t, { bitmapinfo_byref, archiveobject_t, bindings.float32_t, VM::ByRef(colour_t) }, [](VM::Context & context)
	//{
	//	VM_POP(System::BitmapInfo&,Data::ArchiveObject&,Float32,GLX::Colour&);

	//	auto & bitmapinfo = args.a;

	//	bitmapinfo.size.w *= bitmapinfo.pixdensity;
	//	bitmapinfo.size.h *= bitmapinfo.pixdensity;
	//	bitmapinfo.pixdensity = 1;

	//	auto src = args.b.GetView();

	//	auto radians = args.c * Reflex::k2Pif * 1.0f;

	//	auto bgcolour = Pack({ args.d.r, args.d.g, args.d.b, args.d.a });

	//	auto rtn = CreateArchiveWithSize(src.b);

	//	if (GLX::Detail::VerifyBitmap(bitmapinfo, src) && System::kBPP[bitmapinfo.format] == 4)
	//	{
	//		auto pdst = Reinterpret<RGBA8>(rtn->GetRegion().a);

	//		auto psrc = Reinterpret<RGBA8>(src.a);

	//		auto sin = Reflex::Sin(radians);
	//		auto cos = Reflex::Cos(radians);

	//		float centerX = bitmapinfo.size.w / 2.0f;
	//		float centerY = bitmapinfo.size.h / 2.0f;

	//		//auto fw = Float(bitmapinfo.size.w);
	//		//auto fh = Float(bitmapinfo.size.h);

	//		REFLEX_LOOP(y, bitmapinfo.size.h)
	//		{
	//			auto fy = Float(y);

	//			auto ysin = (fy - centerY) * sin;
	//			auto ycos = (fy - centerY) * cos;

	//			REFLEX_LOOP(x, bitmapinfo.size.w)
	//			{
	//				auto fx = Float32(x) - centerX;

	//				GLX::Point pos = { fx * cos - ysin + centerX, fx * sin + ycos + centerY };

	//				*pdst++ = Pack(BicubicInterpolation(psrc, bitmapinfo.size, pos, bgcolour));
	//			}
	//		}
	//	}

	//	VM_RTN(rtn);
	//});
});
