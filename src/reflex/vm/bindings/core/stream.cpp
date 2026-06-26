#include "stream.h"




REFLEX_BEGIN_INTERNAL(Reflex::VM)

inline const Function * GetPackFunction(Compiler::State & state, TypeRef type, Key32 op)
{
	auto & bindings = state.bindings;

	Symbol symbol = { type->symbol.a, op };

	auto argshash = Detail::HashArgs({}, { type, bindings->archive_t });

	try
	{
		state.EnumerateScriptFunctions(symbol, [void_t = bindings->void_t, argshash](const ScriptFunction & fn)
		{
			if (fn.args_hash == argshash && fn.rtn.type == void_t)
			{
				throw(&fn);
			}
		});
	}
	catch (const ScriptFunction * fn)
	{
		if (fn->instructions) return fn;
	}

	auto packs = bindings->GetExternalFunctions(symbol);

	for (auto & i : packs)
	{
		auto & fn = i.value;

		if (fn.args_hash == argshash)
		{
			REFLEX_ASSERT(fn.args.GetSize() == 2 && fn.rtn.type == bindings->void_t);

			return &fn;
		}
	}

	return 0;
}

REFLEX_END_INTERNAL

const Reflex::VM::Function * Reflex::VM::GetStore(Compiler::State & tbindings, TypeRef type)
{
	return GetPackFunction(tbindings, type, kSerialize);
}

const Reflex::VM::Function * Reflex::VM::GetRestore(Compiler::State & tbindings, TypeRef type)
{
	return GetPackFunction(tbindings, type, kDeserialize);
}

void Reflex::VM::Dispatch(Context & context, const Function & fn, Pair <Object*> && args)
{
	Detail::Push(context.stack, args);

	switch(fn.type)
	{
	case Function::kTypeExternal:
	{
		auto & externalfn = Cast<ExternalFunction>(fn);
		
		auto & ip = context.instructionptr;

		auto current = ip;

		auto line = current->line;

		auto file = current->file;

		Detail::Instruction program[2];

		program[0] = { line, file, OPCODE(CallExternal), 2, ToUIntNative(externalfn.externalfnptr) };

		program[1] = { line, file, OPCODE(Data), 0, ToUIntNative(&externalfn.clientdata) };
		
		ip = program;

		(reinterpret_cast<ExternalFunctionPtr>(Cast<ExternalFunction>(fn).externalfnptr))(context);

		ip = current;
	}
	break;

	case Function::kTypeScript:
		context.DoCall(Cast<ScriptFunction>(fn));
		break;
	}
}
