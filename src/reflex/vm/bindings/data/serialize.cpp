REFLEX_BEGIN_INTERNAL(Reflex::VM)

bool InstantiateSerialize(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	ExternalFunctionPtr packfns[] = 
	{
		[](Context & context)
		{
			auto bytes = ReadFunctionData<UInt16>(context);

			auto & stack = context.stack;

			auto top = stack.GetData() + stack.GetSize() - bytes;

			auto & archive = **Reinterpret<ValueArray*>((top - sizeof(ValueArray*)));

			MemCopy(top, archive.Extend<UInt8>(bytes), bytes);

			stack.Shrink(bytes + sizeof(ValueArray*));
		},
		[](Context & context)
		{
			Data::Archive::View clientdataitr = GetFunctionData(context);

			auto & stack = context.stack;

			auto bytes = Data::Deserialize<UInt16>(clientdataitr);

			Data::Archive args = Data::Archive::View(Detail::Pop(stack, bytes), bytes);

			auto top = args.GetData();

			auto parchive = Detail::Pop<ValueArray*>(stack);

			while (clientdataitr)
			{
				auto [type, function] = Data::Deserialize<TypeRef,Function*>(clientdataitr);

				if (type->IsObject())
				{
					auto pobject = *Reinterpret<Object*>(top);

					Dispatch(context, *function, { pobject, parchive });

					top += sizeof(Object *);
				}
				else
				{
					Append(*parchive, { top, type->size });

					top += type->size;
				}
			}
		}
	};

	if (!targs && args.size > 1)
	{
		auto bindings = cstate.bindings;

		if (args.GetFirst().type == bindings->archive_t)
		{
			auto params = Splice(args, 1).b;

			bool complex = false;

			Data::Archive clientinfo;

			clientinfo.SetSize(2 + (sizeof(Tuple<TypeRef,Function*>) * params.size));

			auto pclientinfo = clientinfo.GetData() + 2;

			UInt16 bytes = 0;

			for (auto & i : params)
			{
				auto & type = *i.type;

				bytes += type.size;

				const Function * pack = 0;

				if (type.IsObject())
				{
					complex = true;

					pack = GetStore(cstate, &type);

					if (!pack) return false;
				}

				*Reinterpret<Tuple<TypeRef,const Function*>>(pclientinfo) = MakeTuple(&type, pack);

				pclientinfo += sizeof(Tuple<TypeRef,Function*>);
			}

			*Reinterpret<UInt16>(clientinfo.GetData()) = bytes;

			AddFunction(bindings, ns, name, bindings->void_t, args, {}, clientinfo, packfns[complex]);

			return true;
		}
	}

	return false;
}

bool InstantiateDeserialize(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	if (!targs && args.size > 1)
	{
		auto bindings = cstate.bindings;

		if (args.GetFirst().type == bindings->archive_t)
		{
			Array <Argument> arguments = args;

			Data::Archive clientinfo;

			clientinfo.SetSize(2);

			for (auto & i : Splice(arguments, 1).b)
			{
				auto & arg = RemoveConst(i);

				auto & type = *arg.type;

				const Function * unpack = 0;

				if (type.IsObject())
				{
					unpack = GetRestore(cstate, &type);

					if (!unpack) return false;
				}

				clientinfo.Append(Data::Pack(MakeTuple(&type, unpack)));

				arg.byref = true;
			}

			*Reinterpret<UInt16>(clientinfo.GetData()) = UInt16((arguments.GetSize() - 1) * sizeof(void *));

			AddFunction(bindings, ns, name, bindings->void_t, arguments, {}, clientinfo, [](Context & context)
			{
				Data::Archive::View clientdataitr = GetFunctionData(context);

				auto & stack = context.stack;

				auto bytes = Data::Deserialize<UInt16>(clientdataitr);

				Data::Archive args = Data::Archive::View(Detail::Pop(stack, bytes), bytes);

				auto parchive = Detail::Pop<ValueArray*>(stack);

				auto ptr = args.GetData();

				while (clientdataitr)
				{
					auto [type, function] = Data::Deserialize<TypeRef,Function*>(clientdataitr);

					auto remainder = parchive->GetRawRegion().size;

					if (type->IsObject())
					{
						auto object_ptr = **Reinterpret<Detail::Pointer*>(ptr);

						Dispatch(context, *function, { object_ptr, parchive });
					}
					else if (remainder >= type->size)
					{
						RestoreBytes<true>(*parchive, *Reinterpret<UInt8 *>(ptr), type->size);
					}

					ptr += sizeof(void *);
				}
			});

			return true;
		}
	}

	return false;
}

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gDataBinaryObject("Data > BinaryObject", {}, kMaxUInt8, [](Compiler::State & cstate, UInt8 contextflags, Object &)
{
	auto bindings = cstate.bindings;

	auto bool_t = bindings->bool_t;

	auto archive_t = bindings->archive_t;


	auto archiveobject_t = RegisterObject<Data::ArchiveObject>(bindings, kDataNamespace, "BinaryObject");

	struct FakeArray : public Data::Archive
	{
		using Data::Archive::m_size;
	};

	archiveobject_t->members.Push({ K32("size"), MakeMember(bindings->int32_t, REFLEX_OFFSETOF(Data::ArchiveObject, value) + REFLEX_OFFSETOF(FakeArray, m_size), true) });


	REFLEX_ASSERT(archiveobject_t->flags.Check(Type::kFlagNonCircular));

	archiveobject_t->contextcopyfn[false] = Detail::kTrivialContextCopy;

	cstate.RegisterResourceType(K32("binary"), archiveobject_t, [](const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream) -> TRef <Object>
	{
		return REFLEX_CREATE(Data::ArchiveObject, File::ReadBytes(instream));
	});


	AddMethod(bindings, Compiler::opCast, archiveobject_t, { archive_t }, { archiveobject_t }, {}, [](Context & context)
	{
		VM_POP1(ValueArray&);

		VM_RTN(REFLEX_CREATE(Data::ArchiveObject, arg.GetView<UInt8>()));
	});

	AddMethod(bindings, Compiler::opCast, archive_t, { archiveobject_t }, { archive_t }, {}, [](Context & context)
	{
		VM_POP1(Data::ArchiveObject&);

		auto object = CreateByteArray({});

		object->SetData(arg, ToRegion(arg.value));

		VM_RTN(object);
	});

	AddFunction(bindings, kGlobal, Compiler::opCast, bool_t, { archiveobject_t }, { bool_t }, {}, [](Context & context)
	{
		VM_POP1(Data::ArchiveObject&);

		VM_RTN(True(arg.value));
	});
});

const Reflex::VM::Module Reflex::VM::gDataSerialize("Data > Serialize", {}, kMaxUInt8, [](Compiler::State & cstate, UInt8 contextflags, Object &)
{
	VM_TBIND_VARADIC(InstantiateSerialize, cstate, kDataNamespace, VM::Serialize, 0, 0);

	VM_TBIND_VARADIC(InstantiateDeserialize, cstate, kDataNamespace, VM::Deserialize, 0, 0);
});
