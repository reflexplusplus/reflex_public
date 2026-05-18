#pragma once

#include "../bindings.h"




REFLEX_NS(Reflex::VM::Detail)

template <auto FN> void BindObjectMethod(Bindings & bindings, TypeRef object_t, const StaticString & name);

REFLEX_END




//
//impl

REFLEX_NS(Reflex::VM)
constexpr UInt8 kMemberFunction = Function::kFlagsMember;
REFLEX_END

#define VM_MAP_TYPE(TYPE,vmtype) template <> struct TypeGetter <TYPE> { /*static TypeID GetRTTID() { return GetTypeID<TYPE>(); } */static TypeRef Get(const Bindings & bindings) { return bindings.vmtype; } }

REFLEX_NS(Reflex::VM::Detail)

VM_MAP_TYPE(bool, bool_t);
VM_MAP_TYPE(UInt8, uint8_t);
VM_MAP_TYPE(void, void_t);
VM_MAP_TYPE(Float32, float32_t);
VM_MAP_TYPE(Int32, int32_t);

template <typename T> struct MemberFunctionTraits;

template <typename T> struct MethodOpcode;

using Value32 = UInt32;

using Value8 = UInt8;


template <class TYPE>
struct UnderlyingType {};

template <class TYPE>
struct UnderlyingType < TRef <TYPE> >
{
	using Type = Object *;
};

template <class TYPE>
struct UnderlyingType <TYPE*>
{
	using Type = Object *;
};

template <class TYPE>
struct UnderlyingType <TYPE&>
{
	using Type = Object *;
};

template <>
struct UnderlyingType <void>
{
	using Type = void;
};

template <>
struct UnderlyingType <Float32>
{
	using Type = Float32;
};

template <>
struct UnderlyingType <UInt32>
{
	using Type = Value32;
};

template <>
struct UnderlyingType <Key32>
{
	using Type = Value32;
};

template <>
struct UnderlyingType <Int32>
{
	using Type = Value32;
};

template <>
struct UnderlyingType <Idx>
{
	using Type = Value32;
};

template <>
struct UnderlyingType <bool>
{
	using Type = Value8;
};

template <>
struct UnderlyingType <UInt8>
{
	using Type = Value8;
};

template <class TYPE> using UnderlyingTypeOf = typename UnderlyingType<TYPE>::Type;

template <class TYPE>
struct NonTRefImpl
{
	using Type = TYPE;
};

template <class TYPE>
struct NonTRefImpl <TRef<TYPE>>
{
	using Type = TYPE;
};

template <class TYPE> using NonTRef = typename NonTRefImpl<TYPE>::Type;

template <class TYPE> struct ArgumentTypeImpl
{
	using Type = NonConstT<NonTRef<NonRefT<NonPointerT<TYPE>>>>;
};

template <> struct ArgumentTypeImpl <UInt32>
{
	using Type = Int32;
};

template <> struct ArgumentTypeImpl <Idx>
{
	using Type = Int32;
};

template <class TYPE> using ArgumentType = typename ArgumentTypeImpl<TYPE>::Type;



template <class CLASS, class RTN, class ARG1, class ARG2>
struct MemberFunctionTraits <RTN(CLASS::*)(ARG1,ARG2)>
{
	inline static const UInt32 kNArg = 2;
	using Type = UnderlyingTypeOf<RTN>(Object::*)(UnderlyingTypeOf<ARG1>, UnderlyingTypeOf<ARG2>);
	using Rtn = ArgumentType <RTN>;
	using Arg1 = ArgumentType <ARG1>;
	using Arg2 = ArgumentType <ARG2>;
};

template <class CLASS, class RTN, class ARG1>
struct MemberFunctionTraits <RTN(CLASS::*)(ARG1)>
{
	inline static const UInt32 kNArg = 1;
	using Type = UnderlyingTypeOf<RTN>(Object::*)(UnderlyingTypeOf<ARG1>);
	using Rtn = ArgumentType <RTN>;
	using Arg1 = ArgumentType <ARG1>;
};

template <class CLASS, class RTN>
struct MemberFunctionTraits <RTN(CLASS::*)()>
{
	inline static const UInt32 kNArg = 0;
	using Type = UnderlyingTypeOf<RTN>(Object::*)();
	using Rtn = ArgumentType <RTN>;
};

template <class CLASS, class RTN, class ARG1, class ARG2>
struct MemberFunctionTraits <RTN(CLASS::*)(ARG1, ARG2) const>
{
	inline static const UInt32 kNArg = 2;
	using Type = UnderlyingTypeOf<RTN>(Object::*)(UnderlyingTypeOf<ARG1>, UnderlyingTypeOf<ARG2>);
	using Rtn = ArgumentType <RTN>;
	using Arg1 = ArgumentType <ARG1>;
	using Arg2 = ArgumentType <ARG2>;
};

template <class CLASS, class RTN, class ARG1>
struct MemberFunctionTraits <RTN(CLASS::*)(ARG1) const>
{
	inline static const UInt32 kNArg = 1;
	using Type = UnderlyingTypeOf<RTN>(Object::*)(UnderlyingTypeOf<ARG1>);
	using Rtn = ArgumentType <RTN>;
	using Arg1 = ArgumentType <ARG1>;
};

template <class CLASS, class RTN>
struct MemberFunctionTraits <RTN(CLASS::*)() const>
{
	inline static const UInt32 kNArg = 0;
	using Type = UnderlyingTypeOf<RTN>(Object::*)();
	using Rtn = ArgumentType <RTN>;
};

template <> struct MethodOpcode <void(Object::*)()>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void;
};

template <> struct MethodOpcode <Value8(Object::*)()>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Value8;
};

template <> struct MethodOpcode <Value32(Object::*)()>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Value32;
};

template <> struct MethodOpcode <Float32(Object::*)()>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Float32;
};

template <> struct MethodOpcode <Object*(Object::*)()>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Object;
};

template <> struct MethodOpcode <void(Object::*)(Object*)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void_Object;
};

template <> struct MethodOpcode <Value8(Object::*)(Object*)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Value8_Object;
};

template <> struct MethodOpcode <Value32(Object::*)(Object*)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Value32_Object;
};

template <> struct MethodOpcode <void(Object::*)(Value8)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void_Value8;
};

template <> struct MethodOpcode <void(Object::*)(Value32)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void_Value32;
};

template <> struct MethodOpcode <void(Object::*)(Float32)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void_Float32;
};

template <> struct MethodOpcode <void(Object::*)(Value32,Object*)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void_Value32_Object;
};

template <> struct MethodOpcode <Float32(Object::*)(Value32)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Float32_Value32;
};

template <> struct MethodOpcode <void(Object::*)(Value32,Value32)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void_Value32_Value32;
};

template <> struct MethodOpcode <Object*(Object::*)(Value32, Value32)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Object_Value32_Value32;
};

template <> struct MethodOpcode <void(Object::*)(Float32, Float32)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Void_Float32_Float32;
};

template <> struct MethodOpcode <Value8(Object::*)(Object*,Object*)>
{
	inline static const auto kOpcode = VM::kintrinsicObjectMethod_Value8_Object_Object;
};

template <auto FN> inline void BindObjectMethod(Bindings & bindings, TypeRef object_t, const StaticString & name)
{
	static const auto data = FN;

	using Traits = MemberFunctionTraits <decltype(FN)>;

	using BaseFnType = typename Traits::Type;

	if constexpr (Traits::kNArg == 0)
	{
		bindings.RegisterIntrinsic(object_t->symbol.a, name, MethodOpcode<BaseFnType>::kOpcode, ToUIntNative(&data), 0, TypeGetter<typename Traits::Rtn>::Get(bindings), { object_t }, {}, kMemberFunction);
	}
	else if constexpr (Traits::kNArg == 1)
	{
		bindings.RegisterIntrinsic(object_t->symbol.a, name, MethodOpcode<BaseFnType>::kOpcode, ToUIntNative(&data), 0, TypeGetter<typename Traits::Rtn>::Get(bindings), { object_t, TypeGetter<typename Traits::Arg1>::Get(bindings) }, {}, kMemberFunction);
	}
	else if constexpr (Traits::kNArg == 2)
	{
		bindings.RegisterIntrinsic(object_t->symbol.a, name, MethodOpcode<BaseFnType>::kOpcode, ToUIntNative(&data), 0, TypeGetter<typename Traits::Rtn>::Get(bindings), { object_t, TypeGetter<typename Traits::Arg1>::Get(bindings), TypeGetter<typename Traits::Arg2>::Get(bindings) }, {}, kMemberFunction);
	}
}

REFLEX_END