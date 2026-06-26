#include "library.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM)

constexpr Opcode kObjectOpEqual = (sizeof(void *) == sizeof(UInt64)) ? OPCODE(optimisationValueEqual64) : OPCODE(optimisationValueEqual32);
constexpr Opcode kObjectOpInequal = (sizeof(void *) == sizeof(UInt64)) ? OPCODE(optimisationValueInequal64) : OPCODE(optimisationValueInequal32);

constexpr CString::View kCast = "cast";

REFLEX_INLINE void FinaliseValueType(Bindings & bindings, TypeRef type)
{
	auto size = type->size;

	bindings.RegisterIntrinsic(kGlobal, Compiler::opEqual, OPCODE(intrinsicValueEqual), size, 0, bindings.bool_t, { type, type });

	bindings.RegisterIntrinsic(kGlobal, Compiler::opInequal, OPCODE(intrinsicValueInequal), size, 0, bindings.bool_t, { type, type });
}

REFLEX_END_INTERNAL

REFLEX_NS(Reflex::VM::Detail)

TypeRef BindArrayUInt8(Bindings & bindings);

REFLEX_END

REFLEX_SET_TRAIT(Reflex::VM::Detail::Int32Iterator, IsSingleThreadExclusive);




//
//

#define VM_PREREGISTER_VALUE(var,TYPE) const_cast<TypeRef&>(void_t) = CreateValueType(*this, REFLEX_TYPEID(TYPE), kGlobal, REFLEX_STRINGIFY(TYPE), Data::Pack(TYPE(0))

#define VM_REGISTER_OPERATOR(x) RegisterSymbol(Compiler::k##x, Compiler::x)

Reflex::VM::Bindings::Bindings(UInt8 context_flags)
	: context_flags(context_flags),
	void_t(0),
	bool_t(0),
	uint8_t(0),
	int32_t(0),
	float32_t(0),
	key32_t(0),
	object_t(0),
	archive_t(0),
	string_t(0),
	callback_t(0)
{
	auto PreRegisterValue = [](Bindings & bindings, TypeID type_id, CString::View name, UInt size, const TypeRef & type)
	{
		UIntNative memory = 0;

		auto null = Data::Pack(memory);

		const_cast<TypeRef&>(type) = REFLEX_CREATE(Type, bindings, type_id, kGlobal, name, Splice(null, size).a);
	};

	PreRegisterValue(*this, REFLEX_TYPEID(void), "void", 0, void_t);

	Detail::SetTypeFlag(void_t, Type::kFlagAssignable, false);


	PreRegisterValue(*this, REFLEX_TYPEID(UInt8), "UInt8", 1, uint8_t);

	const_cast<TypeRef&>(bool_t) = uint8_t;

	PreRegisterValue(*this, REFLEX_TYPEID(Int32), "Int32", 4, int32_t);


	auto & object_t = const_cast<Type*&>(this->object_t);

	object_t = REFLEX_CREATE(Type, *this, REFLEX_TYPEID(Object), kGlobal, "Object", sizeof(Object*));

	object_t->object_t = REFLEX_OBJECT_TYPE(Object);

	Detail::UseGlobalNull<Object>(object_t);

	//RegisterIntrinsic(kGlobal, Compiler::opCast, kObjectOpEqual, 0, 0, bool_t, { object_t, object_t });

	RegisterIntrinsic(kGlobal, Compiler::opEqual, kObjectOpEqual, 0, 0, bool_t, { object_t, object_t });

	RegisterIntrinsic(kGlobal, Compiler::opInequal, kObjectOpInequal, 0, 0, bool_t, { object_t, object_t });


	FinaliseValueType(*this, uint8_t);

	FinaliseValueType(*this, int32_t);


	const_cast<TypeRef&>(float32_t) = RegisterValue<Float32>(*this, kGlobal, "Float32");

	const_cast<TypeRef&>(key32_t) = RegisterValue<Key32>(*this, kGlobal, "Key32");


	const_cast<TypeRef &>(archive_t) = Detail::BindArrayUInt8(*this);


	const_cast<TypeRef&>(string_t) = RegisterObject<String>(*this, kGlobal, "String");

	Detail::SetTypeCtr(string_t, {}, [](Context & context, TypeRef string_t) -> Object&
	{
		auto & null = REFLEX_NULL(String);		//for optimisation use null instance, because threadsafe and immutable anyway
		
		if constexpr (REFLEX_DEBUG)
		{
			RemoveConst(null.GetContextID()) = context.GetContextID(); //to prevent intrinsicNewObject runtime assertion, need to set to current context
		}

		return null;
	});

	RemoveConst(string_t)->members.Push({ K32("size"), MakeMember(int32_t, REFLEX_OFFSETOF(VM::String, size), true) });


	const_cast<TypeRef&>(callback_t) = Detail::FnObject::RegisterType(*this, "Fn@void", { void_t });


	RegisterIntrinsic(kGlobal, Compiler::opLogicalNot, OPCODE(intrinsicLogicalNot8), 0, 0, bool_t, { uint8_t });
	RegisterIntrinsic(kGlobal, Compiler::opLogicalOr, OPCODE(intrinsicLogicalOr8), 0, 0, bool_t, { uint8_t, uint8_t });
	RegisterIntrinsic(kGlobal, Compiler::opLogicalAnd, OPCODE(intrinsicLogicalAnd8), 0, 0, bool_t, { uint8_t, uint8_t });

	RegisterIntrinsic(kGlobal, Compiler::opLogicalNot, OPCODE(intrinsicLogicalNot32), 0, 0, bool_t, { int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opLogicalOr, OPCODE(intrinsicLogicalOr32), 0, 0, bool_t, { int32_t, int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opLogicalAnd, OPCODE(intrinsicLogicalAnd32), 0, 0, bool_t, { int32_t, int32_t });

	RegisterIntrinsic(kGlobal, Compiler::opLessThan, OPCODE(intrinsicInt32LessThan), 0, 0, bool_t, { int32_t, int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opGreaterThan, OPCODE(intrinsicInt32GreaterThan), 0, 0, bool_t, { int32_t, int32_t });

	RegisterIntrinsic(kGlobal, Compiler::opPreInc, OPCODE(intrinsicPreIncInt32), 0, 0, int32_t, { ByRef(int32_t) });
	RegisterIntrinsic(kGlobal, Compiler::opPreDec, OPCODE(intrinsicPreDecInt32), 0, 0, int32_t, { ByRef(int32_t) });
	RegisterIntrinsic(kGlobal, Compiler::opPostInc, OPCODE(intrinsicPostIncInt32), 0, 0, int32_t, { ByRef(int32_t) });
	RegisterIntrinsic(kGlobal, Compiler::opPostDec, OPCODE(intrinsicPostDecInt32), 0, 0, int32_t, { ByRef(int32_t) });

	RegisterIntrinsic(kGlobal, Compiler::opInvert, OPCODE(intrinsicInvertInt32), 0, 0, int32_t, { int32_t });

	RegisterIntrinsic(kGlobal, Compiler::opAdd, OPCODE(intrinsicAddInt32Pair), 0, 0, int32_t, { int32_t, int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opSubtract, OPCODE(intrinsicSubInt32Pair), 0, 0, int32_t, { int32_t, int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opMultiply, OPCODE(intrinsicMulInt32Pair), 0, 0, int32_t, { int32_t, int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opDivide, OPCODE(intrinsicDivInt32Pair), 0, 0, int32_t, { int32_t, int32_t });

	RegisterIntrinsic(kGlobal, Compiler::opAddAssign, OPCODE(intrinsicAddAssignInt32), 0, 0, void_t, { ByRef(int32_t), int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opSubtractAssign, OPCODE(intrinsicSubtractAssignInt32), 0, 0, void_t, { ByRef(int32_t), int32_t });
	RegisterIntrinsic(kGlobal, Compiler::opMultiplyAssign, OPCODE(intrinsicMulAssignInt32), 0, 0, void_t, { ByRef(int32_t), int32_t });

	RegisterIntrinsic(kGlobal, Compiler::opLogicalNot, OPCODE(intrinsicLogicalNot32), 0, 0, bool_t, { float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opLogicalOr, OPCODE(intrinsicLogicalOr32), 0, 0, bool_t, { float32_t, float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opLogicalAnd, OPCODE(intrinsicLogicalAnd32), 0, 0, bool_t, { float32_t, float32_t });

	RegisterIntrinsic(kGlobal, Compiler::opLessThan, OPCODE(intrinsicFloat32LessThan), 0, 0, bool_t, { float32_t, float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opGreaterThan, OPCODE(intrinsicFloat32GreaterThan), 0, 0, bool_t, { float32_t, float32_t });

	RegisterIntrinsic(kGlobal, Compiler::opInvert, OPCODE(intrinsicInvertFloat32), 0, 0, float32_t, { float32_t });

	RegisterIntrinsic(kGlobal, Compiler::opAdd, OPCODE(intrinsicAddFloat32Pair), 0, 0, float32_t, { float32_t, float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opSubtract, OPCODE(intrinsicSubFloat32Pair), 0, 0, float32_t, { float32_t, float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opMultiply, OPCODE(intrinsicMulFloat32Pair), 0, 0, float32_t, { float32_t, float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opDivide, OPCODE(intrinsicDivFloat32Pair), 0, 0, float32_t, { float32_t, float32_t });

	RegisterIntrinsic(kGlobal, Compiler::opAddAssign, OPCODE(intrinsicAddAssignFloat32), 0, 0, void_t, { ByRef(float32_t), float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opSubtractAssign, OPCODE(intrinsicSubtractAssignFloat32), 0, 0, void_t, { ByRef(float32_t), float32_t });
	RegisterIntrinsic(kGlobal, Compiler::opMultiplyAssign, OPCODE(intrinsicMulAssignFloat32), 0, 0, void_t, { ByRef(float32_t), float32_t });


	auto int32iterator_t = RegisterObject<Detail::Int32Iterator>(*this, kGlobal, "Int32Iterator");

	AddFunction(*this, kGlobal, Compiler::opBegin, int32iterator_t, { int32_t }, {}, {}, [](Context & context)
	{
		VM_POP1(Int32);
		VM_RTN(REFLEX_CREATE(Detail::Int32Iterator, arg));
	});

	RegisterIntrinsic(kGlobal, Compiler::opNext, OPCODE(intrinsicInt32Next), 0, 0, bool_t, { int32iterator_t, ByRef(int32_t) });

	RegisterIntrinsic(kGlobal, "ToFloat32", OPCODE(intrinsicInt32ToFloat32), 0, 0, float32_t, { int32_t }, {});

	RegisterIntrinsic(kGlobal, Compiler::opCast, OPCODE(intrinsicStringToBool), 0, 0, bool_t, { string_t }, { bool_t });
	RegisterIntrinsic(kGlobal, Compiler::opCast, OPCODE(intrinsicStringToKey32), 0, 0, key32_t, { string_t }, { key32_t });
	RegisterIntrinsic(kGlobal, Compiler::opEqual, OPCODE(intrinsicStringEqual), 0, 0, bool_t, { string_t, string_t });
	RegisterIntrinsic(kGlobal, Compiler::opInequal, OPCODE(intrinsicStringInequal), 0, 0, bool_t, { string_t, string_t });
}

Reflex::VM::Bindings::Bindings(const Bindings & v)
	: Data::PropertySet(v),
	context_flags(v.context_flags),
	void_t(v.void_t),
	bool_t(v.bool_t),
	uint8_t(v.uint8_t),
	int32_t(v.int32_t),
	float32_t(v.float32_t),
	key32_t(v.key32_t),
	object_t(v.object_t),
	archive_t(v.archive_t),
	string_t(v.string_t),
	callback_t(v.callback_t),

	m_types(v.m_types),
	m_rttid2type(v.m_rttid2type),
	m_intrinsics(v.m_intrinsics),
	m_external_functions(v.m_external_functions)
{
	for (auto & i : m_types) Retain(*i.value);
}

Reflex::VM::Bindings::~Bindings()
{
	for (auto & i : m_types)
	{
		Release(*i.value);
	}

	Data::PropertySet::Clear();
}

Reflex::Sequence <Reflex::UInt64,Reflex::VM::TypeRef>::ConstRange Reflex::VM::Bindings::GetTypes(Symbol symbol, UInt64 range) const
{
	auto & symbolid = Detail::ToUInt64(symbol);

	return Sequence<UInt64,TypeRef>::ConstRange(m_types, symbolid, symbolid + range);
}

const Reflex::VM::Intrinsic & Reflex::VM::Bindings::RegisterIntrinsic(Key32 ns, CString::View name, Opcode opcode, UInt64 param64, UInt32 param32, const Argument & rtn, const ArrayView <Argument> & args, const ArrayView <Argument> & targs, UInt8 flags)
{
	REFLEX_ASSERT(rtn.type);

	Symbol symbol = { ns, name };

	auto & op = m_intrinsics.Insert(Detail::ToUInt64(symbol));

	op.type = Function::kTypeIntrinsic;

	op.opcode = opcode;

	op.symbol = symbol;

	op.rtn = rtn;

	op.targs = targs;

	op.args = args;

	op.args_hash = Detail::HashArgs(targs, args);

	op.param64 = param64;

	op.param32 = param32;

	op.name = name;

	op.flags2 = flags;

	return op;
}

const Reflex::VM::ExternalFunction & Reflex::VM::Bindings::RegisterFunction(Key32 ns, CString::View name, const Argument & rtn, const ArrayView <Argument> & args, const ArrayView <Argument> & targs, const Data::Archive::View & clientdata, UInt8 flags, ExternalFunctionPtr fnptr)
{
	REFLEX_ASSERT(rtn.type);

	Symbol symbol = { ns, name };

	auto symbolid = Detail::ToUInt64(symbol);

	auto args_hash = Detail::HashArgs(targs, args);

	for (auto & i : MakeRange(m_external_functions, symbolid, symbolid + 1))
	{
		auto & fn = i.value;

		if (fn.args_hash == args_hash) return fn;
	}

	auto & fn = m_external_functions.Insert(symbolid);

	fn.type = Function::kTypeExternal;

	fn.symbol = symbol;

	fn.name = name;

	fn.targs = targs;

	fn.rtn = rtn;

	fn.args = args;

	fn.args_hash = args_hash;

	fn.clientdata = clientdata;

	fn.externalfnptr = fnptr;

	fn.flags2 = flags;

	return fn;
}

Reflex::Sequence<Reflex::UInt64,Reflex::VM::Intrinsic>::ConstRange Reflex::VM::Bindings::GetIntrinsics(Symbol symbol, UInt64 range) const
{
	auto & symbolid = Detail::ToUInt64(symbol);

	return Sequence<UInt64,Intrinsic>::ConstRange(m_intrinsics, symbolid, symbolid + range);
}

Reflex::Sequence<Reflex::UInt64,Reflex::VM::ExternalFunction>::ConstRange Reflex::VM::Bindings::GetExternalFunctions(Symbol symbol, UInt64 range) const
{
	auto & symbolid = Detail::ToUInt64(symbol);

	return Sequence<UInt64,ExternalFunction>::ConstRange(m_external_functions, symbolid, symbolid + range);
}

Reflex::VM::TypeRef Reflex::VM::Bindings::GetTypeByRTTID(TypeID type_id) const
{
	const TypeRef null_type_ref = nullptr;

	return *m_rttid2type.SearchValue(type_id, &null_type_ref);
}

Reflex::VM::Type::Type()
{
}

Reflex::VM::Type::Type(Bindings & bindings, TypeID type_id, Key32 ns, CString::View name, UInt size)
{
	REFLEX_ASSERT(name);

	Symbol symbol = { ns, name };

	auto & types = bindings.m_types;

	auto & index = bindings.m_rttid2type;

	REFLEX_ASSERT(!types.Search(Detail::ToUInt64(symbol)));

	REFLEX_ASSERT(!index.Search(type_id));

	REFLEX_ASSERT(size <= kMaxUInt8);

	auto tidx = UInt16(types.GetSize());

	types.Acquire(Detail::ToUInt64(symbol)) = this;

	index.Acquire(type_id) = this;

	auto & type = *this;

	Retain(type);

	type.tidx = tidx;

	type.symbol = symbol;

	type.name = name;

	type.type_id = type_id;

	type.size = UInt8(size);

	type.ctr = ctr;

	type.contextcopyfn[false] = Detail::kNoContextCopy;

	type.contextcopyfn[true] = Detail::kNoContextCopy;
}

Reflex::VM::Type::Type(Bindings & bindings, TypeID type_id, Key32 ns, CString::View name, const Data::Archive::View & null)
	: Type(bindings, type_id, ns, name, null.size)
{
	params = null;

	flags |= UInt8(BitSet(0, Type::kFlagNonCircular) | BitSet(0, Type::kFlagThreadsafe));
}

Reflex::Object & Reflex::VM::Type::GetContextNull(VM_CTR_PARAMS)
{
	return *context.GetNullByRTTID(type->type_id, 0);
}

Reflex::VM::Type::Type(Bindings & bindings, Reflex::Detail::DynamicTypeRef rttypeinfo, Key32 ns, CString::View name, bool noncircular, bool threadsafe)
	: Type(bindings, rttypeinfo->type_id, ns, name, sizeof(Object*))
{
	Type::object_t = rttypeinfo;

	flags |= UInt8(BitSet(0, Type::kFlagNonCircular, noncircular) | BitSet(0, Type::kFlagThreadsafe, threadsafe));


	contextcopyfn[false] = noncircular ? Detail::kTrivialContextCopy : Detail::kNoContextCopy;

	contextcopyfn[true] = threadsafe ? Detail::kTrivialContextCopy : Detail::kNoContextCopy;


	bindings.RegisterIntrinsic(kGlobal, kCast, OPCODE(intrinsicObjectCast), ToUIntNative(rttypeinfo), 0, this, { bindings.object_t }, { this });
}

Reflex::VM::Type * Reflex::VM::Detail::CreateValueType(Bindings & bindings, UInt32 classidx, Key32 ns, CString::View name, const Data::Archive::View & null)
{
	//switch (null.size)
	//{
	//case 0:
	//case sizeof(Value8):
	//case sizeof(Value16):
	//case sizeof(Value32):
	//case sizeof(Value64):
	//case sizeof(Value128):
	//case sizeof(Value192):
	//case sizeof(Value256):
	//	break;

	//	default:
	//		return 0;
	//};

	auto type = REFLEX_CREATE(Type, bindings, classidx, ns, name, null);

	FinaliseValueType(bindings, type);

	return type;
}

Reflex::VM::Type * Reflex::VM::Detail::CreateObjectType(Bindings & bindings, Reflex::Detail::DynamicTypeRef object_t, Key32 ns, CString::View name, bool noncircular, bool threadsafe)
{
	constexpr auto CheckBase = [](Bindings & self, Reflex::Detail::DynamicTypeRef classinfo, Type::Flags flag, bool set)
	{
		while ((classinfo = classinfo->base))
		{
			if (auto vmtype = self.GetTypeByRTTID(classinfo->type_id))
			{
				auto & flags = vmtype->flags;

				if (SetFiltered(set, flags.Check(flag))) { REFLEX_ASSERT(!set); }
			}
		}
	};

	REFLEX_ASSERT_EX(object_t->type_id, "no type_id");

	auto type = REFLEX_CREATE(Type, bindings, object_t, ns, name, noncircular, threadsafe);

	if (REFLEX_DEBUG)
	{
		CheckBase(bindings, object_t, Type::kFlagNonCircular, noncircular);

		CheckBase(bindings, object_t, Type::kFlagThreadsafe, threadsafe);

		if (threadsafe)
		{
			REFLEX_ASSERT(noncircular);
		}
	}

	while ((object_t = object_t->base))
	{
		if (auto type = bindings.GetTypeByRTTID(object_t->type_id))
		{
			auto & flags = RemoveConst(type->flags);

			if (flags.Check(Type::kFlagFinal))
			{
				flags.Clear(Type::kFlagFinal);
			}
			else
			{
				break;
			}
		}
	}

	return type;
}
