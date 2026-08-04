#include "propertyset.h"




#define MAKE_ARRAY_INFO(TYPE) { REFLEX_STRINGIFY(TYPE), &ArrayInfo::Create<TYPE>, REFLEX_TYPEID(ObjectOf<Array<TYPE>>), UInt16(REFLEX_OFFSETOF(ObjectOf<Array<TYPE>>,value)), UInt16(kSizeOf<TYPE>) }

REFLEX_BEGIN_INTERNAL(Reflex::VM)

struct ArrayInfo
{
	template <class TYPE> static Object * Create(UInt size)
	{
		return REFLEX_CREATE(ObjectOf<Array<TYPE>>, size);
	}

	const char * tname;
	FunctionPointer <Object*(UInt)> ctr;
	TypeID type_id;
	UInt16 offset;
	UInt16 stride;
};

struct FormatRef
{
	FormatRef(const Data::Format & format = Data::kPropertySheetFormat)
		: format(format)
	{
	}

	const Data::Format & format;
};

void GetCStringPropertyImpl(Context & context)
{
	VM_POP(Data::PropertySet&,Key32,String&);

	if (auto pstring = args.a.QueryProperty<Data::CStringProperty>(args.b))
	{
		VM_RTN(String::Create(ToView(*pstring)));
	}
	else
	{
		VM_RTN(args.c);
	}
}

void GetWStringPropertyImpl(Context & context)
{
	VM_POP(Data::PropertySet&, Key32, String&);

	if (auto pstring = args.a.QueryProperty<Data::WStringProperty>(args.b))
	{
		VM_RTN(String::Create(pstring->value));
	}
	else
	{
		VM_RTN(args.c);
	}
}

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gDataFormat("Data > Format", { gCoreMap, gDataPropertySet, gDataBinaryObject }, kMaxUInt8, [](Compiler::State & cstate, UInt8 context, Object &)
{
	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto uint8_t = bindings->uint8_t;

	auto key32_t = bindings->key32_t;

	auto archive_t = bindings->archive_t;

	auto string_t = bindings->string_t;

	auto propertyset_t = GetType<Data::PropertySet>(bindings);

	auto format_t = RegisterValue<FormatRef>(bindings, kDataNamespace, "Format");


	//instantiate for format

	auto keymap_t = RegisterObject<Data::KeyMap>(bindings, kDataNamespace, "KeyMap");

	auto array_propertyset_t = cstate.InstantiateTemplateType(kArray, { propertyset_t });



	//binary object

	constexpr auto ResourceHandler = [](const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream)-> TRef <Object>
	{
		auto format = ToPointer<Data::Format>(Data::GetUInt64(ctx.options, K32("format")));

		auto propertyset = New<ReadOnlyPropertySet>(Data::DecodePropertySet(*format, File::ReadBytes(instream)));

		auto pcompilestate = Detail::CompilerImpl::StateImpl::ScopeOf::GetCurrent();

		Detail::AddSource(pcompilestate->m_target, New<String>(ctx.path), REFLEX_TYPEID(Data::PropertySet), propertyset);

		return propertyset;
	};

	constexpr auto PackFormat = [](const Data::Format & format)
	{
		auto options = New<Data::PropertySet>();

		Data::SetUInt64(options, K32("format"), ToUIntNative(&format));

		return options;
	};

	cstate.RegisterResourceType(K32("xml"), propertyset_t, ResourceHandler, L"xml", PackFormat(Data::kReflexXmlFormat));

	cstate.RegisterResourceType(K32("propertysheet"), propertyset_t, ResourceHandler, L"rxps", PackFormat(Data::kPropertySheetFormat));

	cstate.RegisterResourceType(K32("json"), propertyset_t, ResourceHandler, L"json", PackFormat(Data::kJsonFormat));

	VM_BIND_CONST(Data::kJsonFormatOptionInt64, cstate, uint8_t, kDataNamespace, "kJsonFormatOptionInt64");
	VM_BIND_CONST(Data::kJsonFormatOptionFloat64, cstate, uint8_t, kDataNamespace, "kJsonFormatOptionFloat64");
	VM_BIND_CONST(Data::kJsonFormatOptionWString, cstate, uint8_t, kDataNamespace, "kJsonFormatOptionWString");



	//formats

	auto archiveobject_t = GetType<Data::ArchiveObject>(bindings);

	AddFunction(bindings, kDataNamespace, "EncodePropertySet", archiveobject_t, { format_t, propertyset_t }, [](Context & context)
	{
		VM_POP(Data::Format&, Data::PropertySet&);

		Data::Archive packed = Data::EncodePropertySet(args.a, args.b);

		VM_RTN(REFLEX_CREATE(Data::ArchiveObject, std::move(packed)));
	});

	static constexpr auto DecodePropertySet = [](Context & context)
	{
		VM_POP(Data::Format&,Data::ArchiveObject&,UInt8);

		auto out = CreatePropertySet(context);

		Reflex::Detail::SilentReference <Object> retain(out);

		args.a.Decode(out, args.b.value, args.c);

		VM_RTN(out);
	};

	AddFunction(bindings, kDataNamespace, "DecodePropertySet", propertyset_t, { format_t, archiveobject_t, uint8_t }, DecodePropertySet);

	AddFunction(bindings, kDataNamespace, "DecodePropertySet", propertyset_t, { format_t, archiveobject_t }, [](Context & context)
	{
		context.stack.Push(0);
		
		DecodePropertySet(context);
	});

	AddFunction(bindings, kDataNamespace, "GetKeyMap", keymap_t, { propertyset_t }, [](Context & context)
	{
		VM_POP1(Data::PropertySet&);

		auto rtn = Data::Detail::AcquireProperty<Data::KeyMap>(arg, Data::kkeymap);

		VM_RTN(rtn);
	});

	//AddFunction(bindings, kDataNamespace, "SetKeyMap", void_t, { propertyset_t, keymap_t }, [](Context & context)
	//{
	//	VM_POP1(Data::PropertySet&);

	//	auto rtn = Data::Detail::AcquireProperty<Data::KeyMap>(arg, Data::kkeymap);

	//	VM_RTN(rtn);
	//});

	AddFunction(bindings, kDataNamespace, "RegisterKey", key32_t, { keymap_t, string_t }, [](Context & context)
	{
		VM_POP(Data::KeyMap&, String&);

		auto wstring = args.b.GetView();

		auto key32 = Reflex::Detail::MakeHash<UInt32>(wstring);

		args.a.value.Set(key32, ToCString(wstring));

		VM_RTN(key32);
	});

	AddFunction(bindings, kDataNamespace, "GetKey", string_t, { keymap_t, key32_t }, [](Context & context)
	{
		VM_POP(Data::KeyMap&, Key32);

		if (auto pstring = args.a.value.Search(args.b))
		{
			VM_RTN(String::Create(ToView(*pstring)));
		}
		else
		{
			VM_RTN(Null<String>());
		}
	});

	AddFunction(bindings, kDataNamespace, "SetBool", void_t, { propertyset_t, key32_t, bool_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32, UInt8);

		Data::SetBool(args.a, args.b, True(args.c));
	});

	AddFunction(bindings, kDataNamespace, "GetBool", bool_t, { propertyset_t, key32_t, bool_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32, UInt8);

		VM_RTN(Data::GetBool(args.a, args.b, True(args.c)));
	});

	AddFunction(bindings, kDataNamespace, "SetCString", void_t, { propertyset_t, key32_t, string_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32, String&);

		args.a.SetProperty(MakeAddress<Data::CStringProperty>(args.b), New<Data::CStringProperty>(ToCString(args.c.GetView())));
	});

	AddFunction(bindings, kDataNamespace, "GetCString", string_t, { propertyset_t, key32_t }, [](Context & context)
	{
		VM::Detail::Push(context.stack, &REFLEX_NULL(String));

		GetCStringPropertyImpl(context);
	});

	AddFunction(bindings, kDataNamespace, "GetCString", string_t, { propertyset_t, key32_t, string_t }, &GetCStringPropertyImpl);

	AddFunction(bindings, kDataNamespace, "SetWString", void_t, { propertyset_t, key32_t, string_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32, String&);

		args.a.SetProperty(MakeAddress<Data::WStringProperty>(args.b), New<Data::WStringProperty>(args.c.GetView()));
	});

	AddFunction(bindings, kDataNamespace, "GetWString", string_t, { propertyset_t, key32_t }, [](Context & context)
	{
		VM::Detail::Push(context.stack, &REFLEX_NULL(String));

		GetWStringPropertyImpl(context);
	});

	AddFunction(bindings, kDataNamespace, "GetWString", string_t, { propertyset_t, key32_t, string_t }, &GetWStringPropertyImpl);

	AddFunction(bindings, kDataNamespace, "SetBinary", void_t, { propertyset_t, key32_t, archive_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32, ValueArray&);

		args.a.SetProperty(MakeAddress<Data::ArchiveObject>(args.b), New<Data::ArchiveObject>(args.c.GetView<UInt8>()));
	});

	AddFunction(bindings, kDataNamespace, "GetBinary", archive_t, { propertyset_t, key32_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32);

		auto rtn = CreateByteArray(Data::GetBinary(args.a, args.b));

		VM_RTN(rtn.Adr());
	});

	typedef decltype(&ArrayInfo::Create<UInt8>) ArrayCtr;

	const ArrayInfo kArray32Types[] =
	{
		MAKE_ARRAY_INFO(Int32),
		MAKE_ARRAY_INFO(Float32),
		MAKE_ARRAY_INFO(Key32),
	};

	for (auto & i : kArray32Types)
	{
		auto name = ToView(i.tname);

		auto array_t = VM::GetTypeBySymbol(bindings, { kGlobal, Join("Array@", name) });

		Data::Archive function_data = Data::Pack(MakeTuple(i.ctr, array_t, i.type_id, i.offset, i.stride));

		AddFunction(bindings, kDataNamespace, Join("Set", name, "Array"), void_t, { propertyset_t, key32_t, array_t }, { }, function_data, [](Context & context)
		{
			auto [ctr, array_t, type_id, offset, stride] = ReadFunctionData<Tuple<ArrayCtr, TypeRef, TypeID, UInt16, UInt16>>(context);

			VM_POP(Data::PropertySet&, Key32, ValueArray&);

			auto src = args.c.GetRawRegion();

			auto native = ctr(src.size / stride);

			auto dest = ToRegion(*Reinterpret<Data::Archive>(Reinterpret<UInt8>(native) + offset));

			MemCopy(src.data, dest.data, src.size);

			args.a.SetProperty({ args.b, type_id }, *native);
		});

		AddFunction(bindings, kDataNamespace, Join("Get", name, "Array"), array_t, { propertyset_t, key32_t }, { }, function_data, [](Context & context)
		{
			auto [ctr, array_t, type_id, offset, stride] = ReadFunctionData<Tuple<ArrayCtr, TypeRef, TypeID, UInt16, UInt16>>(context);

			VM_POP(Data::PropertySet&, Key32);

			auto out = Cast<ValueArray>(Detail::CreateObject(context, array_t));

			if (auto native = args.a.QueryProperty({ args.b, type_id }))
			{
				auto src = ToView(*Reinterpret<Data::Archive>(Reinterpret<UInt8>(native) + offset));

				auto dest = out->Extend(src.size);

				MemCopy(src.data, dest, src.size * stride);
			}

			VM_RTN(out.Adr());
		});
	}

	auto array_string_t = cstate.InstantiateTemplateType(kArray, { string_t });

	AddFunction(bindings, kDataNamespace, "SetCStringArray", void_t, { propertyset_t, key32_t, array_string_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32, ObjectArray&);

		auto src = args.c.GetView<String>();

		auto native = REFLEX_CREATE(Data::ArrayOfCStringProperty, src.size);

		auto dest = native->value.GetData();

		for (auto & i : src)
		{
			*dest++ = ToCString(i->GetView());
		}

		args.a.SetProperty(MakeAddress<Data::ArrayOfCStringProperty>(args.b), *native);
	});

	AddFunction(bindings, kDataNamespace, "GetCStringArray", array_string_t, { propertyset_t, key32_t }, {}, Data::Pack(array_string_t), [](Context & context)
	{
		auto array_string_t = ReadFunctionData<TypeRef>(context);

		VM_POP(Data::PropertySet&, Key32);

		auto out = Cast<ObjectArray>(Detail::CreateObject(context, array_string_t));

		if (auto native = ToView(GetProperty<Data::ArrayOfCStringProperty>(args.a, args.b)->value))
		{
			out->Extend(context, native.size);

			auto dest = out->GetRegion<String>().data;

			for (auto & i : native)
			{
				*dest++ = String::Create(i);
			}
		}

		VM_RTN(out);
	});

	AddFunction(bindings, kDataNamespace, "SetPropertySetArray", void_t, { propertyset_t, key32_t, array_propertyset_t }, [](Context & context)
	{
		VM_POP(Data::PropertySet&, Key32, ObjectArray&);

		auto src = args.c.GetView<Data::PropertySet>();

		auto native = REFLEX_CREATE(Data::PropertySetArray, src.size);

		auto dest = native->value.GetData();

		for (auto & i : src)
		{
			*dest++ = i;
		}

		args.a.SetProperty(MakeAddress<Data::PropertySetArray>(args.b), *native);
	});

	AddFunction(bindings, kDataNamespace, "GetPropertySetArray", array_propertyset_t, { propertyset_t, key32_t }, {}, Data::Pack(array_propertyset_t), [](Context & context)
	{
		auto array_t = ReadFunctionData<TypeRef>(context);

		VM_POP(Data::PropertySet&,Key32);

		auto native = Data::GetPropertySetArray(args.a, args.b);

		VM_RTN(ImportPropertySets(context, native, array_t));
	});


	VM_BIND_CONST(FormatRef(Data::kBinaryFormat), cstate, format_t, kDataNamespace, "kBinaryFormat");
	VM_BIND_CONST(FormatRef(Data::kPropertySetFormat), cstate, format_t, kDataNamespace, "kPropertySetFormat");
	VM_BIND_CONST(FormatRef(Data::kJsonFormat), cstate, format_t, kDataNamespace, "kJsonFormat");

	VM_BIND_CONST(FormatRef(Data::kRiffFormat), cstate, format_t, kDataNamespace, "kRiffFormat");

	VM_BIND_CONST(FormatRef(Data::kPropertySheetFormat), cstate, format_t, kDataNamespace, "kPropertySheetFormat");
	VM_BIND_CONST(FormatRef(Data::kReflexXmlFormat), cstate, format_t, kDataNamespace, "kReflexXmlFormat");
	VM_BIND_CONST(FormatRef(Data::kReflexMarkupFormat), cstate, format_t, kDataNamespace, "kReflexMarkupFormat");
});
