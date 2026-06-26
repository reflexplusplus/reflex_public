#include "programimpl.h"
#include "compiler/compilerimpl.h"




REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

constexpr char g_tab[2] = { 9,0 };

constexpr CString::View kTab = g_tab;

//REFLEX_INLINE CString::View GetSymbolName(const Compiler::State & bindings, Key32 symbol)
//{
//	if (auto & name = bindings.GetStaticString(symbol, true)) return name;
//
//	return "unknown";
//}

REFLEX_INLINE CString MakeLineNumber(UInt16 i)
{
	auto temp = Join("000", ToCString(i), ": ");

	return Reflex::Right<false>(temp, 5);
}

void ProgramImpl::List(const Compiler::State & compilestate, LayoutTemplate & global_layout_tmpl, Sequence <ScriptFunction*,LayoutTemplate> & function_layouts_tmpls)
{
	INLINE(CString, Quote)(const CString::View & string)
	{
		return Join(kDoubleQuote, string, kDoubleQuote);
	}
	END

	INLINE(const ExternalFunction*, GetExternalFastCall)(const Bindings & bindings, const void * ptr)
	{
		UInt64 zero = 0;

		REFLEX_FOREACH(fn, bindings.GetExternalFunctions(Reinterpret<Symbol>(zero), kMaxUInt64))
		{
			if (fn.value.externalfnptr == ptr)
			{
				return &fn.value;
			}
		}

		return 0;
	}
	END

	LOCAL(void, List)(const Compiler::State & state, ProgramImpl & self, Data::Archive & archive, const CString::View & label, const LayoutTemplate & global, const LayoutTemplate & local, const Array <Instruction> & instructions)
	{
		auto & bindings = self.bindings;

		Data::WriteLine(archive, label);

		//self.m_debuglisting->Push({ kNullKey, 0, label });

		auto & layout = &global == &local ? global : local;

		REFLEX_FOREACH(var, layout.varnames)
		{
			Data::WriteLine(archive, Join(kTab, GetVarDesc(state, layout, var.a)));

			//self.m_debuglisting->Push({ kNullKey, var.a, Join(kTab, type.name, kSpace, GetSymbolName(compilestate, var.c)) });
		}

		//self.m_debuglisting->Push();

		Data::WriteLine(archive);

		auto linez = instructions.GetFirst().line;

		REFLEX_FOREACH(op, instructions)
		{
			if (SetFiltered(linez, op.line)) Data::WriteLine(archive);

			CString desc = Join(kTab, MakeLineNumber(op.line), kOpcodes[op.opcode], kSpace);

			switch (op.opcode)
			{
				case OPCODE(CallExternal):
				{
					auto fn = GetExternalFastCall::Call(bindings, ToPointer<void>(UIntNative(op.param64)));

					desc.Append(Join('"', fn->name, '"', ' ', ToCString(op.param32)));
				}
				break;

				case OPCODE(CallFn):
				case OPCODE(Return):
				{
					REFLEX_FOREACH(itr, self.functions)
					{
						auto & fn = itr.value;

						if (ToPointer<Instruction>(UIntNative(op.param64)) == fn.instructions.GetData())
						{
							desc.Append(Join('"', fn.name, '"'));
						}
						else if (ToPointer<ScriptFunction>(UIntNative(op.param64)) == &fn)
						{
							desc.Append(Join('"', fn.name, '"'));
						}
					}
				}
				break;

				case OPCODE(Marker):
					desc.Append(Join(Data::BytesToHex(Data::Pack(&op + 1)), kSpace, ToCString(op.param32)));
					break;

				case OPCODE(PushGlobal):
				case OPCODE(optimisationPushGlobal8):
				case OPCODE(optimisationPushGlobal32):
				case OPCODE(optimisationPushGlobal64):
				case OPCODE(PushGlobalByAdr):
				case OPCODE(PushLocal):
				case OPCODE(optimisationPushLocal8):
				case OPCODE(optimisationPushLocal32):
				case OPCODE(optimisationPushLocal64):
				case OPCODE(PushLocalByAdr):
				case OPCODE(PushMember):
				case OPCODE(PushMemberByAdr):
				case OPCODE(AssignGlobalValue):
				case OPCODE(optimisationAssignGlobalValue8):
				case OPCODE(optimisationAssignGlobalValue32):
				case OPCODE(optimisationAssignGlobalValue64):
				case OPCODE(AssignGlobalObject):
				case OPCODE(optimisationAssignGlobalObjectST):
				case OPCODE(AssignLocalValue):
				case OPCODE(optimisationAssignLocalValue8):
				case OPCODE(optimisationAssignLocalValue32):
				case OPCODE(optimisationAssignLocalValue64):
				case OPCODE(AssignLocalObject):
				case OPCODE(optimisationAssignLocalObjectST):
					desc.Append(Join(GetVarDesc(state, layout, Reinterpret<Pair<Int16,UInt16>>(op.param32).a)));
					break;

				//case OPCODE(Comment):
				//	desc = Join(kTab, MakeLineNumber(op.line), kOpcodes[op.opcode], kSpace, ToPointer<char>(UIntNative(op.param64)));
				//	self.m_debuglisting->Push({ kNullKey, op.line, Join(kTab, "Comment '", ToPointer<char>(UIntNative(op.param64)), "'")});
				//	break;

				default:
					desc.Append(Join(ToCString(Int(op.param64)), kSpace, ToCString(op.param32)));
					break;
			}

			Data::WriteLine(archive, desc);

			//self.m_debuglisting->Push({ kNullKey, op.line, Join(kTab, kOpcodes[op.opcode]) });
		}

		Data::WriteLine(archive);

		REFLEX_IF_DEBUG(self.m_debuglisting->value.Push();)
	}
	END

	LOCAL(CString,GetFunctionSignature)(const Compiler::State & bindings, const Function & fn)
	{
		if (fn.rtn.type)
		{
			GetSymbolName(bindings, fn.rtn.type->symbol.a);

			GetSymbolName(bindings, fn.symbol.a);

			CString signature = Join(MakeNamespacedSymbol(bindings, fn.rtn.type->symbol.a, fn.rtn.type->name), ' ', MakeNamespacedSymbol(bindings, fn.symbol.a, fn.name), '(');

			bool varadic = fn.flags2 & ExternalFunction::kFlagsVaradic;

			if (auto & args = fn.args)
			{
				for (auto & i : args) signature = Join(signature, MakeNamespacedSymbol(bindings, i.type->symbol.a, i.type->name, true), ", ");

				if (varadic)
				{
					signature.Append("...");
				}
				else
				{
					signature.Shrink(2);
				}
			}
			else if (varadic)
			{
				signature.Append("...");
			}

			signature.Push(')');

			return signature;
		}

		return {};
	}
	END

	auto path = File::CorrectExtension(sources.GetFirst().pathview, L"txt");

	Data::Archive archive;

	List::Call(compilestate, *this, archive, "Global", global_layout_tmpl, global_layout_tmpl, global_instructions);

	REFLEX_FOREACH(itr, functions)
	{
		auto & fn = itr.value;

		if (fn.instructions)
		{
			auto & layout = *function_layouts_tmpls.SearchValue(&fn);

			List::Call(compilestate, *this, archive, GetFunctionSignature::Call(compilestate, fn), global_layout_tmpl, layout, fn.instructions);
		}
	}

	Remove(path, L':');

	path = Reflex::Replace(path, L'/', L'_');

	auto drive = System::kPlatform == System::kPlatformMacOS ? ToView(L"/Volumes/USB_HDD/") : ToView(L"D:/");

	File::Save(Join(drive, L"vmasm/", path), archive);
}

REFLEX_END_INTERNAL
