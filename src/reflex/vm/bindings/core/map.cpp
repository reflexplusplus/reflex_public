#include "stream.h"




REFLEX_BEGIN_INTERNAL(Reflex::VM)

constexpr CString::View kIterator = "::Iterator";

template <class KEY, class VALUE, bool CIRCULAR> void BindMapCommon(Bindings & bindings, TypeRef map_t, TypeRef key_t)
{
	typedef AbstractMap<KEY,VALUE,CIRCULAR> Map;

	AddMethod(bindings, "Clear", bindings.void_t, { map_t }, [](Context & context)
	{
		auto & self = Detail::Pop<Map&>(context.stack);

		self.Sequence<KEY,VALUE>::Clear();
	});

	AddMethod(bindings, "Remove", bindings.void_t, { map_t, key_t }, [](Context & context)
	{
		VM_POP(Map&,KEY);

		RemoveKey(args.a, args.b);
	});

	AddMethod(bindings, Compiler::opCast, bindings.bool_t, { map_t }, { bindings.bool_t }, {}, [](Context & context)
	{
		VM_POP1(Map&);

		VM_RTN(bool(arg));
	});
}

template <class KEY, class VALUE> Object * MapContextCopyMt(Context & context, Object & src, TypeRef type, bool mt)
{
	auto map = Cast<AbstractMap<KEY,VALUE,false>>(src);

	auto clone = Cast<AbstractMap<KEY,VALUE,false>>(type->ctr(context, type));

	clone->Allocate(map->GetSize());

	for (auto & i : map)
	{
		clone->Insert(i.key, i.value);
	}

	return clone.Adr();
}

template <class MAP>
struct AbstractMapIterator : public Object
{
	static Object & CreateNull(VM_CTR_PARAMS)
	{
		auto self = New<AbstractMapIterator>(Cast<MAP>(Detail::GetNull(context, Data::Unpack<TypeRef>(type->params))));

		Detail::FinaliseObject(*self, type);

		return self;
	}

	AbstractMapIterator(MAP & map)
		: map(map),
		m_idx(0)
	{
	}

	Reference <MAP> map;

	UInt m_idx;
};

template <class KEY>
struct ValueMap : public AbstractMap <KEY,Data::Archive,false>
{
	typedef Sequence <KEY,Data::Archive> SequenceType;

	static TypeRef Bind(Compiler::State & cstate, const CString::View & name, TypeRef key_t, TypeRef value_t);

	ValueMap(Context & context, TypeRef map_t)
		: AbstractMap<KEY,Data::Archive,false>(context, map_t),
		m_null(map_t->params)
	{
	}

	template <bool EQUAL> static void opEqual(Context & context)
	{
		VM_POP(ValueMap&,ValueMap&);

		auto v = EQUAL ? Cast<SequenceType>(args.a) == Cast<SequenceType>(args.b) : Cast<SequenceType>(args.a) != Cast<SequenceType>(args.b);

		VM_RTN(v);
	}

	Data::Archive m_null;
};

template <class KEY, bool CIRCULAR>
struct ObjectMap : public AbstractMap <KEY,Reference<Object>,CIRCULAR>
{
	typedef Sequence <KEY,Reference<Object>> SequenceType;

	static TypeRef Bind(Compiler::State & cstate, const CString::View & name, TypeRef key_t, TypeRef value_t);

	ObjectMap(Context & context, TypeRef map_t)
		: AbstractMap<KEY,Reference<Object>,CIRCULAR>(context, map_t),
		context(context),
		m_value_t(Data::Unpack<TypeRef>(map_t->params)),
		m_ctr(m_value_t->ctr ? m_value_t->ctr : Detail::GetNull),
		m_null(Detail::GetNull(context, m_value_t))
	{
	}

	virtual void OnReleaseData() override
	{
		if constexpr (CIRCULAR)
		{
			for (auto & i : *this)
			{
				i.value.Clear();
			}
		}
	}

	Context & context;

	TypeRef m_value_t;

	Type::Ctr m_ctr;

	Reference <Object> m_null;
};

template <class KEY> TypeRef ValueMap<KEY>::Bind(Compiler::State & cstate, const CString::View & name, TypeRef key_t, TypeRef value_t)
{
	typedef AbstractMapIterator <ValueMap> Iterator;

	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto archive_t = bindings->archive_t;


	UInt32 size = value_t->size;

	auto map_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, name), kGlobal, name, true, false);

	map_t->contextcopyfn[false] = Detail::kTrivialContextCopy;

	Detail::SetTypeCtr(map_t, value_t->params, [](VM_CTR_PARAMS) -> Object &
	{
		return *REFLEX_CREATE(ValueMap, context, type);
	});

	BindMapCommon<KEY,Data::Archive,false>(bindings, map_t, key_t);

	Data::Archive map_param = Data::Pack(map_t);

	bindings->RegisterFunction(kGlobal, Compiler::opCreateMap, map_t, { key_t, value_t }, { key_t, value_t }, map_param, ExternalFunction::kFlagsVaradic | ExternalFunction::kFlagsAssociative, [](Context & context)
	{
		auto n = context.instructionptr->param32;

		auto map = REFLEX_CREATE(ValueMap, context, ReadFunctionData<TypeRef>(context));

		auto valuesize = map->m_null.GetSize();

		REFLEX_LOOP(idx, n / 2)
		{
			Data::Archive::View value = { Detail::Pop(context.stack, valuesize), valuesize };

			auto key = Detail::Pop<KEY>(context.stack);

			map->Insert(key, value);
		}

		VM_RTN(map);
	});

	AddMethod(bindings, Compiler::opGet, value_t, { map_t, key_t }, [](Context & context)
	{
		auto & stack = context.stack;

		VM_POP(ValueMap&,KEY);

		auto & self = args.a;

		if (Idx index = self.SearchGTE(args.b))
		{
			auto & item = self[index.value];

			if (item.key == args.b)
			{
				stack.Append(item.value);

				return;
			}
		}

		stack.Append(args.a.m_null);
	});

	AddMethod(bindings, Compiler::opSet, void_t, { map_t, key_t, value_t }, {}, Data::Pack(size), [](Context & context)
	{
		auto size = ReadFunctionData<UInt32>(context);

		auto c = Detail::Pop(context.stack, size);

		VM_POP(ValueMap&, KEY);

		args.a.Acquire(args.b) = Data::Archive::View(c, size);
	});

	AddMethod(bindings, "Query", value_t, { map_t, key_t }, [](Context & context)
	{
		VM_POP(ValueMap&, KEY);

		if (Idx index = args.a.SearchGTE(args.b))
		{
			auto & item = args.a[index.value];

			if (item.key == args.b)
			{
				context.stack.Append(item.value);

				return;
			}
		}

		context.stack.Append(args.a.m_null);
	});


	auto itrname = AcquireStaticString(cstate, Join(name, kIterator));

	auto iterator_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, itrname), kGlobal, itrname, false, false);

	Detail::SetTypeCtr(iterator_t, map_param, &Iterator::CreateNull);

	AddMethod(bindings, Compiler::opBegin, iterator_t, { map_t }, {}, map_param, [](Context & context)
	{
		auto & stack = context.stack;

		auto & map = Detail::Pop<ValueMap&>(stack);

		auto & itr = Detail::FinaliseObject(*REFLEX_CREATE(Iterator, map), ReadFunctionData<TypeRef>(context));

		VM_RTN(itr);
	});

	AddMethod(bindings, Compiler::opNext, bool_t, { iterator_t, ByRef(key_t), ByRef(value_t) }, {}, Data::Pack(size), [](Context & context)
	{
		auto size = ReadFunctionData<UInt32>(context);

		VM_POP(Iterator&,KEY&,void *);

		auto & self = args.a;

		auto & idx = self.m_idx;

		auto & map = *self.map;

		if (idx < map.GetSize())
		{
			auto & item = map[idx];

			args.b = item.key;

			MemCopy(item.value.GetData(), args.c, size);

			idx++;

			VM_RTN(true);
		}
		else
		{
			VM_RTN(false);
		}
	});


	AddMethod(bindings, Serialize, void_t, { map_t, archive_t }, [](Context & context)
	{
		VM_POP(ValueMap&,ValueArray&);

		auto & self = args.a;
		auto & archive = args.b;

		StoreValue(args.b, UInt16(self.GetSize()));

		for (auto & i : self)
		{
			StoreValue(archive, i.key);

			auto & value = i.value;

			Append(archive, value);
		}
	});

	AddMethod(bindings, Deserialize, void_t, { map_t, archive_t }, [](Context & context)
	{
		VM_POP(ValueMap&,ValueArray&);

		auto & self = args.a;

		auto & stream = args.b;

		self.SequenceType::Clear();

		auto n = RestoreValue<false, UInt16>(stream);

		auto valuesize = self.m_null.GetSize();

		auto bytes = (sizeof(KEY) + valuesize) * n;

		if (stream.m_size >= bytes)
		{
			REFLEX_LOOP(idx, n)
			{
				auto key = RestoreValue<true, KEY>(stream);

				auto & value = self.Insert(key);

				value.SetSize(valuesize);

				RestoreBytes<true>(stream, value.GetData(), valuesize);
			}
		}
	});


	AddFunction(bindings, kGlobal, Compiler::opEqual, bool_t, { map_t, map_t }, &ValueMap::opEqual<true>);

	AddFunction(bindings, kGlobal, Compiler::opInequal, bool_t, { map_t, map_t }, &ValueMap::opEqual<false>);


	map_t->contextcopyfn[true] = &MapContextCopyMt<KEY,Data::Archive>;

	return map_t;
}

template <class KEY, bool CIRCULAR> TypeRef ObjectMap<KEY,CIRCULAR>::Bind(Compiler::State & cstate, const CString::View & name, TypeRef key_t, TypeRef value_t)
{
	typedef AbstractMapIterator <ObjectMap> Iterator;

	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;

	auto archive_t = bindings->archive_t;


	auto map_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, name), kGlobal, name, !CIRCULAR, false);

	Detail::SetTypeCtr(map_t, Data::Pack(value_t), [](VM_CTR_PARAMS) -> Object &
	{
		return *REFLEX_CREATE(ObjectMap, context, type);
	});

	BindMapCommon<KEY,Reference<Object>,CIRCULAR>(bindings, map_t, key_t);

	
	auto map_param = Data::Pack(map_t);

	bindings->RegisterFunction(kGlobal, Compiler::opCreateMap, map_t, { key_t, value_t }, { key_t, value_t }, map_param, ExternalFunction::kFlagsVaradic | ExternalFunction::kFlagsAssociative, [](Context & context)
	{
		auto n = context.instructionptr->param32;

		auto map = REFLEX_CREATE(ObjectMap, context, ReadFunctionData<TypeRef>(context));

		REFLEX_LOOP(idx, n / 2)
		{
			VM_POP(KEY, Object &);

			map->Insert(args.a, args.b);
		}

		VM_RTN(map);
	});


	AddMethod(bindings, Compiler::opSet, bindings->void_t, { map_t, key_t, value_t }, [](Context & context)
	{
		VM_POP(ObjectMap&,KEY,Object&);

		args.a.Acquire(args.b) = args.c;
	});

	AddMethod(bindings, Compiler::opGet, value_t, { map_t, key_t }, [](Context & context)
	{
		VM_POP(ObjectMap&,KEY);

		auto & self = args.a;

		auto & item = self.Acquire(args.b);

		if (!item)
		{
			item = self.m_ctr(self.context, self.m_value_t);
		}

		VM_RTN(item.Adr());
	});

	AddMethod(bindings, "Query", value_t, { map_t, key_t }, [](Context & context)
	{
		VM_POP(ObjectMap&,KEY);

		auto & self = args.a;

		if (Idx index = self.SearchGTE(args.b))
		{
			auto & item = self[index.value];

			if (item.key == args.b)
			{
				VM_RTN(item.value.Adr());

				return;
			}
		}

		VM_RTN(self.m_null.Adr());
	});

	AddMethod(bindings, "Query", value_t, { map_t, key_t, value_t }, [](Context & context)
	{
		VM_POP(ObjectMap&, KEY, Object*);

		auto & self = args.a;

		if (Idx index = self.SearchGTE(args.b))
		{
			auto & item = self[index.value];

			if (item.key == args.b)
			{
				VM_RTN(item.value.Adr());

				return;
			}
		}

		VM_RTN(args.c);
	});


	auto itrname = AcquireStaticString(cstate, Join(name, kIterator));

	auto iterator_t = Detail::CreateObjectType(bindings, Detail::AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), kGlobal, itrname), kGlobal, itrname, CIRCULAR, false);

	Detail::SetTypeCtr(iterator_t, Data::Pack(map_t), &Iterator::CreateNull);

	AddMethod(bindings, Compiler::opBegin, iterator_t, { map_t }, {}, map_param, [](Context & context)
	{
		auto & stack = context.stack;

		auto & map = Detail::Pop<ObjectMap&>(stack);

		auto & itr = Detail::FinaliseObject(*REFLEX_CREATE(Iterator, map), ReadFunctionData<TypeRef>(context));

		VM_RTN(itr);
	});

	AddMethod(bindings, Compiler::opNext, bindings->bool_t, { iterator_t, ByRef(key_t), ByRef(value_t) }, [](Context & context)
	{
		VM_POP(Iterator&,KEY&,Object*&);

		auto & self = args.a;

		auto & idx = self.m_idx;

		auto & map = *self.map;

		if (idx < map.GetSize())
		{
			auto & item = map[idx];

			args.b = item.key;

			Reflex::Detail::SetReferenceCountedPointer(args.c, item.value.Adr());

			idx++;

			VM_RTN(true);
		}
		else
		{
			VM_RTN(false);
		}
	});

	if (auto pack = GetStore(cstate, value_t))
	{
		AddMethod(bindings, Serialize, void_t, { map_t, archive_t }, {}, Data::Pack(pack), [](Context & context)
		{
			auto & pack = *ReadFunctionData<Function*>(context);

			VM_POP(ObjectMap&,ValueArray&);

			auto & self = args.a;
			
			auto & stream = args.b;

			StoreValue(stream, UInt16(self.GetSize()));

			for (auto & i : self)
			{
				StoreValue(stream, i.key);

				Dispatch(context, pack, { i.value.Adr(), &stream });
			}
		});
	}

	if (auto unpack = GetRestore(cstate, value_t))
	{
		AddMethod(bindings, Deserialize, void_t, { map_t, archive_t }, {}, Data::Pack(unpack), [](Context & context)
		{
			auto & unpack = *ReadFunctionData<Function*>(context);

			VM_POP(ObjectMap&,ValueArray&);

			auto & self = args.a;
			
			auto & stream = args.b;

			self.SequenceType::Clear();

			auto size = RestoreValue<false,UInt16>(stream);

			auto ctr = self.m_ctr;

			auto value_t = self.m_value_t;

			REFLEX_LOOP(idx, size)
			{
				Object & object = self.Insert(RestoreValue<false,KEY>(stream), ctr(context, value_t));

				Dispatch(context, unpack, { &object, &stream });
			}
		});
	}


	if (!Copy(CIRCULAR) && value_t->contextcopyfn[true] == Detail::kTrivialContextCopy)
	{
		map_t->contextcopyfn[true] = &MapContextCopyMt<KEY,Reference<Object>>;
	}
	else if (value_t->contextcopyfn[true] != Detail::kNoContextCopy)
	{
		map_t->contextcopyfn[true] = [](Context & context, Object & src, TypeRef type, bool mt) -> Object *
		{
			auto map = Cast<ObjectMap>(src);

			auto clone = Cast<ObjectMap>(type->ctr(context, type));

			clone->Allocate(map->GetSize());

			for (auto & i : map)
			{
				clone->Insert(i.key, i.value);
			}

			return clone.Adr();
		};
	}

	return map_t;
}

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gCoreMap("Core > Map", { gCore }, kMaxUInt8, [](Compiler::State & cstate, UInt8 context, Object & clientdata)
{
	cstate.RegisterTemplateType(kGlobal, "Map", 2, false, {}, [](Compiler::State & cstate, const Compiler::State::ClientData clientdata, Key32 ns, const CString::View & name, const ArrayView <TypeRef> & targs)
	{
		auto key_t = targs[0];

		auto value_t = targs[1];

		if (!key_t->IsObject() && key_t->size <= 16)
		{
			if (value_t->IsObject())
			{
				bool circular = !value_t->flags.Check(Type::kFlagNonCircular);

				switch (key_t->size)
				{
				case 4:
					if (circular)
					{
						return ObjectMap<UInt32, true>::Bind(cstate, name, key_t, value_t);
					}
					else
					{
						return ObjectMap<UInt32, false>::Bind(cstate, name, key_t, value_t);
					}

				case 8:
					if (circular)
					{
						return ObjectMap<UInt64, true>::Bind(cstate, name, key_t, value_t);
					}
					else
					{
						return ObjectMap<UInt64, false>::Bind(cstate, name, key_t, value_t);
					}
				}
			}
			else
			{
				switch (key_t->size)
				{
				case 4:
					return ValueMap<UInt32>::Bind(cstate, name, key_t, value_t);

				case 8:
					return ValueMap<UInt64>::Bind(cstate, name, key_t, value_t);
				}
			}
		}

		return TypeRef(0);
	});

	auto InstantiateCreateMap = [](Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
	{
		auto key_t = targs.GetFirst().type;

		auto value_t = targs.GetLast().type;

		cstate.InstantiateTemplateType(kMap, { key_t, value_t });

		return true;
	};

	VM_TBIND_X(InstantiateCreateMap, cstate, kGlobal, Compiler::opCreateMap, ExternalFunction::kFlagsVaradic | ExternalFunction::kFlagsAssociative, 2, 0, 0);
});
