REFLEX_BEGIN_INTERNAL(Reflex::VM)

typedef FunctionPointer <void(Data::Archive&,const WString::View&)> EncodeTextFn;

typedef FunctionPointer <void(WString&, const Data::Archive::View&)> DecodeTextFn;

void DecodeText(Context & context)
{
	auto decode = ReadFunctionData<DecodeTextFn>(context);

	VM_POP1(ValueArray&);

	WString buffer;

	decode(buffer, arg.GetView<UInt8>());

	VM_RTN(New<String>(buffer));
}

void EncodeText(Context & context)
{
	auto data = ReadFunctionData<Tuple<EncodeTextFn,TypeRef>>(context);

	VM_POP1(String&);

	auto & workspace = context.workspace;

	workspace.Clear();

	data.a(workspace, arg.GetView());

	VM_RTN(CreateByteArray(workspace));
}

REFLEX_END_INTERNAL

const Reflex::VM::Module Reflex::VM::gDataString("Data > String", {}, kMaxUInt8, [](Compiler::State & compilestate, UInt8 contextflags, Object &)
{
	auto bindings = compilestate.bindings;

	auto archive_t = bindings->archive_t;

	auto string_t = bindings->string_t;



	//encoding

	EncodeTextFn EncodeUTF8 = &Data::EncodeUTF8;
	DecodeTextFn DecodeUTF8 = &Data::DecodeUTF8;

	AddFunction(bindings, kDataNamespace, "EncodeUTF8", archive_t, { string_t }, {}, Data::Pack(MakeTuple(EncodeUTF8, archive_t)), EncodeText);

	AddFunction(bindings, kDataNamespace, "DecodeUTF8", string_t, { archive_t }, {}, Data::Pack(DecodeUTF8), DecodeText);


	EncodeTextFn EncodeUCS2 = &Data::EncodeUCS2;
	DecodeTextFn DecodeUCS2 = &Data::DecodeUCS2;

	AddFunction(bindings, kDataNamespace, "EncodeUCS2", archive_t, { string_t }, {}, Data::Pack(MakeTuple(EncodeUCS2, archive_t)), EncodeText);

	AddFunction(bindings, kDataNamespace, "DecodeUCS2", string_t, { archive_t }, {}, Data::Pack(DecodeUCS2), DecodeText);


	AddFunction(bindings, kDataNamespace, "HexToBytes", archive_t, { string_t }, [](VM::Context & context)
	{
		VM_POP1(String&);

		auto bytes = Data::HexToBytes(ToCString(arg.GetView()));

		VM_RTN(CreateByteArray(bytes));
	});

	AddFunction(bindings, kDataNamespace, "BytesToHex", string_t, { archive_t }, [](Context & context)
	{
		VM_POP1(ValueArray&);

		auto hex = Data::BytesToHex(arg.GetView<UInt8>());

		VM_RTN(New<String>(hex));
	});
});

const Reflex::VM::Module Reflex::VM::gDataHash("Data > Hash", {}, kMaxUInt8, [](Compiler::State & compilestate, UInt8 contextflags, Object &)
{
	auto bindings = compilestate.bindings;

	auto int32_t = bindings->int32_t;

	auto key32_t = bindings->key32_t;

	auto archive_t = bindings->archive_t;


	AddFunction(bindings, kDataNamespace, "Hash", archive_t, { key32_t, archive_t }, [](Context & context)
	{
		VM_POP(UInt32,ValueArray&);

		auto data = args.b.GetView<UInt8>();

		UInt8 buffer[32];
		
		UInt32 size = 0;

		switch (args.a)
		{
		case K32("sha1"):
			size = 20;
			Data::Detail::SHA1(data, buffer);
			break;

		case K32("sha256"):
			size = 32;
			Data::Detail::SHA256(data, buffer);
			break;
		}

		VM_RTN(CreateByteArray({ buffer, size }));
	});

	AddFunction(bindings, kDataNamespace, "CRC32", int32_t, { archive_t, int32_t }, [](Context & context)
	{
		VM_POP(ValueArray&,UInt32);

		VM_RTN(Data::CRC32(args.a.GetView<UInt8>(), args.b));
	});
});

const Reflex::VM::Module Reflex::VM::gDataCompress("Data > Compress", {}, kMaxUInt8, [](Compiler::State & compilestate, UInt8 contextflags, Object &)
{
	auto bindings = compilestate.bindings;

	auto key32_t = bindings->key32_t;

	auto archive_t = bindings->archive_t;

	AddFunction(bindings, kDataNamespace, "Compress", archive_t, { key32_t, archive_t }, [](Context & context)
	{
		VM_POP(UInt32,ValueArray&);

		auto rtn = CreateByteArray({});

		switch (args.a)
		{
		case K32("lz4"):
			{
				auto compressed = Data::Compress(Data::kLZ4, args.b.GetView<UInt8>());

				auto size = compressed.GetSize() + 4;

				Data::Archive::Region region = { rtn->Extend<UInt8>(size), size };

				*Reinterpret<UInt32>(region.data) = K32("lz4");

				MemCopy(compressed.GetData(), region.data + 4, compressed.GetSize());
			}
			break;
		}

		VM_RTN(rtn);
	});

	AddFunction(bindings, kDataNamespace, "Decompress", archive_t, { key32_t, archive_t }, [](Context & context)
	{
		VM_POP(UInt32, ValueArray&);

		auto rtn = CreateByteArray({});

		auto compressed = args.b.GetView<UInt8>();

		if (compressed.size > 4)
		{
			switch (args.a)
			{
			case K32("lz4"):
				if (*Reinterpret<UInt32>(compressed.data) == K32("lz4"))
				{
					auto decompressed = Data::Decompress(Data::kLZ4, Splice(compressed, 4).b);

					auto size = decompressed.GetSize();

					Data::Archive::Region region = { rtn->Extend<UInt8>(size), size };

					MemCopy(decompressed.GetData(), region.data, size);
				}
				break;
			}
		}

		VM_RTN(rtn);
	});
});
