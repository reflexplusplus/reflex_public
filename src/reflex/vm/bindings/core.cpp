#include "[require].h"

#include "core/array.cpp"
#include "core/string.cpp"
#include "core/tuple.cpp"
#include "core/stream.cpp"
#include "core/map.cpp"
#include "core/queue.cpp"




REFLEX_BEGIN_INTERNAL(Reflex::VM)

template <class TYPE, bool FLOAT>
struct MathBinder
{
	static inline void Call(Bindings & bindings, Key32 ns, TypeRef type)
	{
		AddFunction(bindings, kGlobal, Compiler::opBor, type, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(Reinterpret<TYPE>(args.a | args.b));
		});

		AddFunction(bindings, kGlobal, Compiler::opBand, type, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(Reinterpret<TYPE>(args.a & args.b));
		});

		AddFunction(bindings, kGlobal, Compiler::opXor, type, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(Reinterpret<TYPE>(args.a ^ args.b));
		});

		AddFunction(bindings, kGlobal, Compiler::opShl, type, { type, bindings.uint8_t }, [](Context & context)
		{
			VM_POP(TYPE,UInt8);
			VM_RTN(Reinterpret<TYPE>(args.a << args.b));
		});

		AddFunction(bindings, kGlobal, Compiler::opShr, type, { type, bindings.uint8_t }, [](Context & context)
		{
			VM_POP(TYPE,UInt8);
			VM_RTN(Reinterpret<TYPE>(args.a >> args.b));
		});

		AddFunction(bindings, kGlobal, Compiler::opMod, type, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(args.b ? Reinterpret<TYPE>(args.a % args.b) : TYPE(0));
		});
	}
};

template <class TYPE>
struct MathBinder <TYPE,true>
{
	static inline void Call(Bindings & bindings, Key32 global, TypeRef float32_t)
	{
		AddFunction(bindings, kGlobal, "RoundNearest", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(RoundNearest(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "RoundDown", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(RoundDown(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "RoundUp", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(RoundUp(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Truncate", bindings.int32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Truncate(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Sign", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Sign(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Abs", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Abs(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Pow", float32_t, { float32_t, float32_t }, [](Context & context)
		{
			VM_POP(Float32,Float32);
			VM_RTN(Pow(args.a, args.b));
		});

		AddFunction(bindings, kGlobal, "Log", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Log(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Square", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Square(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Cube", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Cube(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Quartic", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Quartic(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "LinearInterpolate", float32_t, { float32_t, float32_t, float32_t }, [](Context & context)
		{
			VM_POP(Float32,Float32,Float32);
			VM_RTN(LinearInterpolate(args.a, args.b, args.c));
		});

		AddFunction(bindings, kGlobal, "Normalise", float32_t, { float32_t, float32_t, float32_t }, [](Context & context)
		{
			VM_POP(Float32,Float32,Float32);
			VM_RTN(Normalise(args.a, args.b, args.c));
		});

		AddFunction(bindings, kGlobal, "Sin", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Sin(Detail::Pop<Float32>(context.stack)));
		});

		AddFunction(bindings, kGlobal, "Cos", float32_t, { float32_t }, [](Context & context)
		{
			VM_RTN(Cos(Detail::Pop<Float32>(context.stack)));
		});
	}
};

template <class TYPE> inline void BindMath(Bindings & bindings, TypeRef type)
{
	auto bool_t = bindings.bool_t;

	AddFunction(bindings, kGlobal, "Min", type, { type, type }, [](Context & context)
	{
		VM_POP(TYPE,TYPE);
		VM_RTN(Min<TYPE>(args.a, args.b));
	});

	AddFunction(bindings, kGlobal, "Max", type, { type, type }, [](Context & context)
	{
		VM_POP(TYPE,TYPE);
		VM_RTN(Max<TYPE>(args.a, args.b));
	});

	AddFunction(bindings, kGlobal, "Clip", type, { type, type, type }, [](Context & context)
	{
		VM_POP(TYPE,TYPE,TYPE);
		VM_RTN(Min<TYPE>(Max<TYPE>(args.a, args.b), args.c));
	});

	MathBinder<TYPE,IsType<TYPE,Float32>::value>::Call(bindings, kGlobal, type);

	if constexpr (IsType<TYPE,UInt8>::value)
	{
		AddFunction(bindings, kGlobal, Compiler::opLessThan, bool_t, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(args.a < args.b);
		});

		AddFunction(bindings, kGlobal, Compiler::opGreaterThan, bool_t, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(args.a > args.b);
		});


		//AddFunction(bindings, kGlobal, Compiler::opAdd, type, {type, type}, [](Context & context)
		//{
		//	VM_POP(TYPE,TYPE);
		//	VM_RTN(Reinterpret<TYPE>(args.a + args.b));
		//});

		//AddFunction(bindings, kGlobal, Compiler::opSubtract, type, { type, type }, [](Context & context)
		//{
		//	VM_POP(TYPE,TYPE);
		//	VM_RTN(Reinterpret<TYPE>(args.a - args.b));
		//});


		//auto byref = ByRef(type);

		//AddFunction(bindings, kGlobal, Compiler::opPreInc, type, { byref }, [](Context & context)
		//{
		//	VM_RTN(Copy(++Detail::Pop<TYPE&>(context.stack)));
		//});

		//AddFunction(bindings, kGlobal, Compiler::opPreDec, type, { byref }, [](Context & context)
		//{
		//	VM_RTN(Copy(--Detail::Pop<TYPE&>(context.stack)));
		//});

		//AddFunction(bindings, kGlobal, Compiler::opPostInc, type, {byref}, [](Context & context)
		//{
		//	VM_RTN(Detail::Pop<TYPE&>(context.stack)++);
		//});

		//AddFunction(bindings, kGlobal, Compiler::opPostDec, type, {byref}, [](Context & context)
		//{
		//	VM_RTN(Detail::Pop<TYPE&>(context.stack)--);
		//});
	}
	else
	{
		AddFunction(bindings, kGlobal, "Inside", bool_t, { type, type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE,TYPE);
			VM_RTN(Reflex::Inside<TYPE>(args.a, args.b, args.c));
		});

		AddFunction(bindings, kGlobal, "Modulo", type, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(args.b ? Reflex::Modulo(args.a, args.b) : TYPE(0));
		});

		AddFunction(bindings, kGlobal, "QuantiseDown", type, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(args.b ? Reflex::QuantiseDown(args.a, args.b) : TYPE(0));
		});

		if constexpr (IsType<TYPE,Int32>::value)
		{
			AddFunction(bindings, kGlobal, "Quantise", type, { type, type }, [](Context & context)
			{
				VM_POP(Int32,Int32);
				VM_RTN(Truncate(Reflex::Quantise(Float64(args.a), Float64(args.b))));
			});
		}
		else
		{
			AddFunction(bindings, kGlobal, "Quantise", type, { type, type }, [](Context & context)
			{
				VM_POP(TYPE,TYPE);
				VM_RTN(args.b ? Reflex::Quantise(args.a, args.b) : TYPE(0));
			});
		}

		AddFunction(bindings, kGlobal, "Quantise", type, { type, type }, [](Context & context)
		{
			VM_POP(TYPE,TYPE);
			VM_RTN(args.b ? Reflex::QuantiseUp(args.a, args.b) : TYPE(0));
		});
	}

	AddFunction(bindings, kGlobal, Compiler::opLessThanOrEqual, bool_t, { type, type }, [](Context & context)
	{
		VM_POP(TYPE,TYPE);
		VM_RTN(args.a <= args.b);
	});

	AddFunction(bindings, kGlobal, Compiler::opGreaterThanOrEqual, bool_t, { type, type }, [](Context & context)
	{
		VM_POP(TYPE,TYPE);
		VM_RTN(args.a >= args.b);
	});

	//AddFunction(bindings, kGlobal, Compiler::opAnd, bool_t, { type, type }, [](Context & context)
	//{
	//	VM_POP(TYPE,TYPE);
	//	VM_RTN(args.a && args.b);
	//});

	//AddFunction(bindings, kGlobal, Compiler::opOr, bool_t, { type, type }, [](Context & context)
	//{
	//	VM_POP(TYPE,TYPE);
	//	VM_RTN(args.a || args.b);
	//});

	//AddFunction(bindings, kGlobal, Compiler::opNot, bool_t, { type }, [](Context & context)
	//{
	//	VM_RTN(!Pop<TYPE>(context.stack));
	//});
}

void RegisterBuiltIns(Bindings & bindings)
{
	//types

	auto float32_t = bindings.float32_t;

	//AddFunction(bindings, kGlobal, "ToFloat32", float32_t, { bindings.int32_t }, [](Context & context)
	//{
	//	VM_RTN(Float32(Pop<Int32>(context.stack)));
	//});

	AddFunction(bindings, kGlobal, "ToInt32", bindings.int32_t, { bindings.uint8_t }, [](Context & context)
	{
		VM_RTN(Int32(Detail::Pop<UInt8>(context.stack)));
	});

	AddFunction(bindings, kGlobal, "ToInt32", bindings.int32_t, { float32_t }, [](Context & context)
	{
		VM_RTN(Int32(Detail::Pop<Float32>(context.stack)));
	});

	AddFunction(bindings, kGlobal, "ToUInt8", bindings.uint8_t, { bindings.int32_t }, [](Context & context)
	{
		VM_RTN(UInt8(Detail::Pop<Int32>(context.stack)));
	});



	//math

	BindMath<Float32>(bindings, float32_t);

	AddFunction(bindings, kGlobal, "SquareRoot", float32_t, { float32_t }, [](Context & context)
	{
		VM_RTN(Reflex::SquareRoot(Detail::Pop<Float32>(context.stack)));
	});

	//AddFunction(bindings, kGlobal, "Random", float32_t, { }, [](Context & context)
	//{
	//	VM_RTN(Reflex::Random() / Float(kMaxInt32));
	//});

	auto pair_float32_t = RegisterValue< Pair<Float32> >(bindings, kGlobal, "Tuple@(Float32,Float32)");

	auto members = Extend(pair_float32_t->members, 2).data;

	members[0] = { K32("a"), MakeMember(float32_t, 0, false) };
	members[1] = { K32("b"), MakeMember(float32_t, 4, false) };

	AddFunction(bindings, kGlobal, "SinCos", pair_float32_t, { float32_t }, [](Context & context)
	{
		VM_POP1(Float32);

		VM_RTN(MakeTuple(Sin(arg), Cos(arg)));
	});


	BindMath<Int32>(bindings, bindings.int32_t);

	BindMath<UInt8>(bindings, bindings.uint8_t);


	AddFunction(bindings, kGlobal, "MergeKeys", bindings.key32_t, { bindings.key32_t, bindings.key32_t }, [](Context & context)
	{
		VM_POP(UInt32,UInt32);

		VM_RTN(Reflex::Detail::MergeHashes(args.a, args.b));
	});
}

template <class TYPE> void IntegralSetFiltered(Context & context)
{
	VM_POP(TYPE&,TYPE);

	VM_RTN(SetFiltered(args.a, args.b));
}

bool InstantiateSetFiltered(Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
{
	if (args.size == 2)
	{
		auto p0_t = args[0].type;

		auto p1_t = args[1].type;

		if (!p1_t) p1_t = p0_t;

		if (!p0_t->IsObject() && p0_t == p1_t)
		{
			auto & bindings = cstate.bindings;

			auto byref =  ByRef(p0_t);

			ExternalFunctionPtr fn;

			switch (p0_t->size)
			{
			case 1:
				fn = &IntegralSetFiltered <UInt8>;
				break;

			case 4:
				fn = &IntegralSetFiltered <UInt32>;
				break;

			case 8:
				fn = &IntegralSetFiltered <UInt64>;
				break;

			default:
				AddFunction(bindings, ns, name, bindings->bool_t, { byref, byref }, {}, Data::Pack(p0_t->size), [](Context & context)
				{
					UInt8 size = VM::ReadFunctionData<UInt8>(context);

					VM_POP(void*, void*);

					if (MemCompare(args.a, args.b, size))
					{
						VM_RTN(false);
					}
					else
					{
						MemCopy(args.b, args.a, size);

						VM_RTN(true);
					}
				});
				return true;
			}

			AddFunction(bindings, ns, name, bindings->bool_t, { byref, p0_t}, fn);

			return true;
		}
	}

	return false;
}

template <bool EQUAL> void CompareObjectOf(Context & context)
{
	REFLEX_STATIC_ASSERT(sizeof(Pair<UInt8>) == 2);

	auto info = ReadFunctionData<Pair<UInt8>>(context);

	VM_POP(Object&,Object&);

	UInt8 eq = MemCompare(reinterpret_cast<UInt8 *>(&args.a) + info.a, reinterpret_cast<UInt8 *>(&args.b) + info.a, info.b);

	if constexpr (EQUAL)
	{
		VM_RTN(eq);
	}
	else
	{
		VM_RTN(!eq);
	}
}

void FinaliseObjectOf(Bindings & bindings, Type * objectof_t, TypeRef value_t, UInt address)
{
	bindings.RegisterIntrinsic(kGlobal, Compiler::opCast, OPCODE(PushMember), 0, Detail::MakeParam32(address, value_t->size), value_t, { objectof_t }, { value_t });

	Pair <UInt8> tuple = { UInt8(address), value_t->size };	//!important seperate lines, xcode optimises out

	auto data = Data::Pack(tuple);							//!important seperate lines, xcode optimises out

	AddMethod(bindings, Serialize, bindings.void_t, { objectof_t, bindings.archive_t }, {}, data, [](Context & context)
	{
		auto info = ReadFunctionData< Pair<UInt8> >(context);

		VM_POP(Object&,ValueArray&);

		VM::Append(args.b, { Reinterpret<UInt8>(&args.a) + info.a, info.b });
	});

	AddMethod(bindings, Deserialize, bindings.void_t, { objectof_t, bindings.archive_t }, {}, data, [](Context & context)
	{
		auto info = ReadFunctionData< Pair<UInt8> >(context);

		VM_POP(Object&,ValueArray&);

		RestoreBytes<false>(args.b, Reinterpret<UInt8>(&args.a) + info.a, info.b);
	});


	AddFunction(bindings, kGlobal, Compiler::opEqual, bindings.bool_t, { objectof_t, objectof_t }, {}, data, CompareObjectOf<true>);

	AddFunction(bindings, kGlobal, Compiler::opInequal, bindings.bool_t, { objectof_t, objectof_t }, {}, data, CompareObjectOf<false>);
}

void BindObjectOf(Compiler::State & cstate)
{
	cstate.RegisterTemplateType(kGlobal, "ObjectOf", 1, false, {}, [](Compiler::State & cstate, const Compiler::State::ClientData clientdata, Key32 ns, const CString::View & name, const ArrayView <TypeRef> & targs) -> TypeRef
	{
		auto value_t = targs.GetFirst();

		if (!value_t->IsObject())
		{
			Detail::Variables members;

			members.Push({ K32("value"), MakeMember(value_t, 0, false) });

			typedef Detail::LayoutTemplateImpl <true> LayoutTemplateX;

			LayoutTemplateX layout;

			layout.init_state = value_t->params;

			layout.init_state.SetSize(value_t->size);

			auto objectof_t = Detail::ScriptObject::RegisterType(cstate, {}, ns, name, std::move(members), &layout);

			UInt16 offset = objectof_t->members.GetFirst().b.address;

			FinaliseObjectOf(cstate.bindings, RemoveConst(objectof_t), value_t, offset);

			return objectof_t;
		}

		return 0;
	});

	AcquireStaticString(cstate, "value");
}

REFLEX_END_INTERNAL




//
//public

void Reflex::VM::Detail::RegisterObjectOf(Bindings & bindings, Type * objectof_t, TypeRef value_t, UInt16 address)
{
	objectof_t->members.Push({ K32("value"), MakeMember(value_t, address, false) });

	REFLEX_ASSERT(objectof_t->flags.Check(Type::kFlagNonCircular));

	REFLEX_ASSERT(objectof_t->flags.Check(Type::kFlagThreadsafe));

	FinaliseObjectOf(bindings, objectof_t, value_t, address);
}

const Reflex::VM::Module Reflex::VM::gCore("Core", {}, kMaxUInt8, [](Compiler::State & cstate, UInt8 flags, Object &)
{
	auto bindings = cstate.bindings;

	auto void_t = bindings->void_t;

	auto int32_t = bindings->int32_t;
	
	auto float32_t = bindings->float32_t;

	auto key32_t = bindings->key32_t;

	auto string_t = bindings->string_t;


	if constexpr (REFLEX_DEBUG)
	{
		AddFunction(bindings, kGlobal, "dbgbrk", void_t, {}, [](Context&)
		{
			System::DebugLog(true, "dbgbrk");
		});
	}


	RegisterBuiltIns(bindings);

	//Detail::AddFunction(bindings, kGlobal, "typeid", key32_t, {bindings.object_t}, [](Context & context)
	//{
	//	VM_RTN(Detail::Pop<Object&>(context.stack).classinfo->type_id);
	//});


	RegisterExternalObject<false>(cstate, string_t, kGlobal, "kDoubleQuote", TheLibrary::Get()->doublequote);

	BindConstant(cstate, kGlobal, "true", true);
	BindConstant(cstate, kGlobal, "false", false);

	BindArray(cstate);

	BindObjectOf(cstate);

	RegisterTuple(cstate);

	VM_TBIND(InstantiateSetFiltered, cstate, kGlobal, "SetFiltered", 0, ByRef(0), 0);

	BindString(cstate);


	constexpr auto InstantiateGetTypeName = [](Compiler::State & cstate, Key32 ns, const CString::View & name, const ArrayView <Argument> & targs, const ArrayView <Argument> & args)
	{
		auto & bindings = cstate.bindings;

		auto type = targs.GetFirst().type;

		auto fullname = Detail::MakeNamespacedSymbol(cstate, type);

		auto string = New<String>(fullname);

		bindings->SetProperty(fullname, string);

		AddFunction(bindings, ns, name, bindings->string_t, {}, targs, Data::Pack(string.Adr()), [](Context & context)
		{
			VM_RTN(ReadFunctionData<String*>(context));
		});

		return true;
	};

	VM_TBIND(InstantiateGetTypeName, cstate, kGlobal, "GetTypeName", 1);



	//standard arrays

	cstate.InstantiateTemplateType(VM::kArray, { int32_t });

	cstate.InstantiateTemplateType(VM::kArray, { float32_t });

	cstate.InstantiateTemplateType(VM::kArray, { key32_t });

	cstate.InstantiateTemplateType(VM::kArray, { string_t });



	//standard objectofs

	InstantiateObjectOf<UInt8>(cstate);

	InstantiateObjectOf<Float32>(cstate);

	InstantiateObjectOf<Int32>(cstate);

	InstantiateObjectOf<Key32>(cstate);

	InstantiateObjectOf<Pair<Float32>>(cstate);	//for RangeBar
});
