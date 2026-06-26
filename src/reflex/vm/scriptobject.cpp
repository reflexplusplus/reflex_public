#include "bindings/core/stream.h"

#include "container.h"
#include "programimpl.h"	//for stack frame layout




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct ScriptObjectImpl;

template <ContainerType TYPE> struct ScriptObjectImplX;

template <ContainerType TYPE> using ScriptObjectCircularType = typename Selector<TYPE == kContainerTypeCircularObjects,Circular,Dummy>::type;

struct ScriptObjectImpl : public ScriptObject
{
	template <bool RESTORE> static void StreamValues(Context & context)
	{
		auto info = ReadFunctionData<Pair<UInt16>>(context);

		VM_POP(ScriptObjectImpl&,ValueArray&);

		auto & stream = args.b;

		auto base = Reinterpret<UInt8>(&args.a);

		if constexpr (RESTORE)
		{
			RestoreBytes<false>(stream, base + info.a, info.b);
		}
		else
		{
			VM::Append(stream, { base + info.a, info.b });
		}
	}

	template <bool RESTORE> static void Stream(Context & context);
};

template <ContainerType TYPE>
struct ScriptObjectImplX :
	public ScriptObjectImpl,
	public ScriptObjectCircularType<TYPE>
{
	static TypeRef Register(Compiler::State & compilestate, const WString::View & source, Key32 ns, CString::View name, Data::Archive & params, Variables & members);


	template <bool INITALISE = true> static Object & Create(VM_CTR_PARAMS);

	static void CreateFromInitialiserList(Context & context);


	ScriptObjectImplX(VM_CTR_PARAMS);

	virtual ~ScriptObjectImplX();

	virtual void OnReleaseData() override;

	static Object * CrossContextCopyImpl(Context & tocontext, Object & object, TypeRef to, bool mt);


	typename Selector <bool(TYPE > kContainerTypeValues),ConstReference<Type>,Dummy>::type m_type;
};

template <ContainerType TYPE> inline TypeRef ScriptObjectImplX<TYPE>::Register(Compiler::State & compilestate, const WString::View & path, Key32 ns, CString::View name, Data::Archive & params, Variables & members)
{
	auto bindings = compilestate.bindings;

	auto void_t = bindings->void_t;

	auto archive_t = bindings->archive_t;

	auto object_t = AcquireObjectType(compilestate, REFLEX_OBJECT_TYPE(Object), ns, name, path);

	for (auto & i : members) i.b.address += sizeof(ScriptObjectImplX);

	bool noncircular = false;

	bool threadsafe = false;

	switch (TYPE)
	{
	case kContainerTypeNull:
	case kContainerTypeValues:
	case kContainerTypeThreadSafeObjects:
		noncircular = true;
		threadsafe = true;
		break;

	case kContainerTypeNonCircularObjects:
		noncircular = true;
		break;

	case kContainerTypeCircularObjects:
		break;
	}

	auto type = CreateObjectType(bindings, object_t, ns, name, noncircular, threadsafe);

	SetTypeCtr(type, params, &ScriptObjectImplX::Create);

	if (members)
	{
		type->members = std::move(members);

		Array <Argument> arguments;

		arguments.Allocate(type->members.GetSize());

		auto data = MakeTuple(ToUIntNative(type), UIntNative(0));

		Data::Archive storerestoredata;

		auto streamable = true;

		for (auto & i : type->members)
		{
			auto type = i.b.type;

			UInt8 size = type->size;

			UInt8 isobject = type->IsObject();

			REFLEX_STATIC_ASSERT(sizeof(decltype(MakeTuple(i.b.address, size, isobject))) == 4);

			Data::Serialize(storerestoredata, MakeTuple(i.b.address, size, isobject));

			if (isobject)
			{
				auto storerestore = MakeTuple(GetStore(compilestate, type), GetRestore(compilestate, type));

				REFLEX_STATIC_ASSERT(sizeof(storerestore) == sizeof(Pair<void*>));

				if constexpr (TYPE != kContainerTypeValues)
				{
					if (storerestore.a && storerestore.b)
					{
						Data::Serialize(storerestoredata, Reinterpret<Pair<UIntNative>>(storerestore));
					}
					else
					{
						streamable = false;
					}
				}
			}

			arguments.Push(type);

			data.b += size;
		}

		AddConstructor(bindings, type, arguments, Data::Pack(data), &ScriptObjectImplX::CreateFromInitialiserList);

		if constexpr (TYPE == kContainerTypeValues)
		{
			auto start = UInt16(sizeof(ScriptObjectImplX<TYPE>));

			Data::Archive info = Data::Pack(MakeTuple(start, UInt16(data.b)));

			AddMethod(bindings, VM::Serialize, void_t, { type, archive_t }, {}, info, &ScriptObjectImpl::StreamValues<false>);

			AddMethod(bindings, VM::Deserialize, void_t, { type, archive_t }, {}, info, &ScriptObjectImpl::StreamValues<true>);
		}
		else if (streamable)
		{
			AddMethod(bindings, VM::Serialize, void_t, { type, archive_t }, {}, storerestoredata, &ScriptObjectImpl::Stream<false>);

			AddMethod(bindings, VM::Deserialize, void_t, { type, archive_t }, {}, storerestoredata, &ScriptObjectImpl::Stream<true>);
		}
	}

	switch (TYPE)
	{
	case kContainerTypeValues:
	case kContainerTypeThreadSafeObjects:
		REFLEX_ASSERT(type->contextcopyfn[false] == kTrivialContextCopy);
		REFLEX_ASSERT(type->contextcopyfn[true] == kTrivialContextCopy);
		break;

	case kContainerTypeNonCircularObjects:
		REFLEX_ASSERT(type->contextcopyfn[false] == kTrivialContextCopy);
		type->contextcopyfn[true] = &ScriptObjectImplX::CrossContextCopyImpl;
		break;

	case kContainerTypeCircularObjects:
		type->contextcopyfn[false] = &ScriptObjectImplX::CrossContextCopyImpl;
		type->contextcopyfn[true] = &ScriptObjectImplX::CrossContextCopyImpl;
		break;

	default:
		break;
	}

	type->flags.Set(Type::kFlagScriptType);

	return type;
}

template <ContainerType TYPE> template <bool INITALISE> inline Object & ScriptObjectImplX<TYPE>::Create(VM_CTR_PARAMS)
{
	auto & layout = *Reinterpret<Layout>(type->params.GetData());

	auto mem = g_default_allocator->Allocate(sizeof(ScriptObjectImplX) + layout.size, Reflex::AllocInfo(type->name.data));

	auto self = Reflex::Detail::Constructor<ScriptObjectImplX>::Construct(mem, context, type);

	if constexpr (INITALISE)
	{
		InitialiseLayout<bool(TYPE > kContainerTypeValues),true>(Cast<ContextImpl>(context), layout, Reinterpret<UInt8>(self) + sizeof(ScriptObjectImplX<TYPE>));
	}

	return *self;
}

template <ContainerType TYPE> inline void ScriptObjectImplX<TYPE>::CreateFromInitialiserList(Context & context)
{
	auto info = ReadFunctionData< Pair <TypeRef,UIntNative> >(context);

	auto type = info.a;

	auto & layout = *Reinterpret<Layout>(type->params.GetData());

	auto object = Cast<ScriptObjectImplX>(Create<false>(context, type));

	auto base = Reinterpret<UInt8>(object.Adr()) + sizeof(ScriptObjectImplX);

	auto & stack = context.stack;

	stack.Shrink(UInt(info.b));

	auto top = stack.GetData() + stack.GetSize();

	MemCopy(top, base, info.b);


	//retain members (because we skipped InitLayout)

	if constexpr (TYPE > kContainerTypeValues)
	{
		auto layout_base = Reinterpret<UInt8>(&layout) + 4;

		auto objects = Reinterpret<typename LayoutTemplateImpl<true>::ObjectInfo>(layout_base + layout.size);

		REFLEX_LOOP_PTR(objects, itr, layout.num_object)
		{
			auto & info = *itr;

			Pointer & object = *Reinterpret<Pointer>(base + info.a);

			object->RetainMt();
		}
	}

	Push(context.stack, object.Adr());
}

template <ContainerType TYPE> inline ScriptObjectImplX<TYPE>::ScriptObjectImplX(VM_CTR_PARAMS)
	: ScriptObjectCircularType<TYPE>(context, *this),
	m_type(type)
{
	SetOnHeap(g_default_allocator);

	Reflex::Detail::SetDynamicCastableTypeInfo(this, *type->object_t);
}

template <ContainerType TYPE> inline ScriptObjectImplX<TYPE>::~ScriptObjectImplX()
{
	if constexpr (TYPE > kContainerTypeValues)
	{
		auto & layout = *Reinterpret<Layout>(m_type->params.GetData());

		DestroyLayout<bool(TYPE > kContainerTypeValues), true > (layout, Reinterpret<UInt8>(this) + sizeof(ScriptObjectImplX<TYPE>));
	}
}

template <ContainerType TYPE> inline void ScriptObjectImplX<TYPE>::OnReleaseData()
{
	if constexpr (TYPE == kContainerTypeCircularObjects)
	{
		auto & layout = *Reinterpret<Layout>(m_type->params.GetData());

		auto target = Reinterpret<UInt8>(this) + sizeof(ScriptObjectImplX<TYPE>);

		auto base = Reinterpret<UInt8>(&layout) + 4;

		auto objects = Reinterpret<typename LayoutTemplateImpl<true>::ObjectInfo>(base + layout.size);

		UInt16 contextid = GetContextID();

		auto null = &REFLEX_NULL(Object);

		REFLEX_LOOP_PTR(objects, itr, layout.num_object)
		{
			auto & info = *itr;

			if (!info.b->flags.Check(Type::kFlagNonCircular))
			{
				auto & ptr = *Reinterpret<Pointer>(target + info.a);

				ReleaseContextCirculars(contextid, *ptr);

				Reflex::Detail::SetReferenceCountedPointer(ptr, null);
			}
		}
	}
}

template <ContainerType TYPE> Reflex::Object * ScriptObjectImplX<TYPE>::CrossContextCopyImpl(Context & to_context, Object & object, TypeRef to_t, bool mt)
{
	if constexpr (TYPE == kContainerTypeValues || TYPE == kContainerTypeThreadSafeObjects)
	{
		return &object;
	}
	else
	{
		auto copy = Cast<ScriptObjectImplX>(Create(to_context, to_t));

		auto from = Reinterpret<UInt8>(&object);

		auto to = Reinterpret<UInt8>(copy.Adr());

		for (auto & i : to_t->members)
		{
			auto & member = i.b;

			auto member_t = member.type;

			auto adr = member.address;

			if (member_t->IsObject())
			{
				auto from_object = *Reinterpret<Pointer>(from + adr);

				auto & to_object = *Reinterpret<Pointer>(to + adr);

				if (auto clone = member_t->contextcopyfn[mt](to_context, *from_object, member_t, mt))
				{
					Reflex::Detail::SetReferenceCountedPointer(to_object, clone);
				}
			}
			else
			{
				MemCopy(from + adr, to + adr, member_t->size);
			}
		}

		return copy.Adr();
	}
}

template <bool RESTORE> void ScriptObjectImpl::Stream(Context & context)
{
	VM_POP(ScriptObjectImpl&,ValueArray&);

	auto base = Reinterpret<UInt8>(&args.a);
	
	auto & outstream = args.b;

	auto infostream = ToView(GetFunctionData(context));

	while (infostream)
	{
		REFLEX_STATIC_ASSERT(sizeof(Tuple<UInt16, UInt8, UInt8>) == 4);

		auto info = *Reinterpret<Tuple<UInt16,UInt8,UInt8>>(infostream.data);

		infostream = Nudge(infostream, 4);

		if (info.c)
		{
			auto pointer = *Reinterpret<Pointer>(base + info.a);

			auto fns = Data::Deserialize<Pair<Function*>>(infostream);

			Dispatch(context, *(&fns.a)[RESTORE], { pointer, &outstream });
		}
		else
		{
			if constexpr (RESTORE)
			{
				RestoreBytes<false>(outstream, base + info.a, info.b);
			}
			else
			{
				VM::Append(outstream, { base + info.a, info.b });
			}
		}
	}
}

REFLEX_END_INTERNAL

Reflex::UInt Reflex::VM::Detail::ScriptObject::GetDataOffset(ContainerType type)
{
	switch (type)
	{
	case kContainerTypeValues:
		return sizeof(ScriptObjectImplX<kContainerTypeValues>);

	case kContainerTypeNonCircularObjects:
		return sizeof(ScriptObjectImplX<kContainerTypeNonCircularObjects>);

	case kContainerTypeThreadSafeObjects:
		return sizeof(ScriptObjectImplX<kContainerTypeThreadSafeObjects>);

	case kContainerTypeCircularObjects:
		return sizeof(ScriptObjectImplX<kContainerTypeCircularObjects>);
	}

	REFLEX_ASSERT(false);

	return 0;
}

Reflex::VM::TypeRef Reflex::VM::Detail::ScriptObject::RegisterType(Compiler::State & compilestate, const WString::View & path, Key32 ns, CString::View name, Variables && members, const void * playout, bool threadsafe)
{
	typedef LayoutTemplateImpl <true> LayoutTemplateX;

	auto & layout = *Cast<LayoutTemplateX>(playout);

	Data::Archive params;

	layout.PackLayout(params);

	bool objects = false;

	bool noncircular = true;

	for (auto & i : members)
	{
		auto type = i.b.type;

		objects = objects || type->IsObject();

		auto & flags = type->flags;

		noncircular = noncircular && flags.Check(Type::kFlagNonCircular);

		threadsafe = threadsafe && flags.Check(Type::kFlagThreadsafe);
	}

	switch (GetContainerType2(True(members), objects, threadsafe, noncircular))
	{
	case kContainerTypeCircularObjects:
		return ScriptObjectImplX<kContainerTypeCircularObjects>::Register(compilestate, path, ns, name, params, members);

	case kContainerTypeNonCircularObjects:
		return ScriptObjectImplX<kContainerTypeNonCircularObjects>::Register(compilestate, path, ns, name, params, members);

	case kContainerTypeThreadSafeObjects:
		return ScriptObjectImplX<kContainerTypeThreadSafeObjects>::Register(compilestate, path, ns, name, params, members);

	case kContainerTypeValues:
	case kContainerTypeNull:
	default:
		return ScriptObjectImplX<kContainerTypeValues>::Register(compilestate, path, ns, name, params, members);
	}
}
