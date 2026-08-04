#pragma once

#include "bind.h"
#include "../traits.h"




//
//typetraits

REFLEX_NS(Reflex::VM)

template <class TYPE> Type * RegisterValue(Bindings & bindings, Key32 ns, StaticString name);

template <class TYPE> Type * RegisterObject(Bindings & bindings, Key32 ns, StaticString name);

REFLEX_INLINE Detail::Variable MakeMember(TypeRef type, UIntNative address, bool is_const) { return { type, Int16(address), Detail::Location::kMember, is_const }; }

REFLEX_END

#define VM_BIND_MEMBER(TYPE, VAR, tbindings, type) Reflex::VM::Detail::NamedVariable({VM::AcquireStaticString(tbindings, REFLEX_STRINGIFY(VAR)), Reflex::VM::MakeMember(type, VM_OFFSETOF(TYPE,VAR), VM_ISCONST(TYPE,VAR)) })




//
//impl

#define VM_OFFSETOF(TYPE,MEMBER) Reflex::Int16(Reflex::ToUIntNative(Reflex::GetAdr(((TYPE*)(0))->MEMBER)))
#define VM_ISCONST(TYPE,MEMBER) Reflex::IsConst<decltype(Reflex::ToPointer<TYPE>(0)->MEMBER)>::value

REFLEX_NS(Reflex::VM::Detail)

template <class TYPE> inline void UseGlobalNull(TypeRef typeref)
{
	REFLEX_STATIC_ASSERT(Reflex::Detail::kIsNullable <TYPE>);

	RemoveConst(typeref)->null = [](VM_CTR_PARAMS) -> Object &
	{
		return Reflex::Detail::GetNullInstance<TYPE>();
	};

	RemoveConst(typeref)->flags.Set(Type::kFlagExplicitNullable, true);
}

REFLEX_END

template <class TYPE> inline Reflex::VM::Type * Reflex::VM::RegisterValue(Bindings & bindings, Key32 ns, StaticString name)
{
	REFLEX_STATIC_ASSERT(!kIsObject<TYPE>);

	TYPE null = {};

	return Detail::CreateValueType(bindings, GetTypeID<TYPE>(), ns, name, Data::Pack(null));
}

template <class TYPE> inline Reflex::VM::Type * Reflex::VM::RegisterObject(Bindings & bindings, Key32 ns, StaticString name)
{
	REFLEX_STATIC_ASSERT(kIsObject<TYPE>);

	REFLEX_STATIC_ASSERT(!((IsThreadSafe<TYPE>::value) && !(IsNonCircular<TYPE>::value)));
	REFLEX_STATIC_ASSERT(!(kIsThreadSafe<TYPE> && !kIsNonCircular<TYPE>));

	auto type = Detail::CreateObjectType(bindings, REFLEX_OBJECT_TYPE(TYPE), ns, name, kIsNonCircular<TYPE>, kIsThreadSafe<TYPE>);

	if constexpr (std::is_default_constructible<TYPE>::value)
	{
		Detail::SetTypeCtr(type, {}, [](VM_CTR_PARAMS) -> Object &
		{
			return *REFLEX_CREATE(TYPE);
		});
	}

	if constexpr (Reflex::Detail::kIsNullable <TYPE> && (kIsSingleThreadExclusive<TYPE> || kIsThreadSafe<TYPE>))
	{
		Detail::UseGlobalNull<TYPE>(type);
	}

	return type;
}
