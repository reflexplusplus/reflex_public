#include "[require].h"




//TODO

REFLEX_BEGIN_INTERNAL(Reflex::VM)

const UInt32 kFile = K32("File");

typedef VM_TEMPLATE_TARG("Array@PropertySet", VM::ArrayOfCircularObjects) ArrayOfDynamicTarg;
typedef VM_TEMPLATE_TARG("Tuple@(Array@String,Array@String)", Object) PairArrayOfStringTarg;

template <bool EXTENSION> void SplitFilename(Context & context)
{
	VM_POP1(String&);

	auto tuple_t = ReadFunctionData<TypeRef>(context);

	auto pair = EXTENSION ? File::SplitExtension(arg.GetView()) : File::SplitFilename(arg.GetView());

	auto & tuple = VM::Detail::FinaliseObject(tuple_t->ctr(context, tuple_t), tuple_t);

	VM::SetMemberObject(tuple_t, tuple, 0, New<String>(pair.a));

	VM::SetMemberObject(tuple_t, tuple, 1, New<String>(pair.b));

	VM_RTN(tuple);
}

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gFilePath("File > Path", { &gCore }, kMaxUInt8, [](Compiler::State & cstate, UInt8 context, Object &)
{
	auto bindings = cstate.bindings;


	AcquireStaticString(cstate, "File");

	auto bool_t = bindings->bool_t;

	auto string_t = bindings->string_t;

	auto pair_string_t = cstate.InstantiateTemplateType(kTuple, { string_t, string_t });

	auto array_string_t = GetTypeBySymbol(bindings, { kGlobal, K32("Array@String") });

	auto pair_array_string_t = cstate.InstantiateTemplateType(kTuple, { array_string_t, array_string_t });


	const WChar eol = 10;

	RegisterExternalObject(cstate, string_t, kFile, "kEOL", New<String>(ToView(eol)));


	AddFunction(bindings, kFile, "List", pair_array_string_t, { string_t, bool_t }, {}, Data::Pack(MakeTuple(pair_array_string_t, array_string_t)), [](Context & context)
	{
		auto [pair_array_string_t,array_string_t] = ReadFunctionData<Pair<TypeRef>>(context);

		auto string_t = context.program->bindings->string_t;

		VM_POP(String&, bool);

		auto rtn = Detail::CreateObject(context, pair_array_string_t);

		if (auto path = args.a.GetView())
		{
			auto filenames = File::List(path, args.b);

			REFLEX_LOOP(files_idx, 2)
			{
				auto & src = (&filenames.a)[files_idx];

				auto dest = SetMemberObject(pair_array_string_t, rtn, files_idx, New<ObjectArray>(context, array_string_t, string_t));

				auto pdest = dest->Extend<String>(context, src.GetSize());

				for (auto & i : src) *pdest++ = New<String>(i.key);
			}
		}

		VM_RTN(rtn);
	});

	AddFunction(bindings, kFile, "ResolveRelativePath", string_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(New<String>(File::ResolveRelativePath(arg.GetView())));
	});

	AddFunction(bindings, kFile, "ResolveIncludePath", string_t, { string_t, string_t }, [](Context & context)
	{
		VM_POP(String&,String&);

		VM_RTN(New<String>(File::ResolveIncludePath(args.a.GetView(), args.b.GetView())));
	});

	AddFunction(bindings, kFile, "CorrectTrailingStroke", string_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		auto string = arg.GetView();

		if (string && string.GetLast() != File::kStroke)
		{
			VM_RTN(New<String>(Join(string, File::kStroke)));
		}
		else
		{
			VM_RTN(arg);
		}
	});

	AddFunction(bindings, kFile, "RemoveTrailingStroke", string_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		auto string = arg.GetView();

		if (string && string.GetLast() == File::kStroke)
		{
			string.size--;

			VM_RTN(New<String>(string));
		}
		else
		{
			VM_RTN(arg);
		}
	});

	AddFunction(bindings, kFile, "ResolveExistingFolder", string_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(New<String>(File::ResolveExistingFolder(arg.GetView())));
	});


	Data::Archive param = Data::Pack(pair_string_t);

	AddFunction(bindings, kFile, "SplitFilename", pair_string_t, { string_t }, {}, param, &SplitFilename<false>);

	AddFunction(bindings, kFile, "SplitExtension", pair_string_t, { string_t }, {}, param, &SplitFilename<true>);

	AddFunction(bindings, kFile, "CheckExtension", bool_t, { string_t, string_t }, [](Context & context)
	{
		VM_POP(String&,String&);

		VM_RTN(File::CheckExtension(args.a.GetView(), args.b.GetView()));
	});

	AddFunction(bindings, kFile, "CheckExtension", bool_t, { string_t, array_string_t }, [](Context & context)
	{
		VM_POP(String&,ObjectArray&);

		auto ext = File::SplitExtension(args.a.GetView()).b;

		for (auto & i : args.b.GetView<String>())
		{
			if (CaseInsensitive::eq(ext, i->GetView())) return VM_RTN(true);
		}

		VM_RTN(false);
	});

	AddFunction(bindings, kFile, "SetExtension", string_t, { string_t, string_t }, [](Context & context)
	{
		VM_POP(String&,String&);

		VM_RTN(New<String>(File::CorrectExtension(args.a.GetView(), args.b.GetView())));
	});
});

const Reflex::VM::Module Reflex::VM::gFileIO("File > IO", { &gSystem, &gDataBinaryObject }, kMaxUInt8, [](Compiler::State & cstate, UInt8 context, Object &)
{
	auto bindings = cstate.bindings;


	AcquireStaticString(cstate, "File");

	auto void_t = bindings->void_t;

	auto bool_t = bindings->bool_t;

	auto int32_t = bindings->int32_t;

	auto string_t = bindings->string_t;


	AddFunction(bindings, kFile, "GetSize", int32_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		Pair <UInt64> attributes;

		if (System::GetFileAttributes(arg.GetView(), attributes))
		{
			VM_RTN(Int32(Min<UInt64>(attributes.a, kMaxInt32)));
		}
		else
		{
			VM_RTN(Int32(0));
		}
	});

	AddFunction(bindings, kFile, "Copy", bool_t, { string_t, string_t }, [](Context & context)
	{
		VM_POP(String&, String&);

		VM_RTN(File::Copy(args.a.GetView(), args.b.GetView()));
	});

	auto archiveobject_t = GetType<Data::ArchiveObject>(bindings);

	AddFunction(bindings, kFile, "Open", archiveobject_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(REFLEX_CREATE(Data::ArchiveObject, File::Open(arg.GetView())));
	});

	AddFunction(bindings, kFile, "Save", bool_t, { string_t, archiveobject_t }, [](Context & context)
	{
		VM_POP(String&, Data::ArchiveObject&);

		VM_RTN(File::Save(args.a.GetView(), args.b.value));
	});


	auto file_t = GetType<System::FileHandle>(bindings);

	AddFunction(bindings, kFile, "WriteLine", void_t, { file_t, string_t }, [](Context & context)
	{
		VM_POP(System::FileHandle&,String&);

		context.workspace.Clear();

		Data::EncodeUTF8(context.workspace, args.b.GetView());

		context.workspace.Push(UInt8(10));

		File::WriteBytes(args.a, context.workspace);
	});
});


