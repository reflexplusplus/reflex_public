#include "propertyset.h"

REFLEX_BEGIN_INTERNAL(Reflex::VM)

REFLEX_INLINE UInt8 * AddressOf(Object & object) { return Reinterpret<UInt8>(&object); };

struct ObjectOfInfo
{
	TypeRef objectof_t;
	TypeID type_id;
	UInt16 offset, size;
};

Data::Archive PackObjectOfInfo(Compiler::State & cstate, TypeRef value_t)
{
	auto objectof_t = cstate.InstantiateTemplateType(kObjectOf, { value_t });

	ObjectOfInfo objectofinfo = { objectof_t, objectof_t->type_id, UInt16(objectof_t->members.GetFirst().b.address), value_t->size };

	REFLEX_STATIC_ASSERT(sizeof(ObjectOfInfo) == sizeof(TypeRef) + 8);

	return Data::Pack(objectofinfo);
}

REFLEX_INLINE TRef <Object> ValidateContextID(Context & context, Object & object)
{
	REFLEX_ASSERT(object.GetContextID() == context.GetContextID());

	return object;
}

bool InstantiateCreateMap(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	REFLEX_USE(Detail);

	if (!(args.size & 1))
	{
		auto bindings = cstate.bindings;

		auto propertyset_t = GetType<Data::PropertySet>(bindings);

		auto key32_t = bindings->key32_t;

		Array <Argument> actualargs;

		Array <Pair<TypeRef>> object_ts;

		auto parg = args.data;

		auto end = args.end();

		while (parg < end)
		{
			parg++;

			auto value_t = (*parg++).type;

			actualargs.Push(key32_t);

			actualargs.Push(value_t);

			if (!value_t->IsObject())
			{
				object_ts.Push({ value_t, cstate.InstantiateTemplateType(kObjectOf, { value_t }) });
			}
			else
			{
				object_ts.Push({ value_t, value_t });
			}
		}

		AddFunction(bindings, ns, name, propertyset_t, actualargs, {}, Data::Pack(object_ts), [](Context & context)
		{
			auto & stack = context.stack;

			auto map = REFLEX_CREATE(PropertySet, context);

			auto value_ts = Data::Unpack< ArrayView < Pair <TypeRef> > >(GetFunctionData(context));

			auto itr = &value_ts.GetLast() + 1;

			auto end = &value_ts.GetFirst();

			while (itr > end)
			{
				Pair <TypeRef> types = *--itr;

				auto value_t = types.a;

				auto raw = Pop(stack, value_t->size);

				auto key = Detail::Pop<Key32>(stack);

				if (value_t->IsObject())
				{
					auto & object = **Reinterpret<Object *>(raw);

					map->SetProperty({ key, value_t->type_id }, object);
				}
				else
				{
					auto objectof_t = types.b;

					auto object = CreateObject(context, objectof_t);

					MemCopy(raw, AddressOf(object) + objectof_t->members.GetFirst().b.address, value_t->size);

					map->SetProperty({ key, objectof_t->type_id }, object);
				}
			}

			Push(context.stack, map);
		});

		return true;
	}

	return false;
}

TypeRef InstantiateIterator(Compiler::State & cstate, const Compiler::State::ClientData clientdata, Key32 ns, const CString::View & name, const ArrayView <TypeRef> & targs)
{
	REFLEX_USE(Detail);

	struct Iterator : public Object
	{
		Iterator(Data::PropertySet & dynamic, Object & null, UInt32 tid)
			: dynamic(dynamic),
			null(null),
			next_key(kNullKey),
			next_value(null)
		{
			auto range = dynamic.Iterate(tid);

			addresses.Allocate(range.GetSize());

			for (auto & i : range) addresses.Push({ i.key.id, tid });

			itr = addresses.GetData();

			end = itr + addresses.GetSize();

			if (itr < end)
			{
				next_value = dynamic.QueryProperty(*itr, &null);

				next_key = (*itr++).id;

				valid = true;
			}
			else
			{
				valid = false;
			}
		}

		Reference <Data::PropertySet> dynamic;

		Reference <Object> null;

		Array <Address> addresses;

		Address * itr, * end;

		Key32 next_key;

		Reference <Object> next_value;

		UInt8 valid;
	};

	auto bindings = cstate.bindings;

	if (targs.size == 1)
	{
		auto propertyset_t = GetType<Data::PropertySet>(bindings);

		auto data_t = targs.GetFirst();

		if (data_t->IsObject())
		{
			auto object_t = AcquireObjectType(cstate, REFLEX_OBJECT_TYPE(Object), ns, name);

			auto itr_t = CreateObjectType(bindings, object_t, ns, name, false, false);

			SetTypeCtr(itr_t, {}, [](VM_CTR_PARAMS) -> Object &
			{
				return FinaliseObject<Object>(*REFLEX_CREATE(Iterator, REFLEX_NULL(Data::PropertySet), REFLEX_NULL(Object), REFLEX_TYPEID(Object)), type);
			});

			AddFunction(bindings, kGlobal, Compiler::opBegin, itr_t, { propertyset_t }, { data_t }, Data::Pack(data_t), [](Context & context)
			{
				auto & stack = context.stack;

				auto type = ReadFunctionData<TypeRef>(context);

				VM_RTN(FinaliseObject(*REFLEX_CREATE(Iterator, Detail::Pop<PropertySet&>(stack), Detail::GetNull(context, type), type->type_id), type));
			});

			AddFunction(bindings, kGlobal, Compiler::opNext, bindings->uint8_t, { itr_t, VM::ByRef(bindings->key32_t), VM::ByRef(data_t) }, [](Context & context)
			{
				VM_POP(Iterator&,Key32&,Object**);

				auto & self = args.a;

				args.b = self.next_key;

				Reflex::Detail::SetReferenceCountedPointer(*args.c, self.next_value.Adr());

				VM_RTN(self.valid);

				auto & itr = self.itr;

				if (itr != self.end)
				{
					auto ptr = self.null.Adr();

					self.next_key = itr->id;

					self.next_value = self.dynamic->QueryProperty(*(itr++), ptr);

					self.valid = true;
				}
				else
				{
					self.valid = false;
				}
			});

			return itr_t;
		}
		else
		{
			//auto types = Pair(cstate.InstantiateTemplateType(VM::Detail::kObjectOf, { data_t }), data_t);

			//bindings.RegisterFunction(&Iterator::Begin, {}, kNullKey, VM::Compiler::opBegin, itr_t, { propertyset_t }, { item_t });

			//bindings.RegisterFunction(&Iterator::NextValue, {}, kNullKey, VM::Compiler::opNext, bindings.uint8_t, { itr_t, item_arg });
		}
	}

	return 0;
}

bool InstantiateClear(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	REFLEX_USE(Detail);

	if (targs.size == 1)
	{
		auto bindings = cstate.bindings;

		auto propertyset_t = args.GetFirst().type;

		auto type = targs[0].type;

		if (!type->IsObject())
		{
			type = cstate.InstantiateTemplateType(kObjectOf, { type });
		}

		if (args.size == 2)
		{
			AddMethod(bindings, name, bindings->void_t, { propertyset_t, bindings->key32_t }, targs, Data::Pack(type->type_id), [](Context & context)
			{
				auto type_id = ReadFunctionData<TypeID>(context);

				VM_POP(Object&,Key32);

				args.a.UnsetProperty({ args.b, type_id });
			});

			return true;
		}
	}

	return false;
}

bool InstantiateOpSet(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	REFLEX_USE(Detail);

	auto bindings = cstate.bindings;

	auto propertyset_t = args.GetFirst().type;

	auto key32_t = bindings->key32_t;

	if (args.size == 3)
	{
		auto data_t = args[2].type;

		if (data_t->IsObject())
		{
			AddMethod(bindings, name, bindings->void_t, { propertyset_t, key32_t, data_t }, {}, Data::Pack(data_t->type_id), [](Context & context)
			{
				VM_POP(Object&,Key32,Object&);

				Address address = { args.b, ReadFunctionData<TypeID>(context) };

				if (args.a.GetContextID() == context.GetContextID())
				{
					args.a.SetProperty(address, args.c);
				}
				else
				{
					auto & object = args.a;

					if (auto copy = CrossContextCopy(args.c, context, false))
					{
						object.SetProperty(address, *copy);
					}
				}
			});

			return true;
		}
		else
		{
			AddMethod(bindings, name, bindings->void_t, { propertyset_t, key32_t, VM::ByRef(data_t) }, {}, PackObjectOfInfo(cstate, data_t), [](Context & context)
			{
				auto & typeinfo = ReadFunctionData<ObjectOfInfo>(context);
					
				VM_POP(Object&,Key32,void*);

				REFLEX_STATIC_ASSERT(sizeof(typeinfo) == sizeof(TypeRef) + 8);

				auto objectof_t = typeinfo.objectof_t;

				auto object = ValidateContextID(context, (objectof_t->ctr)(context, objectof_t));

				MemCopy(args.c, AddressOf(object) + typeinfo.offset, typeinfo.size);

				args.a.SetProperty({ args.b, typeinfo.type_id }, object);
			});

			return true;
		}
	}

	return false;
}

bool InstantiateOpGet(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	REFLEX_USE(Detail);

	auto bindings = cstate.bindings;

	auto propertyset_t = args.GetFirst().type;

	auto key32_t = bindings->key32_t;

	auto data_t = targs[0].type;

	if (data_t->IsObject())
	{
		auto typeinfo = MakeTuple(ToUIntNative(data_t), data_t->ctr ? data_t->ctr : &Detail::GetNull);

		AddMethod(bindings, name, data_t, { propertyset_t, key32_t }, targs, Data::Pack(typeinfo), [](Context & context)
		{
			auto & typeinfo = ReadFunctionData<Pair<TypeRef,Type::Ctr>>(context);

			auto & type = *typeinfo.a;

			VM_POP(Object&,Key32);

			Address address = { args.b, type.type_id };

			auto contextid = context.GetContextID();
			
			if (auto pobject = args.a.QueryProperty(address))
			{
				//get

				if (pobject->GetContextID() == contextid)
				{
					VM_RTN(pobject);
				}
				else if (auto copy = CrossContextCopy(*pobject, context, false))
				{
					if (copy != pobject) AutoRelease(pobject);	//could be avoided (i think) if every context copy fn does AutoRelease, except TrivialCopy

					VM_RTN(copy);
				}
				else
				{
					VM_RTN(context.GetNullByRTTID(type.type_id, &REFLEX_NULL(Object)));
				}
			}
			else if (args.a.GetContextID() == contextid)
			{
				//create on same context

				auto & object = typeinfo.b(context, &type);

				Reflex::Detail::SilentReference <Object> retain(object);

				args.a.SetProperty(address, object);

				VM_RTN(object);
			}
			else
			{
				LogInstruction(kLogWarning, context, Join(type.name, " x-context aquire fail"));

				VM_RTN(context.GetNullByRTTID(type.type_id, &REFLEX_NULL(Object)));
			}
		});
	}
	else
	{
		AddMethod(bindings, name, data_t, { propertyset_t, key32_t }, targs, PackObjectOfInfo(cstate, data_t), [](Context & context)
		{
			auto & stack = context.stack;

			VM_POP(Object&,Key32);

			auto & objectofinfo = ReadFunctionData<ObjectOfInfo>(context);

			Address address = { args.b, objectofinfo.type_id };

			if (auto pobjectof = args.a.QueryProperty(address))
			{
				auto t = AutoRelease(pobjectof);

				stack.Append({ AddressOf(*pobjectof) + objectofinfo.offset, objectofinfo.size });
			}
			else
			{
				auto objectof_t = objectofinfo.objectof_t;

				auto objectof = AutoRelease(ValidateContextID(context, objectof_t->ctr(context, objectof_t)));

				args.a.SetProperty(address, objectof);

				stack.Append({ AddressOf(objectof) + objectofinfo.offset, objectofinfo.size });
			}
		});
	}

	return true;
}

bool InstantiateQuery(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	INLINE(Reflex::Object&, QueryImpl)(Context & context, Object & dynamic, TypeRef type, Key32 key, Object & fallback)
	{
		Address address = { key, type->type_id };

		auto contextid = context.GetContextID();

		auto property = dynamic.QueryProperty(address);

		if (dynamic.GetContextID() == contextid)
		{
			return property ? *property : fallback;
		}
		else if (property)
		{
			return *Detail::CrossContextCopy(*property, context, false, &fallback);
		}
		else
		{
			return fallback;
		}
	}
	END

	auto bindings = cstate.bindings;

	auto propertyset_t = args.GetFirst().type;

	if (args.size == 2 && targs.size == 1)
	{
		auto data_t = targs.GetFirst().type;

		if (data_t->IsObject())
		{
			AddMethod(bindings, name, data_t, { propertyset_t, bindings->key32_t }, targs, Data::Pack(data_t), [](Context & context)
			{
				auto data_t = ReadFunctionData<TypeRef>(context);

				VM_POP(Object&,Key32);

				VM_RTN(QueryImpl::Call(context, args.a, data_t, args.b, Detail::GetNull(context, data_t)));
			});

			return true;
		}
		else
		{
			AddMethod(bindings, name, data_t, { propertyset_t, bindings->key32_t }, targs, PackObjectOfInfo(cstate, data_t), [](Context & context)
			{
				auto & objectofinfo = ReadFunctionData<ObjectOfInfo>(context);

				VM_POP(Object&,Key32);

				auto & object = QueryImpl::Call(context, args.a, objectofinfo.objectof_t, args.b, Detail::GetNull(context, objectofinfo.objectof_t));

				context.stack.Append({ AddressOf(object) + objectofinfo.offset, objectofinfo.size });
			});

			return true;
		}
	}
	else if (args.size == 3 && targs.size < 2)
	{
		auto & data_t = args[2].type;

		if (data_t->IsObject())
		{
			AddMethod(bindings, name, data_t, { propertyset_t, bindings->key32_t, data_t }, targs, Data::Pack(data_t), [](Context & context)
			{
				auto data_t = ReadFunctionData<TypeRef>(context);

				VM_POP(Object&,Key32,Object&);

				VM_RTN(QueryImpl::Call(context, args.a, data_t, args.b, args.c));
			});

			return true;
		}
	}

	return false;
}

void BindPropertySet(Compiler::State & cstate, TypeRef propertyset_t)
{
	Key32 ns = propertyset_t->symbol.a;

	auto bindings = cstate.bindings;

	auto key32_t = bindings->key32_t;

	VM_TBIND(InstantiateClear, cstate, ns, "Clear", 1, propertyset_t, key32_t);

	VM_TBIND(InstantiateOpSet, cstate, ns, Compiler::opSet, 0, propertyset_t, key32_t, 0);

	VM_TBIND(InstantiateOpGet, cstate, ns, Compiler::opGet, 1, propertyset_t, key32_t);

	VM_TBIND(InstantiateQuery, cstate, ns, "Query", 0, propertyset_t, key32_t, 0);

	VM_TBIND(InstantiateQuery, cstate, ns, "Query", 1, propertyset_t, key32_t);
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Data::PropertySet> Reflex::VM::CreatePropertySet(Context & context)
{
	return REFLEX_CREATE(PropertySet, context);
}

Reflex::TRef <Reflex::Data::PropertySet> Reflex::VM::ImportPropertySet(Context & context, const Data::PropertySet & native)
{
	REFLEX_LOCAL(void, Recurse)(Context & context, const Data::PropertySet & native, Data::PropertySet & vm)
	{
		auto propertyset_t = REFLEX_TYPEID(Data::PropertySet);

		auto arrayof_propertyset_t = REFLEX_TYPEID(Data::PropertySetArray);

		for (auto & i : RemoveConst(native).Iterate())
		{
			auto type_id = i.key.type_id;

			if (type_id == propertyset_t)
			{
				auto out = CreatePropertySet(context);

				Call(context, Cast<Data::PropertySet>(i.value), out);

				vm.SetProperty(i.key, out);
			}
			else if (type_id == arrayof_propertyset_t)
			{
				auto output = REFLEX_CREATE(Data::PropertySetArray, Cast<Data::PropertySetArray>(i.value)->value);

				auto t = reinterpret_cast<TRef<Data::PropertySet>*>(output->value.GetData());

				ImportPropertySets(context, Cast< Data::ObjectArray <const Data::PropertySet> >(i.value)->value, t);

				vm.SetProperty({ i.key.id, arrayof_propertyset_t }, *output);
			}
			else
			{
				vm.SetProperty(i.key, i.value);
			}
		}
	}
	REFLEX_END

	auto out = CreatePropertySet(context);

	Recurse::Call(context, native, out);

	return out;
}

void Reflex::VM::ImportPropertySets(Context & context, const ArrayView < ConstReference <Data::PropertySet> > & native, TRef <Data::PropertySet> * output)
{
	for (auto & i : native)
	{
		Reflex::Detail::SetReferenceCountedPointer(*output++, ImportPropertySet(context, i).Adr());
	}
}

Reflex::TRef <Reflex::VM::ObjectArray> Reflex::VM::ImportPropertySets(Context & context, const ArrayView < ConstReference <Data::PropertySet> > & native, TypeRef array_t)
{
	auto out = Cast<ObjectArray>(Detail::CreateObject(context, array_t));

	auto dest = reinterpret_cast<Data::PropertySet**>(out->Extend(context, native.size));

	auto output = reinterpret_cast<TRef<Data::PropertySet>*>(dest);

	ImportPropertySets(context, native, output);

	for (auto & i : native)
	{
		Reflex::Detail::SetReferenceCountedPointer(*dest++, ImportPropertySet(context, i).Adr());
	}

	return out;
}

void Reflex::VM::Detail::BindPropertySetInterface(Compiler::State & cstate, TypeRef type)
{
	VM::BindPropertySet(cstate, type);
}

const Reflex::VM::Module Reflex::VM::gDataPropertySet("Data > PropertySet", { gCore }, kMaxUInt8, [](Compiler::State & cstate, UInt8 contextflags, Object &)
{
	AcquireStaticString(cstate, "Data");

	auto bindings = cstate.bindings;



	//dynamic

	auto propertyset_t = VM::RegisterObject<Data::PropertySet>(bindings, kDataNamespace, "PropertySet");

	VM::Detail::UseGlobalNull<Data::PropertySet>(propertyset_t);	//safe because not-writeable

	Detail::SetTypeCtr(propertyset_t, {}, [](VM_CTR_PARAMS) -> Object &
	{
		return *REFLEX_CREATE(PropertySet, context);
	});

	VM::BindPropertySet(cstate, propertyset_t);

	Key32 class_ns = AcquireStaticString(cstate, "PropertySet");

	cstate.RegisterTemplateType(class_ns, "Iterator", 1, false, {}, &InstantiateIterator);

	cstate.RegisterTemplateFunction(VM::kGlobal, Compiler::opCreatePropertySet, 0, { bindings->key32_t, 0 }, &InstantiateCreateMap, ExternalFunction::kFlagsVaradic | ExternalFunction::kFlagsAssociative);

	propertyset_t->contextcopyfn[false] = &PropertySet::CrossContextCopy;

	propertyset_t->contextcopyfn[true] = &PropertySet::CrossContextCopy;


	AddFunction(bindings, kDataNamespace, "Assimilate", bindings->void_t, { propertyset_t, propertyset_t }, [](VM::Context & context)
	{
		VM_POP(Data::PropertySet&,Data::PropertySet&);

		Data::Assimilate(args.a, args.b);
	});
});
