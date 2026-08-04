#include "layer.h"
#include "layers/layerproperties.h"
#include "../../../../../include/reflex/glx/functions/properties.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct ComparePropertyItem
{
	static bool lt(const Pair <Address, const GenericPropertiesSchema::Item*> & a, UInt stage)
	{
		return a.b->bindable < stage;
	}
};

struct NullLayer : public Layer
{
	NullLayer()
		: Layer(kOptimisationFlagNotResponsive)
	{
		OnAccommodate = [](const Layer &, Object & state, Size & contentsize)
		{
		};

		OnAlign = [](const Layer &, Object & state, Size size, Float & contenth)
		{
		};

		OnRedraw = [](const Layer &, Object & state, Size pixelsize, UInt8 flags)
		{
			return Null<System::Renderer::Graphic>();
		};
	}
}

g_null_layer;

Layer::Class g_null_layer_class
(
	"Null",
	[](Key32)
	{
		return Null<Reflex::Object>();
	},
	[](const Style &, const Reflex::Object & schema, const Data::PropertySet & properties)
	{
		return Null<Layer>();
	},
	true
);

typedef Sequence <Key32, CString::View> Parameters;

#if REFLEX_DEBUG

const Pair <UInt64> kWhite128 = MakeTuple(Reinterpret<UInt64>(kWhite), Reinterpret<UInt64>(kWhite));

CString::View ParseLegacyFunction(const CString::View & string, Parameters & parameters)
{
	struct TrimBrackets
	{
		static CString::View Call(CString::View ref)
		{
			if (ref.size > 1)
			{
				ref.size -= 2;

				ref.data++;
			}

			return ref;
		}
	};

	if (auto pos = Search(string, '('))
	{
		auto splice = Splice(string, pos.value);

		CString::View params = TrimBrackets::Call(splice.b);

		UInt searchpos = 0;

		while (searchpos < params.size)
		{
			if (auto assign = Search(Mid(params, searchpos), ':'))
			{
				assign.value += searchpos;

				auto keystring = Trim(Mid<false>(params, searchpos, (assign.value) - searchpos));

				Key32 key = keystring;

				if (keystring[0] == '$')
				{
					keystring = Splice(keystring, 1).b;

					if (keystring.size == 8)
					{
						key = Data::Unpack<Key32>(Data::HexToBytes(keystring));
					}
				}

				searchpos = (assign.value) + 1;

				if (auto idx = Search(Mid(params, searchpos), ';'))
				{
					idx.value += searchpos;

					UInt end = idx.value;

					auto value = Trim(Mid<false>(params, searchpos, end - searchpos));

					parameters.Insert(key, value);

					searchpos = end + 1;

					continue;
				}
				else
				{
					auto value = Trim(Splice(params, searchpos).b);

					parameters.Insert(key, value);

					searchpos = params.size;

					continue;
				}
			}

			break;
		}

		return TrimLeft(splice.a);
	}

	return {};
}

ConstTRef <Data::KeyMap> GetKeyMap(const Style & style)
{
	ConstTRef <Data::KeyMap> keymap;

	for (auto & i : GLX::Style::ConstParentRange(style))
	{
		if (auto proot = DynamicCast<GLX::StyleSheet>(i))
		{
			keymap = Data::GetKeyMap(*proot);
		}
	}

	return keymap;
}

void Debug_ReportIssues(const GenericPropertiesSchema & schema, const Style & style, const Data::PropertySet & params)
{
#if	(REFLEX_DEBUG)
	Key32 sheet_path;

	for (auto & i : params.Iterate())
	{
		for (auto & valid : schema.defs)
		{
			if (i.key == valid.address)
			{
				goto Next;
			}
			else if (i.key == MakeAddress<ReferenceProperty>(i.key.id))
			{
				if (valid.bindable) goto Next;
			}
		}

		//ConstReference <StyleSheet> sheet;

		if (auto itr = style.GetParent())
		{
			while (auto parent = itr->GetParent())
			{
				itr = parent;
			}

			if (auto psheet = DynamicCast<StyleSheet>(*itr))
			{
				//sheet = psheet;

				File::ResourcePool::Lock lock(Core::desktop->resourcepool);

				if (auto token = lock.Query(MakeAddress<StyleSheet>(psheet->path)))
				{
					sheet_path = token->path;
				}
			}
		}

		if (auto id = GetKey(i.key.id))
		{
			if (id[0] != '_')
			{
				output.Error(GetKey(sheet_path), GetKey(schema.uid), "unsupported property:", id);

				//Data::SetError(sheet.RemoveConst(),)
			}
		}

		REFLEX_MARKER(Next);
	}
#endif
}

Data::Key32Property & RemapAlign(Data::Key32Property & attribute)
{
	switch (attribute.value.value)
	{
	case K32("top-left"):
		attribute.value = K32("top_left");
		break;

	case K32("top-right"):
		attribute.value = K32("top_right");
		break;

	case K32("bottom-left"):
		attribute.value = K32("bottom_left");
		break;

	case K32("bottom-right"):
		attribute.value = K32("bottom_right");
		break;
	}

	return attribute;
}

void RemapAlign(LayerDesc & layer, UInt32 to)
{
	if (auto attribute = layer.QueryProperty<Data::Key32Property>(K32("align")))
	{
		layer.SetProperty(to, RemapAlign(*attribute));

		layer.UnsetProperty<Data::Key32Property>(K32("align"));
	}
}

#endif

const Layer::Factory & gLayerFactory = Reflex::The<Library>::Get<true>()->m_layerfactory;

UInt32 gta = 0;

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Detail::ArrayOfLayerDesc> Reflex::GLX::Detail::ConvertLegacyLayers(const Data::ArrayOfCStringProperty & strings)
{
	auto layers = New<ArrayOfLayerDesc>();

#if	REFLEX_DEBUG
	TRef format = g_library->kStyleSheetFormat;

	auto style = Make<Style>(kNullKey);

	Parameters params;

	for (auto & legacy : strings.value)
	{
		auto type = ParseLegacyFunction(legacy, params);

		auto layerid = MakeKey32(type);

		const Layer::Class * null = &g_null_layer_class;

		const Layer::Class * cls = *gLayerFactory.SearchValue(layerid, &null);

		if (cls)
		{
			auto idstring = Data::BytesToHex(Data::Pack(gta++));

			Data::Archive a = Data::Pack(Join(idstring, ':', legacy, ';'));

			//REFLEX_ASSERT(st_current_style == &style);

			Key32 id = idstring;

			format->Reset(style);

			format->Decode(style, a);

			if (auto layer = style->QueryProperty<LayerDesc>(id))
			{
				layers->value.Push(layer);

				if (auto offset_values = Data::GetFloat32Array(*layer, K32("offset")))
				{
					auto offset = Reinterpret<Point>(ToSize(offset_values));

					if (auto indent_values = Data::GetFloat32Array(*layer, kindent))
					{
						auto indent = ToMargin(indent_values);

						indent.near.w += offset.x;

						indent.near.h += offset.y;

						indent.far.w -= offset.x;

						indent.far.h -= offset.y;

						layer->SetProperty(kindent, New<MarginProperty>(indent));

						Data::UnsetFloat32Array(*layer, K32("offset"));
						Data::UnsetFloat32Array(*layer, kindent);
					}
				}

				switch (UInt32(layerid))
				{
				case K32("Text"):
				case K32("TextEdit"):
					RemapAlign(*layer, K32("justify"));
					if (auto value = layer->QueryProperty<Data::Key32Property>(K32("value")))
					{
						Data::SetKey32(*layer, K32("value"), Data::GetKey(GetKeyMap(style), value->value));
					}
					else if (!layer->QueryProperty<ReferenceProperty>(K32("value")))
					{
						layer->SetProperty(K32("value"), New<ReferenceProperty>(K32("value")));
					}
					if (auto remap = layer->QueryProperty<Data::Key32Property>(K32("uppercase")))
					{
						Data::SetKey32(*layer, K32("transform"), K32("uppercase"));
					}

					if (auto remap = layer->QueryProperty<ReferenceProperty>(kdata))
					{
						layer->SetProperty(kvalue, remap);
					}
					break;

				case K32("Image"):
					RemapAlign(*layer, K32("anchor"));
					if (auto bitmap = layer->QueryProperty<Data::Key32Property>(K32("bitmap")))
					{
						if (auto frame = layer->QueryProperty<Data::Key32Property>(K32("frame")))
						{
							layer->SetProperty(K32("bitmap"), REFLEX_CREATE(Data::ArrayOfKey32Property, ArrayView<Key32>({ bitmap->value, frame->value })));

							Data::UnsetKey32(*layer, K32("bitmap"));

							Data::UnsetKey32(*layer, K32("frame"));
						}
					}
					break;

				case K32("Line"):
				{
					auto axis = Data::GetKey32(*layer, K32("axis"));

					auto position = Data::Detail::AcquireProperty<Data::Key32Property>(*layer, K32("position"));

					auto orientation = ParseOrientation(position->value);

					if (axis == K32("y"))
					{
						switch (orientation)
						{
						case kOrientationNear:
							position->value = K32("left");
							break;

						case kOrientationFar:
							position->value = K32("right");
							break;
						}
					}
					else
					{
						switch (orientation)
						{
						case kOrientationNear:
							position->value = K32("top");
							break;

						case kOrientationFar:
							position->value = K32("bottom");
							break;
						}
					}
				}
				break;

				case K32("Gradient"):
					if (auto axis = layer->QueryProperty<Data::Key32Property>(K32("axis")))
					{
						const Pair <UInt32> remap[2] = { MakeTuple(K32("from"), K32("start_colour")), MakeTuple(K32("to"), K32("end_colour")) };

						for (auto & i : remap)
						{
							SetColour(*layer, i.b, ToColour(Data::GetFloat32Array(*layer, i.a)));
						}

						if (axis->value == kXY[1])
						{
							Data::SetFloat32(*layer, K32("angle"), 0.25f);
						}
					}
					break;

				default:
					break;
				}

				output.Error("Importing", Join('"', legacy, '"'));
			}
		}
		else
		{
			output.Error("unknown legacy layer", legacy);
		}
	}
#endif

	return layers;
}

const Reflex::GLX::Detail::Layer::Class * Reflex::GLX::Detail::Layer::Class::Query(Key32 layerid, const Class * fallback)
{
	return *gLayerFactory.SearchValue(layerid, &fallback);
}

Reflex::GLX::Detail::ComplexObjectData::ComplexObjectData(GLX::Object & object, const GenericPropertiesSchema & schema, UInt16 propertyflags, const void * staticproperties)
	: ObjectState(object, propertyflags)
{
	RemoveConst(reserved) = reinterpret_cast<void*>(schema.state_ctr_dtr.b);

	schema.state_ctr_dtr.a(state, staticproperties);
}

Reflex::GLX::Detail::ComplexObjectData::~ComplexObjectData()
{
	reinterpret_cast< FunctionPointer <void(void*)> >(reserved)(state);
}

void Reflex::GLX::Detail::GenericLayer::PrepareBindingLevel(UInt level, UInt8 bindstage, UInt8 bindstage_end)
{
	auto from = Reflex::Detail::SearchSortedGTE<ComparePropertyItem>(ToView(m_bindings), bindstage);

	auto to = Reflex::Detail::SearchSortedLT<ComparePropertyItem>(ToView(m_bindings), bindstage_end) + 1;
	
	auto & view = m_binding_stages[level];

	REFLEX_ASSERT(to - from);
	
	view = { m_bindings.GetData() + from, UInt32(to - from) };
}

Reflex::GLX::Detail::GenericLayer::GenericLayer(const Style & style, const Reflex::Object & schema_)
	: LayerImpl(kOptimisationFlagNotResponsive)
	, schema(Cast<GenericPropertiesSchema>(schema_))
	, stylesheet_flags(style.stylesheet_flags)
	, propertyflags(0)
{
	SetOnHeap(g_default_allocator);

	auto copyctr = schema->state_ctr_dtr.a;

	copyctr(RemoveConst(m_static_state), schema->default_state);
}

Reflex::GLX::Detail::GenericLayer::GenericLayer(const Style & style, const Reflex::Object & schema_, const Data::PropertySet & params)
	: LayerImpl(kOptimisationFlagNotResponsive)
	, schema(Cast<GenericPropertiesSchema>(schema_))
	, stylesheet_flags(style.stylesheet_flags)
	, propertyflags(0)
{
	SetOnHeap(g_default_allocator);

	auto copyctr = schema->state_ctr_dtr.a;

	copyctr(RemoveConst(m_static_state), schema->default_state);

	auto properties = Reinterpret<UInt8>(RemoveConst(m_static_state));

	UInt16 & propertyflags = RemoveConst(GenericLayer::propertyflags);

	for (auto & i : params.Iterate())
	{
		auto adr = i.key;

		for (auto & def : schema->defs)
		{
			if (adr == def.address)
			{
				def.apply(i.value, properties + def.offset, def.data, stylesheet_flags);

				propertyflags |= def.flags;

				break;
			}
		}
	}

	UInt8 bindingflags = 0;

	for (auto & ref : params.Iterate<ReferenceProperty>())
	{
		auto property_id = ref.key.id;
		auto binding_id = ref.value->id;

		for (auto & i : schema->bindable)
		{
			auto address = i->address;

			if (property_id == address.id)
			{
				propertyflags |= i->flags;

				auto stage = i->bindable;

				bindingflags |= stage;

				decltype(m_bindings)::Type item = { { binding_id, address.type_id }, i };

				if (Idx result = Reflex::Detail::SearchSortedGTE<ComparePropertyItem>(ToView(m_bindings), stage, 0))
				{
					m_bindings.Insert(result.value, item);
				}
				else
				{
					m_bindings.Push(item);
				}
			}
		}
	}

#if REFLEX_DEBUG
	Debug_ReportIssues(schema, style, params);
#endif

	schema->oninit(*this, properties, RemoveConst(properties + schema->scratch_offset), &RemoveConst(vtable));

	REFLEX_ASSERT(vtable.OnRedraw || vtable.OnVectoriseColour || vtable.OnVectoriseMonochrome);

	if (!(flags & kOptimisationFlagNotResponsive))
	{
		REFLEX_ASSERT(vtable.OnAccommodate && vtable.OnAlign) 
	}

	if (bindingflags & kBindStageBuildLayout)
	{
		PrepareBindingLevel(kBindStageBuildLayout >> 1, kBindStageBuildLayout, kBindStageBuildLayout + 1);

		Layer::OnCreateState = [](const Layer & layer, GLX::Object & object)
		{
			auto self = Cast<GenericLayer>(layer);

			auto createstatefn = CreateStateBinder::Bind(MakeBits(self->schema->complex, true, True(self->vtable.OnBind)));

			return createstatefn(self, object);
		};
	}
	else
	{
		Layer::OnCreateState = CreateStateBinder::Bind(MakeBits(schema->complex, false, True(vtable.OnBind)));
	}

	if (bindingflags & kBindStageAccommodate)
	{
		REFLEX_ASSERT(vtable.OnAccommodate);

		PrepareBindingLevel(kBindStageAccommodate >> 1, kBindStageAccommodate, kBindStageAccommodate + 1);
		
		SetOnAccommodate<ObjectState>(&GenericLayer::AccommodateDynamic);
	}
	else if (vtable.OnAccommodate)
	{
		SetOnAccommodate<ObjectState>(vtable.OnAccommodate);
	}

	if (bindingflags & (kBindStageRealign | kBindStageRedraw))
	{
		REFLEX_ASSERT(vtable.OnAlign);

		PrepareBindingLevel(kBindStageRealign >> 1, kBindStageRealign, kBindStageRealign | kBindStageRedraw + 1);

		SetOnAlign<ObjectState>(&GenericLayer::AlignDynamic);
	}
	else if (vtable.OnAlign)
	{
		SetOnAlign<ObjectState>(vtable.OnAlign);
	}

	if (flags & kOptimisationFlagVector)
	{
		REFLEX_ASSERT(vtable.OnVectoriseMonochrome || vtable.OnVectoriseColour)

		if (vtable.OnVectoriseMonochrome)
		{
			SetOnRedraw<ObjectState>([](const GenericLayer & self, ObjectState & data, Size pixelsize, UInt8 flags) -> TRef <Graphic>
			{
				GLX_GET_POINT_WORKSPACE(points);

				Colour colour;

				self.vtable.OnVectoriseMonochrome(self, data, pixelsize, colour, points);

				if (self.propertyflags & kPropertyGroupColour)
				{
					return REFLEX_CREATE(GraphicRendererWithOffsetAndColour, CreateGraphic(points), kOrigin, colour);
				}
				else
				{
					return CreateGraphic(points);
				}
			});

			vtable.OnVectoriseColour = [](const GenericLayer & self, const ObjectState & data, Size pixelsize, ColourPoints & output)
			{
				GLX_GET_POINT_WORKSPACE(points);

				Colour colour;

				self.vtable.OnVectoriseMonochrome(self, data, pixelsize, colour, points);

				AddPointsWithColour(output, points, colour);
			};
		}
		else
		{
			SetOnRedraw<ObjectState>([](const GenericLayer & self, ObjectState & data, Size pixelsize, UInt8 flags)
			{
				GLX_GET_COLOURPOINT_WORKSPACE(colour_points);

				self.vtable.OnVectoriseColour(self, data, pixelsize, colour_points);

				return CreateGraphic(colour_points);
			});
		}
	}
	else
	{
		SetOnRedraw<ObjectState>(vtable.OnRedraw);
	}
}

Reflex::GLX::Detail::GenericLayer::~GenericLayer()
{
	schema->state_ctr_dtr.b(RemoveConst(m_static_state));
}

template <bool COMPLEX, bool BINDINGS, bool CALLBACK> Reflex::TRef <Reflex::Object> Reflex::GLX::Detail::GenericLayer::CreateState(const Layer & layer, GLX::Object & object)
{
	REFLEX_STATIC_ASSERT(sizeof(ObjectState) == sizeof(TrivialObjectData));
	REFLEX_STATIC_ASSERT(sizeof(ObjectState) == sizeof(ComplexObjectData));

	auto self = Cast<GenericLayer>(layer);

	auto schema = self->schema;

	auto pstate = Cast<ObjectState>(g_default_allocator->Allocate(sizeof(ObjectState) + schema->state_size, {}));

	if constexpr (COMPLEX)	//schema->complex
	{
		Reflex::Detail::Constructor<ComplexObjectData>::Construct(pstate, object, schema, self->propertyflags, self->m_static_state);
	}
	else
	{
		Reflex::Detail::Constructor<TrivialObjectData>::Construct(pstate, object, schema, self->propertyflags, self->m_static_state);
	}
	
	if constexpr (BINDINGS)
	{
		self->RebindProperties(kBindStageBuildLayout >> 1, *pstate);
	}

	if constexpr (CALLBACK)
	{
		self->vtable.OnBind(self, *pstate, object);
	}

	return pstate;
}

void Reflex::GLX::Detail::GenericLayer::AccommodateDynamic(const GenericLayer & self, ObjectState & data, Size & contentsize)
{
	self.RebindProperties(kBindStageAccommodate >> 1, data);

	self.vtable.OnAccommodate(self, data, contentsize);
}

void Reflex::GLX::Detail::GenericLayer::AlignDynamic(const GenericLayer & self, ObjectState & data, Size size, Float & height)
{
	self.RebindProperties(kBindStageRealign >> 1, data);

	self.vtable.OnAlign(self, data, size, height);
}

REFLEX_INLINE void Reflex::GLX::Detail::GenericLayer::RebindProperties(UInt level, ObjectState & data) const
{
	UInt8 * properties = data.state;

	for (auto & i : m_binding_stages[level])
	{
		auto & def = *i.b;

		if (auto pobject = data.owner->QueryProperty(i.a))
		{
			def.apply(*pobject, Reinterpret<UInt8>(properties) + def.offset, def.data, stylesheet_flags);
		}
	}
}

Reflex::GLX::Detail::Layer & Reflex::GLX::Detail::Layer::null = Reflex::GLX::Detail::g_null_layer;
Reflex::GLX::Detail::Layer::Class & Reflex::GLX::Detail::Layer::Class::null = Reflex::GLX::Detail::g_null_layer_class;
