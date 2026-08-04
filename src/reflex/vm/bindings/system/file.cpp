
REFLEX_BEGIN_INTERNAL(Reflex::VM)

struct DirectoryIteratorFactory : public Object
{
	REFLEX_OBJECT(DirectoryIteratorFactory, Object);

	Reference <String> path;

	bool hidden = false;
};

void BindFile(Bindings & bindings, TypeRef pair_string_t, TypeRef array_pair_string_t)
{
	auto bool_t = bindings.bool_t;

	auto string_t = bindings.string_t;


	//functions
	
	AddFunction(bindings, kSystem, "GetVolumes", array_pair_string_t, {}, {}, Data::Pack(MakeTuple(pair_string_t, array_pair_string_t)), [](Context & context)
	{
		auto [pair_string_t, array_pair_string_t] = ReadFunctionData<Pair<TypeRef>>(context);

		auto & members = pair_string_t->members;

		auto a = members[0].b.address;
		auto b = members[1].b.address;

		Array < Reference <Object> > temp;

		auto volumes = AutoRelease(System::DiskIterator::Create());

		bool removable;

		WString filename, displayname;

		while (volumes->GetNext(removable, filename, displayname))
		{
			auto tuple = VM::Detail::CreateObject(context, pair_string_t);

			SetMemberObjectAtAdr(tuple, a, New<String>(filename));

			SetMemberObjectAtAdr(tuple, b, New<String>(displayname));

			temp.Push(tuple);
		}

		auto rtn = New<ObjectArray>(context, array_pair_string_t, pair_string_t);

		auto ptr = rtn->Extend<String>(context, temp.GetSize());

		for (auto & i : temp) *ptr++ = std::move(i);

		VM_RTN(rtn);
	});

	AddFunction(bindings, kSystem, "MakeDirectory", bindings.bool_t, { bindings.string_t, bindings.bool_t }, [](Context & context)
	{
		VM_POP(String&,bool);

		auto path = args.a.GetView();

		if (args.b)
		{
			File::MakePath(path);

			VM_RTN(System::IsDirectory(path));
		}
		else
		{
			VM_RTN(System::MakeDirectory(path));
		}
	});

	AddFunction(bindings, kSystem, "IsDirectory", bool_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(System::IsDirectory(arg.GetView()));
	});

	AddFunction(bindings, kSystem, "Exists", bool_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(System::Exists(arg.GetView()));
	});

	AddFunction(bindings, kSystem, "Delete", bool_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(System::Delete(arg.GetView()));
	});

	AddFunction(bindings, kSystem, "Rename", bool_t, { string_t, string_t }, [](Context & context)
	{
		VM_POP(String&, String&);

		VM_RTN(System::Rename(args.a.GetView(), args.b.GetView()));
	});



	//object

	auto directoryiterator_t = RegisterObject<DirectoryIteratorFactory>(bindings, kSystem, "DirectoryIterator");

	AddConstructor(bindings, directoryiterator_t, { string_t, bool_t }, [](Context & context)
	{
		VM_POP(String&,bool);

		auto itr = REFLEX_CREATE(DirectoryIteratorFactory);
		
		itr->path = args.a;

		itr->hidden = args.b;

		VM_RTN(itr);
	});

	auto directoryiterator_itr_t = RegisterObject<System::DirectoryIterator>(bindings, kSystem, "DirectoryIterator::Iterator");

	Detail::UseGlobalNull<System::DirectoryIterator>(directoryiterator_itr_t);

	AddFunction(bindings, kGlobal, Compiler::opBegin, directoryiterator_itr_t, { directoryiterator_t }, [](Context & context)
	{
		VM_POP1(DirectoryIteratorFactory&);

		VM_RTN(System::DirectoryIterator::Create(arg.path->GetView(), arg.hidden).Adr());
	});

	AddFunction(bindings, kGlobal, Compiler::opNext, bindings.bool_t, { directoryiterator_itr_t, ByRef(bool_t), ByRef(string_t) }, [](Context & context)
	{
		VM_POP(System::DirectoryIterator&, bool&, String*&);

		System::DirectoryIterator::Item item;

		bool rtn = args.a.GetNext(item);

		args.b = item.is_directory;

		Reflex::Detail::SetReferenceCountedPointer(args.c, String::Create(item.filename).Adr());

		VM_RTN(rtn);
	});



	auto file_t = RegisterObject<System::FileHandle>(bindings, kSystem, "FileHandle");

	AddConstructor(bindings, file_t, { string_t, bindings.key32_t, bool_t }, [](Context & context)
	{
		VM_POP(String&,UInt32,bool);

		auto mode = System::FileHandle::kModeRead;

		switch (args.b)
		{
		case K32("write"):
			mode = System::FileHandle::kModeOverwrite;
			break;

		case K32("append"):
			mode = System::FileHandle::kModeAppend;
			break;
		}

		VM_RTN(System::FileHandle::Create(args.a.GetView(), mode, args.c).Adr());
	});

	AddMethod(bindings, "GetSize", bindings.int32_t, { file_t }, [](Context & context)
	{
		VM_POP1(System::FileHandle&);

		VM_RTN(Int32(arg.GetSize()));
	});

	AddMethod(bindings, "SetPosition", bindings.void_t, { file_t, bindings.int32_t }, [](Context & context)
	{
		VM_POP(System::FileHandle&,UInt32);

		args.a.SetPosition(args.b);
	});

	AddMethod(bindings, "Read", bindings.void_t, { file_t, bindings.archive_t }, [](Context & context)
	{
		VM_POP(System::FileHandle&,ValueArray&);

		auto region = args.b.GetRegion<UInt8>();

		auto read = args.a.Read(region.data, region.size);

		args.b.m_size = read;

		args.b.m_wrap = read ? read : 1;
	});

	AddMethod(bindings, "Write", bindings.void_t, { file_t, bindings.archive_t }, [](Context & context)
	{
		VM_POP(System::FileHandle&,ValueArray&);

		File::WriteBytes(args.a, args.b.GetView<UInt8>());
	});

	AddMethod(bindings, "Flush", bindings.void_t, { file_t, bindings.bool_t }, [](Context & context)
	{
		VM_POP(System::FileHandle&,bool);

		args.a.Flush(args.b);
	});

	VM::Detail::BindObjectMethod<&System::FileHandle::Truncate>(bindings, file_t, "Truncate");


	AddFunction(bindings, kSystem, "Open", bool_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(System::Open(arg.GetView()));
	});
}

REFLEX_END_INTERNAL
