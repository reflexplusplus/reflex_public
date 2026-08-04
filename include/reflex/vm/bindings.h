#pragma once

#include "forward.h"
#include "detail/program.h"




//
//Experimental API

namespace Reflex::VM
{

	class Bindings;


	template <class TYPE> TypeRef GetType(const Bindings & bindings);

	TypeRef GetTypeBySymbol(const Bindings & bindings, Symbol symbol);

}




//
//Bindings

class Reflex::VM::Bindings : public Data::PropertySet
{
public:

	static Bindings & null;



	//lifetime

	Bindings(UInt8 context_flags);

	Bindings(const Bindings & bindings);

	~Bindings();



	//types

	Sequence <UInt64,TypeRef>::ConstRange GetTypes(Symbol symbol, UInt64 range = 1) const;

	TypeRef GetTypeByRTTID(TypeID type_id) const;



	//functions

	const Intrinsic & RegisterIntrinsic(Key32 ns, StaticString name, Opcode opcode, UInt64 param64, UInt32 param32, const Argument & rtn, const ArrayView <Argument> & args, const ArrayView <Argument> & targs = {}, UInt8 flags = 0);

	const ExternalFunction & RegisterFunction(Key32 ns, StaticString name, const Argument & rtn, const ArrayView <Argument> & args, const ArrayView <Argument> & targs, const Data::Archive::View& data, UInt8 flags, ExternalFunctionPtr fnptr);


	Sequence<UInt64,ExternalFunction>::ConstRange GetExternalFunctions(Symbol symbol, UInt64 range = 1) const;

	Sequence<UInt64,Intrinsic>::ConstRange GetIntrinsics(Symbol symbol, UInt64 range = 1) const;



	//builtins

	const TypeRef void_t;


	const TypeRef bool_t;

	const TypeRef uint8_t;

	const TypeRef int32_t;

	const TypeRef float32_t;

	const TypeRef key32_t;


	const TypeRef object_t;

	const TypeRef archive_t;

	const TypeRef string_t;

	const TypeRef callback_t;	//Fn@void, commonly used



	//info

	const UInt8 context_flags;



private:

	friend Type;


	Sequence <UInt64,TypeRef> m_types;

	Sequence <TypeID,TypeRef> m_rttid2type;


	Sequence <UInt64,Intrinsic> m_intrinsics;

	Sequence <UInt64,ExternalFunction> m_external_functions;

};




//
//Detail

REFLEX_NS(Reflex::VM::Detail)

Type * CreateValueType(Bindings & bindings, TypeID type_id, Key32 ns, StaticString name, const Data::Archive::View & null);

Type * CreateObjectType(Bindings & bindings, Reflex::Detail::DynamicTypeRef type, Key32 ns, StaticString name, bool noncircular, bool threadsafe);

inline UInt64 ToUInt64(Key32 ns, Key32 name) { return Reinterpret<UInt64>(Symbol({ ns, name })); }

inline const UInt64 & ToUInt64(const Symbol & symbol) { return Reinterpret<UInt64>(symbol); }

REFLEX_INLINE bool IsDefaultConstructable(TypeRef type)
{
	return type->ctr && type->flags.Check(Type::kFlagDefaultConstructable);
}

REFLEX_INLINE bool IsExplicitNullable(const Bindings & bindings, TypeRef type)
{
	auto legacy = bindings.QueryProperty<ObjectOf<Object*>>(type->type_id);

	return type->flags.Check(Type::kFlagExplicitNullable) && (type->null || !type->IsObject() || legacy);
}

extern const Type::CrossContextCopyFn kNoContextCopy;

extern const Type::CrossContextCopyFn kTrivialContextCopy;	//pass thru

REFLEX_END




//
//impl

REFLEX_NS(Reflex::VM::Detail)

REFLEX_INLINE void SetTypeCtr(TypeRef typeref, const Data::Archive::View & params, Type::Ctr ctr)
{
	REFLEX_ASSERT(ctr);

	auto & type = *RemoveConst(typeref);

	type.ctr = ctr;

	type.params = params;

	type.flags.Set(Type::kFlagDefaultConstructable, ctr);
}

REFLEX_INLINE void SetTypeFlag(TypeRef typeref, Type::Flags idx, bool value = true)
{
	RemoveConst(typeref)->flags.Set(idx, value);
}

REFLEX_END

template <class TYPE> inline Reflex::VM::TypeRef Reflex::VM::GetType(const Bindings & bindings)
{
	return bindings.GetTypeByRTTID(GetTypeID<TYPE>());
}

inline Reflex::VM::TypeRef Reflex::VM::GetTypeBySymbol(const Bindings & bindings, Symbol symbol)
{
	if (auto i = bindings.GetTypes(symbol)) return (*i.begin()).value;

	return 0;
}
