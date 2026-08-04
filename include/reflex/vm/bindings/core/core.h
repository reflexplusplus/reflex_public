#pragma once

#include "../../compiler.h"




//
//

REFLEX_NS(Reflex::VM)

extern const Module gCore, gCoreMap, gCoreQueue, gCoreProgram;

extern const Module gSystem, gSystemDialogs;

extern const Module gDataPropertySet, gDataBinaryObject, gDataFormat, gDataSerialize, gDataString, gDataHash, gDataCompress;

extern const Module gFilePath, gFileIO;


constexpr Symbol kObjectOf = { kGlobal, K32("ObjectOf") };

constexpr Symbol kTuple = { kGlobal, K32("Tuple") };

constexpr Symbol kFn = { kGlobal, K32("Fn") };


constexpr const char * Serialize = "Serialize";

constexpr const char * Deserialize = "Deserialize";

REFLEX_DECLARE_KEY32(Serialize);

REFLEX_DECLARE_KEY32(Deserialize);


struct NonCircularObject : public Object {};


//
//c++side compatible types

template <class TYPE> TypeRef InstantiateObjectOf(Compiler::State & compilestate);	//this is because the exact offset of value depends on c++ compiler, so the script one might not be the same.  so use this first to define the c++ one before the script one is generated


REFLEX_INLINE Data::Archive PackTupleAdrs(TypeRef tuple_t)
{
	Data::Archive rtn;

	auto ptr = Reinterpret<Int16>(Extend<kAllocateExact>(rtn, tuple_t->members.GetSize() * 2).data);

	for (auto & i : tuple_t->members) *ptr++ = i.b.address;

	return rtn;
}

template <class TYPE> REFLEX_INLINE TYPE & GetMemberAtAdr(Object & tuple, UIntNative adr)
{
	if constexpr (VM_ISOBJECT(TYPE))
	{
		return *ToPointer<TYPE>(*Reinterpret<UIntNative>(Reinterpret<UInt8>(&tuple) + adr));
	}
	else
	{
		return *Reinterpret<TYPE>(Reinterpret<UInt8>(&tuple) + adr);
	}
}

template <class TYPE> REFLEX_INLINE TYPE & GetMember(TypeRef tuple_t, Object & tuple, UInt idx)
{
	return GetMemberAtAdr<TYPE>(tuple, tuple_t->members[idx].b.address);
}

template <class auto_1> REFLEX_INLINE auto SetMemberObjectAtAdr(Object & tuple, UIntNative adr, auto_1 && ptr_or_ref)
{
	Reflex::Detail::SetReferenceCountedPointer(Reinterpret<Object*>(*Reinterpret<UIntNative>(Reinterpret<UInt8>(&tuple) + adr)), &Deref(ptr_or_ref));

	return TRef(ptr_or_ref);
}

template <class auto_1> REFLEX_INLINE auto SetMemberObject(TypeRef tuple_t, Object & tuple, UInt idx, auto_1 && object)
{
	return SetMemberObjectAtAdr(tuple, tuple_t->members[idx].b.address, std::forward<auto_1>(object));
}

template <class TYPE> REFLEX_INLINE void SetMemberValueAtAdr(Object & tuple, UIntNative adr, const TYPE & value)
{
	REFLEX_STATIC_ASSERT(kIsRawCopyable<TYPE>);

	*Reinterpret<TYPE*>(ToUIntNative(&tuple) + adr) = value;
}

template <class TYPE> REFLEX_INLINE void SetMemberValue(TypeRef tuple_t, Object & tuple, UInt idx, const TYPE & value)
{
	SetMemberValueAtAdr(tuple, tuple_t->members[idx].b.address, value);
}

REFLEX_END




REFLEX_NS(Reflex::VM::Detail)

void BindPropertySetInterface(Compiler::State & state, VM::TypeRef type);

inline const UInt32 kMaxSizeMask = 0xFFFFFF;

extern UInt64 HashArgs(const ArrayView <Argument> & targs, const ArrayView <Argument> & args);

template <bool CIRCULAR> using CircularT = typename Selector<CIRCULAR,Circular,Dummy>::type;

struct NotificationObject : public Object
{
public:

	REFLEX_OBJECT(NotificationObject, Object);

	NotificationObject(VM::Detail::FnObject & callback, Object & notification);



protected:

	Reference <VM::Detail::FnObject> callback;

	Reference <Object> notification;
};

void RegisterObjectOf(Bindings & bindings, Type * objectof_t, TypeRef value_t, UInt16 address);

REFLEX_END

template <class TYPE> inline Reflex::VM::TypeRef Reflex::VM::InstantiateObjectOf(Compiler::State & compilestate)
{
	typedef ObjectOf <TYPE> ObjectOf;

	auto & bindings = compilestate.bindings;

	auto value_t = GetType<TYPE>(bindings);

	auto name = AcquireStaticString(compilestate, Detail::MakeTemplateName(compilestate, "ObjectOf", { value_t }));

	auto objectof_t = RegisterObject<ObjectOf>(bindings, kGlobal, name);

	AddConstructor(bindings, objectof_t, { value_t }, [](Context & context)
	{
		VM_RTN(REFLEX_CREATE(ObjectOf, Detail::Pop<TYPE>(context.stack)));
	});

	Detail::RegisterObjectOf(bindings, objectof_t, value_t, REFLEX_OFFSETOF(ObjectOf,value));

	compilestate.InstantiateTemplateType({ kGlobal, K32("ObjectOf") }, {value_t});


	return objectof_t;
}

REFLEX_SET_TRAIT(VM::NonCircularObject, IsNonCircular);

template <class VALUE> struct Reflex::IsNonCircular < Reflex::ObjectOf <VALUE> >
{
	static constexpr bool value = true;
};

template <class VALUE> struct Reflex::IsThreadSafe < Reflex::ObjectOf <VALUE> >
{
	static constexpr bool value = IsRawCopyable<VALUE>::value;
};
