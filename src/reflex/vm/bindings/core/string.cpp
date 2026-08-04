#pragma once

#include "../../library.h"




//
//

Reflex::Output Reflex::VM::client("VM Client", kOutputQueue | (REFLEX_DEBUG ? kOutputFile | kOutputConsole : 0));

REFLEX_BEGIN_INTERNAL(Reflex::VM)

constexpr CString::View kSquareBracketOpen = "[";
constexpr CString::View kSquareBracketClose = "] ";
constexpr CString::View kColon = ":";

struct AnyArg
{
	UIntNative value;
	TypeRef type;
};

inline void BindString(Compiler::State & state)
{
	Bindings & bindings = state.bindings;

	auto void_t = bindings.void_t;

	auto bool_t = bindings.bool_t;

	auto uint8_t = bindings.uint8_t;

	auto int32_t = bindings.int32_t;

	auto float32_t = bindings.float32_t;

	auto string_t = bindings.string_t;



	state.RegisterTemplateFunction(kGlobal, Compiler::opCast, 1, { 0 }, [](Compiler::State & state, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
	{
		auto anyarg_t = targs.GetFirst().type;

		if (anyarg_t->type_id == REFLEX_TYPEID(AnyArg))
		{
			auto arg_t = args.GetFirst().type;

			if (arg_t->size <= sizeof(AnyArg::value))
			{
				AddFunction(state.bindings, ns, name, anyarg_t, args, { anyarg_t }, Data::Pack(arg_t), [](VM::Context & context)
				{
					auto type = ReadFunctionData<TypeRef>(context);

					Extend(context.stack, sizeof(AnyArg::value) - type->size);

					VM_RTN(type);
				});

				return true;
			}
		}

		return false;
	}, 0);

	auto PrintImpl = [](Context & context)
	{
		auto args = PopVaradic<AnyArg>(context);

		char bytes[128];

		CString::Region buffer = bytes;

		CString::Region itr = buffer;

		for (auto & i : args)
		{
			auto type_id = i.type->type_id;

			auto value = i.value;

			if (type_id == REFLEX_TYPEID(String))
			{
				Reflex::Detail::WriteStringDelimited(itr, ToPointer<VM::String>(value)->GetView(), Reflex::Detail::kCommaSpace);
			}
			else if (type_id == REFLEX_TYPEID(Int32))
			{
				Reflex::Detail::WriteStringDelimited(itr, Reinterpret<Int32>(value), Reflex::Detail::kCommaSpace);
			}
			else if (type_id == REFLEX_TYPEID(Float32))
			{
				Reflex::Detail::WriteStringDelimited(itr, Reinterpret<Float32>(value), Reflex::Detail::kCommaSpace);
			}
			else if (type_id == REFLEX_TYPEID(UInt8))
			{
				Reflex::Detail::WriteStringDelimited(itr, Reinterpret<UInt8>(value), Reflex::Detail::kCommaSpace);
			}
			else
			{
				Reflex::Detail::WriteStringDelimited(itr, Data::BytesToHex(Left(Data::Pack(value), i.type->size)), Reflex::Detail::kCommaSpace);
			}
		}

		CString::View output = { buffer.data, buffer.size - itr.size };

		output.size -= Min(output.size, Reflex::Detail::kCommaSpace.size);

		buffer[output.size] = 0;

		LogInstruction(ReadFunctionData<LogType>(context), context, output);
	};

	auto anyarg_t = REFLEX_CREATE(Type, bindings, REFLEX_TYPEID(AnyArg), kGlobal, "AnyArg", sizeof(AnyArg));

	anyarg_t->params = Data::Pack(MakeTuple(UIntNative(0), bindings.int32_t));

	bindings.RegisterFunction(kGlobal, "Print", void_t, { anyarg_t }, {}, Data::Pack(kLogNormal), ExternalFunction::kFlagsVaradic, PrintImpl);

	bindings.RegisterFunction(kGlobal, "Error", void_t, { anyarg_t }, {}, Data::Pack(kLogError), ExternalFunction::kFlagsVaradic, PrintImpl);


	auto array_string_t = state.InstantiateTemplateType(kArray, { bindings.string_t });


	AddFunction(bindings, kGlobal, "GetSource", string_t, {}, [](Context & context)
	{
		Detail::Push(context.stack, context.program->sources[context.instructionptr->file].path.Adr());
	});

	AddFunction(bindings, kGlobal, "GetSources", array_string_t, {}, {}, Data::Pack(array_string_t), [](Context & context)
	{
		auto & exe = context.program;

		auto & sources = exe->sources;

		auto rtn = New<ObjectArray>(context, ReadFunctionData<TypeRef>(context), context.program->bindings->string_t);

		auto dest = rtn->Extend<String>(context, sources.GetSize());

		REFLEX_LOOP(idx, sources.GetSize()) dest[idx] = sources[idx].path;

		VM_RTN(rtn);
	});


	AddFunction(bindings, kGlobal, "Split", array_string_t, { string_t, string_t }, {}, Data::Pack(array_string_t), [](Context & context)
	{
		VM_POP(String&,String&);

		auto rtn = New<ObjectArray>(context, ReadFunctionData<TypeRef>(context), context.program->bindings->string_t);

		auto source = Split(args.a.GetView(), args.b.GetView());

		auto dest = rtn->Extend<String>(context, source.GetSize());

		for (auto & i : source) (*dest++) = New<String>(i);

		VM_RTN(rtn);
	});

	AddFunction(bindings, kGlobal, "Merge", string_t, { array_string_t, string_t }, [](Context & context)
	{
		VM_POP(ArrayOfNonCircularObjects&,String&);

		if (auto view = args.a.GetView<String>())
		{
			auto delim = args.b.GetView();

			WString temp;

			for (auto & i : view) temp = Join(temp, i->GetView(), delim);

			VM_RTN(New<String>(Left(temp, temp.GetSize() - delim.size)));
		}
		else
		{
			VM_RTN(REFLEX_NULL(String));
		}
	});

	AddFunction(bindings, kGlobal, "Left", string_t, { string_t, int32_t }, [](Context & context)
	{
		VM_POP(String&,UInt32);

		VM_RTN(New<String>(Left<true>(args.a.GetView(), args.b)));
	});

	AddFunction(bindings, kGlobal, "Mid", string_t, { string_t, int32_t, int32_t }, [](Context & context)
	{
		VM_POP(String&,UInt32,UInt32);

		VM_RTN(New<String>(Mid<true>(args.a.GetView(), args.b, args.c)));
	});

	AddFunction(bindings, kGlobal, "Right", string_t, { string_t, int32_t }, [](Context & context)
	{
		VM_POP(String&,UInt32);

		VM_RTN(New<String>(Right<true>(args.a.GetView(), args.b)));
	});

	AddFunction(bindings, kGlobal, "Trim", string_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(New<String>(Trim(arg.GetView())));
	});

	AddFunction(bindings, kGlobal, "Search", bool_t, { string_t, string_t, ByRef(int32_t) }, [](Context & context)
	{
		VM_POP(String&,String&,UInt32&);

		if (auto idx = Reflex::Search(Mid<true>(args.a.GetView(), args.c), args.b.GetView()))
		{
			args.c += idx.value;

			VM_RTN(true);
		}
		else
		{
			VM_RTN(false);
		}
	});

	AddFunction(bindings, kGlobal, "Lowercase", string_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(New<String>(Lowercase(arg.GetView())));
	});

	AddFunction(bindings, kGlobal, "Uppercase", string_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(New<String>(Uppercase(arg.GetView())));
	});

	AddFunction(bindings, kGlobal, "ToInt32", int32_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(ToInt32(arg.GetView()));
	});

	AddFunction(bindings, kGlobal, "ToFloat32", float32_t, { string_t }, [](Context & context)
	{
		VM_POP1(String&);

		VM_RTN(ToFloat32(arg.GetView()));
	});

	AddFunction(bindings, kGlobal, "ToString", string_t, { uint8_t }, [](Context & context)
	{
		VM_RTN(New<String>(ToWString(Detail::Pop<UInt8>(context.stack))));
	});

	AddFunction(bindings, kGlobal, "ToString", string_t, { int32_t }, [](Context & context)
	{
		VM_RTN(New<String>(ToWString(Detail::Pop<Int32>(context.stack))));
	});

	AddFunction(bindings, kGlobal, "ToString", string_t, { float32_t, int32_t, bool_t }, [](Context & context)
	{
		VM_POP(Float32,UInt32,bool);

		VM_RTN(New<String>(ToWString(args.a, args.b, args.c)));
	});

	bindings.RegisterFunction(kGlobal, "Join", string_t, { string_t }, {}, {}, ExternalFunction::kFlagsVaradic, [](Context & context)
	{
		auto args = PopVaradic<String*>(context);

		UInt length = 0;

		for (auto & i : args) length += i->size;

		auto rtn = New<String>(length);

		auto dest = rtn->data;

		for (auto & i : args)
		{
			auto sublength = i->size;

			MemCopy(i->data, dest, sublength * sizeof(WChar));

			dest += sublength;
		}

		RemoveConst(rtn->hash) = rtn->GetView();

		RemoveConst(rtn->data[length]) = 0;

		VM_RTN(rtn);
	});
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::VM::String> Reflex::VM::String::Create(UInt length)
{
	return Reflex::Detail::Constructor<String>::CreateVariableSize(g_default_allocator, length * sizeof(WChar), kNullKey, length);
}

Reflex::TRef <Reflex::VM::String> Reflex::VM::String::Create(const WString::View & string, Key32 hash)
{
	auto length = string.size;

	auto bytes = UInt(length * sizeof(WChar));

	auto self = Reflex::Detail::Constructor<String>::CreateVariableSize(g_default_allocator, bytes, hash, length);

	MemCopy(string.data, self->data, bytes);

	self->data[length] = 0;

	return self;
}

Reflex::TRef <Reflex::VM::String> Reflex::VM::String::Create(const CString::View & string, Key32 hash)
{
	auto length = string.size;

	auto self = Reflex::Detail::Constructor<String>::CreateVariableSize(g_default_allocator, length * sizeof(WChar), hash, length);

	auto dest = self->data;

	REFLEX_FOREACH(c, string) *dest++ = c;

	self->data[length] = 0;
	
	return self;
}

void Reflex::VM::LogInstruction(LogType type, Context & context, CString && msg)
{
	auto & current = *context.instructionptr;

	client.LogEx(type, {}, kSquareBracketOpen, ToCString(current.line + 1), kColon, File::SplitFilename(context.program->sources[current.file].pathview).b, kSquareBracketClose, msg);
}

