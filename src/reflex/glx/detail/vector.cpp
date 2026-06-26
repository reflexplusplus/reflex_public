#include "reflex/glx/detail/vector.h"
#include "reflex/glx/detail/functions/geometry.h"




//
//

REFLEX_NS(Reflex::GLX::Detail)

REFLEX_DECLARE_KEY32(corner);

constexpr CString::View kAuto = "auto";

struct LayerProperties
{
	enum Flags
	{
		kIndent,
		kFloatX,
		kFloatY,
		kDynamic,
		kCustom0,
	};

	LayerProperties()
		: position(kAlignmentTopLeft)
	{
	}

	Flags8 flags;

	Alignment position;

	Size size;

	Margin indent;
};

bool ParseIndent(const Data::PropertySet & parameters, LayerProperties & properties)
{
	bool set = false;

	if (auto value = Data::GetFloat32Array(parameters, kindent))
	{
		properties.flags.Set(LayerProperties::kIndent);

		properties.indent = ToMargin(value);

		set = true;
	}

	if (auto value = Data::GetFloat32Array(parameters, koffset))
	{
		Point offset = Reinterpret<Point>(ToSize(value));

		properties.indent.near.w += offset.x;

		properties.indent.near.h += offset.y;

		properties.indent.far.w -= offset.x;

		properties.indent.far.h -= offset.y;

		properties.flags.Set(LayerProperties::kIndent);

		return true;
	}

	return set;
}

bool ParseSize(const Data::PropertySet & parameters, LayerProperties & properties)
{
	if (auto values = Data::GetFloat32Array(parameters, ksize))
	{
		properties.flags.Set(LayerProperties::kFloatX);
		properties.flags.Set(LayerProperties::kFloatY);

		properties.size = ToSize(values);

		goto Position;
	}
	else if (auto value = Data::GetCString(parameters, ksize))
	{
		auto lines = SplitCommaDelimited(value);

		switch (lines.GetSize())
		{
		case 1:
			lines.Push(lines.GetFirst());
			break;

		case 2:
			break;

		default:
			lines.SetSize(2);
			break;
		}

		auto pwh = &properties.size.w;

		UInt8 axis = LayerProperties::kFloatX;

		REFLEX_FOREACH(line, lines)
		{
			if (line != kAuto)
			{
				*pwh = ToFloat32(TrimLeft(line));

				properties.flags.Set(axis);
			}

			pwh++;
			axis++;
		}

		goto Position;
	}

	return false;

	REFLEX_MARKER(Position);

	properties.position = ParseAlignment(Data::GetKey32(parameters, kalign), kAlignmentTopLeft);

	return true;
}

template <bool INDENT, bool FLOAT_X, bool FLOAT_Y> Rect MakeRect(const Size & size, const LayerProperties & properties)
{
	if constexpr (FLOAT_X | FLOAT_Y)
	{
		auto & content_size = properties.size;

		Rect rect = { kOrigin, size };

		if constexpr (INDENT) rect = Indent(size, properties.indent);

		if constexpr (FLOAT_X & FLOAT_Y)
		{
			rect.origin += Align(rect.size, content_size, properties.position);

			rect.size = content_size;
		}
		else if constexpr (FLOAT_X)
		{
			Size t = { content_size.w, rect.size.h };

			rect.origin += Align(rect.size, t, properties.position);

			rect.size = t;
		}
		else if constexpr (FLOAT_Y)
		{
			Size t = { rect.size.w, content_size.h };

			rect.origin += Align(rect.size, t, properties.position);

			rect.size = t;
		}

		return rect;
	}
	else if constexpr (INDENT)
	{
		return Indent(size, properties.indent);
	}

	return { kOrigin, size };
}

REFLEX_TBINDER_3P(MakeRect);

REFLEX_END




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct VectorCompiler : public Reflex::Object
{
public:

	static inline const UInt64 kVectorFileHeader = Reinterpret<UInt64>(MakeTuple(UInt32(5786695ul), K32("VectorSet")));


	//lifetime

	VectorCompiler();



	//compile

	TRef <ImageSet> Compile(const Data::PropertySet & propertysheet) const;



private:

	struct CLayer
	{
		Reference <System::Renderer::Graphic> graphic;

		Float opacity = 1.0f;
	};

	template <bool OPAQUE> struct Graphic;


	TRef <System::Renderer::Graphic> Compile(Size size, const ArrayView < ConstReference <Data::PropertySet> > & layers) const;


	static System::Renderer::Graphic & CreateBorder(const Data::PropertySet & parameters, const Size & size);

	static System::Renderer::Graphic & CreateFill(const Data::PropertySet & parameters, const Size & size);

	static System::Renderer::Graphic & CreateCircle(const Data::PropertySet & parameters, const Size & size);

	static System::Renderer::Graphic & CreateCircleFill(const Data::PropertySet & parameters, const Size & size);

	static System::Renderer::Graphic & CreateTriangleFill(const Data::PropertySet & parameters, const Size & size);


	static Rect MakeRect(const Size & size, const Data::PropertySet & parameters);

	static System::Renderer::Graphic & Normalise(const Data::PropertySet & parameters, const Size & size, Points & points);


	Sequence <Key32,decltype(&CreateBorder)> m_types;


	static inline bool st_legacy = false;

};

template <bool OPAQUE>
struct VectorCompiler::Graphic : public System::Renderer::Graphic
{
	Graphic(Array <CLayer> && clayers);

	virtual void Render(const System::Renderer::Transform & transform, const Colour & colour) const override;

	Array <CLayer> m_layers;
};

VectorCompiler::VectorCompiler()
{
	m_types.Insert(K32("Border"), &CreateBorder);

	m_types.Insert(K32("Fill"), &CreateFill);

	m_types.Insert(K32("Circle"), &CreateCircle);

	m_types.Insert(K32("CircleFill"), &CreateCircleFill);

	m_types.Insert(K32("TriangleFill"), &CreateTriangleFill);

	m_types.Insert(K32("Path"), [](const Data::PropertySet & parameters, const Size & size) -> System::Renderer::Graphic &
	{
		auto points = Data::GetFloat32Array(parameters, K32("points"));

		points.size /= 2;
		points.size *= 2;

		GLX_GET_POINT_WORKSPACE(buffer);

		auto rect = MakeRect(size, parameters);

		GLX::AddPath(buffer, Data::Unpack<ArrayView<Point>>(Data::Pack(points)), false, Data::GetFloat32(parameters, K32("width"), 2.0f));

		auto region = ToRegion(buffer);

		Rescale(region, Reinterpret<Scale>(kNormal));

		Translate(region, rect.origin);

		return Normalise(parameters, size, buffer);
	});
}

REFLEX_INLINE TRef <System::Renderer::Graphic> VectorCompiler::Compile(Size size, const ArrayView < ConstReference <Data::PropertySet> > & layers) const
{
	Array <CLayer> clayers;

	clayers.Allocate(layers.size);

	bool colours = false;

	for (auto & i : layers)
	{
		auto & layer = *i;

		auto type = Data::GetKey32(layer, K32("type"));

		if (auto pctr = m_types.SearchValue(type))
		{
			LayerProperties p;

			ParseIndent(layer, p);

			ParseSize(layer, p);

			auto opacity = GLX::Detail::GetNumber(layer, kopacity, 1.0f);

			if (opacity == 0.0f)
			{
				continue;
			}
			else if (opacity < 1.0f)
			{
				colours = true;
			}

			clayers.Push<kAllocateNone>({ (*pctr)(layer, size), opacity});
		}
	}

	if (colours)
	{
		return REFLEX_CREATE(Graphic<true>, std::move(clayers));
	}
	else
	{
		return REFLEX_CREATE(Graphic<false>, std::move(clayers));
	}
}

template <bool OPAQUE> REFLEX_INLINE VectorCompiler::Graphic<OPAQUE>::Graphic(Array <CLayer> && layers)
	: m_layers(std::move(layers))
{
}

template <bool OPAQUE> void VectorCompiler::Graphic<OPAQUE>::Render(const System::Renderer::Transform & transform, const Colour & colour) const
{
	if constexpr (OPAQUE)
	{
		Colour t = colour;

		for (auto & i : m_layers)
		{
			t.a = colour.a * i.opacity;

			i.graphic->Render(transform, t);
		}
	}
	else
	{
		for (auto & i : m_layers) i.graphic->Render(transform, colour);
	}
}

REFLEX_INLINE Rect VectorCompiler::MakeRect(const Size & size, const Data::PropertySet & parameters)
{
	LayerProperties p;

	ParseIndent(parameters, p);

	ParseSize(parameters, p);

	auto makerect = MakeRectBinder::Bind(p.flags.GetWord());

	return makerect(size, p);
}

System::Renderer::Graphic & VectorCompiler::CreateBorder(const Data::PropertySet & parameters, const Size & size)
{
	GLX_GET_POINT_WORKSPACE(points);

	Rect rect = MakeRect(size, parameters);

	auto width = ToMargin(Data::GetFloat32Array(parameters, K32("width")));

	if (auto corner = Detail::GetNumber(parameters, kcorner))
	{
		AddRoundedOutline(points, rect, width, corner);
	}
	else
	{
		AddRectOutline(points, rect, width, GLX::kNormal);
	}

	return Normalise(parameters, size, points);
}

System::Renderer::Graphic & VectorCompiler::CreateFill(const Data::PropertySet & parameters, const Size & size)
{
	GLX_GET_POINT_WORKSPACE(points);

	Rect rect = MakeRect(size, parameters);

	if (auto corner = Detail::GetNumber(parameters, kcorner))
	{
		AddRoundedFill(points, rect, corner);
	}
	else
	{
		AddRectFill(points, rect);
	}

	return Normalise(parameters, size, points);
}

System::Renderer::Graphic & VectorCompiler::CreateCircle(const Data::PropertySet & parameters, const Size & size)
{
	GLX_GET_POINT_WORKSPACE(points);

	auto width = GetNumber(parameters, K32("width"), 1.0f);

	if (auto range = Data::GetFloat32(parameters, krange))
	{
		Float start = Data::GetFloat32(parameters, K32("start"));

		if (st_legacy)
		{
			start = Modulo(start + 0.25f, 1.0f);

			//range *= 0.5f;
		}

		AddEllipseOutline(points, MakeRect(size, parameters), { width, width }, start * k2Pif, range * k2Pif);
	}
	else
	{
		AddEllipseOutline(points, MakeRect(size, parameters), { width,width });
	}

	return Normalise(parameters, size, points);
}

System::Renderer::Graphic & VectorCompiler::CreateCircleFill(const Data::PropertySet & parameters, const Size & size)
{
	GLX_GET_POINT_WORKSPACE(points);

	if (auto range = GetProperty<Data::Float32Property>(parameters, krange))
	{
		Float start = Data::GetFloat32(parameters, K32("start"));

		AddEllipseFill(points, MakeRect(size, parameters), start * k2Pif, range->value * k2Pif);
	}
	else
	{
		AddEllipseFill(points, MakeRect(size, parameters));
	}

	return Normalise(parameters, size, points);
}

System::Renderer::Graphic & VectorCompiler::CreateTriangleFill(const Data::PropertySet & parameters, const Size & size)
{
	GLX_GET_POINT_WORKSPACE(points);

	auto direction = ParseAlignment(Data::GetKey32(parameters, K32("direction")), kAlignmentTop);

	AddTriangleFill(points, MakeRect(size, parameters), direction);

	return Normalise(parameters, size, points);
}

System::Renderer::Graphic & VectorCompiler::Normalise(const Data::PropertySet & parameters, const Size & size, Points & points)
{
	if (auto rotate = GetProperty<Data::Float32Property>(parameters, K32("rotate")))
	{
		auto origin_id = ParseAlignment(Data::GetKey32(parameters, K32("rotate-origin")), kAlignmentCenter);

		Size zero;

		auto origin = Align(size, zero, origin_id);

		Rotate(points, origin, rotate->value * k2Pif);
	}

	for (auto & i : points)
	{
		i.x /= size.w;

		i.y /= size.h;
	}

	return CreateGraphic(points);
}

TRef <ImageSet> VectorCompiler::Compile(const Data::PropertySet & node) const
{
	auto rtn = REFLEX_CREATE(ImageSet, System::Renderer::Canvas::null);

	st_legacy = Data::GetBool(node, K32("legacy"));// ? 1.0f : 1.0f;

	for (auto & i : node.Iterate<Data::PropertySet>())
	{
		auto size = ToSize(Data::GetFloat32Array(i.value, ksize));

		auto layers = Data::GetPropertySetArray(i.value, K32("layers"));

		auto graphic = Compile(size, layers);

		rtn->AddFrame(i.key.id, graphic, size);
	}

	return rtn;
}

#define IMPORT_NAME(symbol) case K32(symbol): Data::RegisterKey(keymap, symbol); break

//TRef <Data::PropertySet> VectorCompiler::ConvertLegacy(const Data::Archive::View & archive) const
//{
//	REFLEX_ASSERT(false);
//
//	auto rtn = New<Data::PropertySet>();
//
//	auto keymap = Data::AcquireKeyMap(rtn);
//
//	Data::RegisterKey(keymap, "size");
//	Data::RegisterKey(keymap, "layers");
//
//	Data::RegisterKey(keymap, "indent");
//	Data::RegisterKey(keymap, "align");
//	Data::RegisterKey(keymap, "colour");
//	Data::RegisterKey(keymap, "color");
//	Data::RegisterKey(keymap, "opacity");
//	Data::RegisterKey(keymap, "start");
//	Data::RegisterKey(keymap, "sweep");
//	Data::RegisterKey(keymap, "range");
//	Data::RegisterKey(keymap, "corner");
//	Data::RegisterKey(keymap, "width");
//	Data::RegisterKey(keymap, "size");
//	Data::RegisterKey(keymap, "direction");
//	Data::RegisterKey(keymap, "rotate");
//
//	Pair <UInt64, UInt32> header;
//
//	auto stream = archive;
//
//	if (stream.size > sizeof(header) && gLZO)
//	{
//		Data::Deserialize(stream, header);
//
//		if (header.a == kVectorFileHeader)
//		{
//			auto archive = Data::Decompress(*gLZO, stream);
//
//			stream = archive;
//
//			UInt64 id;
//
//			Size size;
//
//			REFLEX_LOOP(idx, Data::Deserialize<UInt32>(stream))
//			{
//				Data::Deserialize(stream, id, size);
//
//				if constexpr (REFLEX_DEBUG)
//				{
//					switch (UInt32(id))
//					{
//						IMPORT_NAME("debug");
//						IMPORT_NAME("ui");
//
//						IMPORT_NAME("browse");
//						IMPORT_NAME("plugins");
//						IMPORT_NAME("history");
//						IMPORT_NAME("control");
//						IMPORT_NAME("undo");
//						IMPORT_NAME("redo");
//						IMPORT_NAME("menu");
//						IMPORT_NAME("more");
//						IMPORT_NAME("remove");
//						IMPORT_NAME("cycle");
//						IMPORT_NAME("save");
//						IMPORT_NAME("add");
//						IMPORT_NAME("filebrowser");
//						IMPORT_NAME("filter");
//						IMPORT_NAME("back");
//						IMPORT_NAME("left");
//						IMPORT_NAME("right");
//						IMPORT_NAME("submenu");
//						IMPORT_NAME("star");
//						IMPORT_NAME("audition");
//						IMPORT_NAME("restart");
//						IMPORT_NAME("play");
//						IMPORT_NAME("sample");
//						IMPORT_NAME("folder");
//						IMPORT_NAME("file");
//						IMPORT_NAME("prev");
//						IMPORT_NAME("next");
//						IMPORT_NAME("info");
//						IMPORT_NAME("warning");
//						IMPORT_NAME("cloud");
//						IMPORT_NAME("cancel");
//						IMPORT_NAME("ok");
//						IMPORT_NAME("shop");
//						IMPORT_NAME("panic");
//						IMPORT_NAME("arrow-nw");
//						IMPORT_NAME("arrow-n");
//						IMPORT_NAME("arrow-ne");
//						IMPORT_NAME("arrow-s");
//						IMPORT_NAME("arrow-e");
//						IMPORT_NAME("arrow-w");
//						IMPORT_NAME("arrow-se");
//						IMPORT_NAME("arrow-sw");
//						IMPORT_NAME("fullscreen");
//						IMPORT_NAME("track");
//					}
//				}
//
//				auto node = Data::Detail::AcquireProperty<Data::PropertySet>(rtn, UInt32(id));
//
//				node.SetProperty(K32("size"), ArrayView<Float>(&size.w, 2));
//
//				auto output = Data::Detail::AcquireProperty<Data::ArrayOfPropertySet>(node, K32("layers"));
//
//				REFLEX_LOOP(x, Data::Deserialize<UInt32>(stream))
//				{
//					auto p = output->value.Push(REFLEX_CREATE(Data::PropertySet));
//
//					//Sequence <UInt64, LegacyCString> legacy;
//
//					//Data::Deserialize(stream, legacy);
//
//					REFLEX_LOOP(idx, Data::Deserialize<UInt32>(stream))
//					{
//						auto ptype = UInt32(Data::Deserialize<UInt64>(stream));
//
//						auto value = Data::Unpack<CString::View>(Data::ReadBytes(stream, Data::Deserialize<UInt32>(stream) + 1));
//
//						value.size--;
//
//						switch (ptype)
//						{
//						case K32("type"):
//						case K32("align"):
//						case K32("direction"):
//							if (Key32(value) == K32("centre"))
//							{
//								value = "center";
//							}
//							p.SetProperty(ptype, Data::RegisterKey(keymap, value));
//							break;
//
//						case K32("sweep"):
//							ptype = UInt32(krange);
//
//						case K32("start"):
//						case K32("range"):
//						case K32("rotate"):
//						case K32("opacity"):
//							p.SetProperty(ptype, ToFloat32(value));
//							break;
//
//						case K32("size"):
//							if (Search(value, "auto"))
//							{
//								p.SetProperty(ptype, value);
//
//								break;
//							}
//
//						case K32("indent"):
//						case K32("margin"):
//						case K32("width"):
//						case K32("corner"):
//						{
//							auto lines = SplitCommaDelimited(value);
//
//							Array <Float> values;
//
//							REFLEX_FOREACH(v, lines) values.Push(ToFloat32(v));
//
//							p.SetProperty(ptype, values);
//						}
//						break;
//
//						case K32("colour"):
//						case K32("color"):
//							REFLEX_ASSERT(false);
//							//not supported anymore
//							break;
//
//						default:
//							p.SetProperty(ptype, value);
//							break;
//						}
//					}
//				}
//			}
//		}
//	}
//
//	return rtn;
//}

REFLEX_END_INTERNAL

Reflex::ConstTRef <Reflex::GLX::Detail::ImageSet> Reflex::GLX::Detail::CreateLegacyVectorSet(const Data::PropertySet & properties)
{
	if (auto path = GetPathProperty(properties))
	{
		auto imageset = RetrieveRelativeResource<ImageSet>(path, Data::PropertySet::null, [](const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream) -> TRef <Reflex::Object>
		{
			auto & compilerref = g_library->cache.vectorcompiler;

			if (!compilerref) compilerref = REFLEX_CREATE(VectorCompiler);

			auto compiler = Cast<VectorCompiler>(compilerref);

			auto bytes = File::ReadBytes(instream);

			Data::PropertySet propertysheet;

			Data::kPropertySheetFormat->Decode(propertysheet, bytes);

			return compiler->Compile(propertysheet);
		});

		return imageset;
	}
	else
	{
		return ImageSet::null;
	}
}
