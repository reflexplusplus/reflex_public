#pragma once

#include "forward.h"



REFLEX_NS(Reflex::VM::Detail)

using Stack = Array <UInt8>;

using Pointer = Object *;


template <class TYPE> TYPE & Push(Stack & stack);

template <class TYPE> void Push(Stack & stack, const TYPE & value);

template <class TYPE> inline void Push(Stack & stack, TRef <TYPE> value) { Push(stack, *value); }


template <class TYPE, bool POP = true> TYPE & Pop(Stack & stack);

UInt8 * Pop(Stack & stack, UInt n);

REFLEX_END




//
//

#define VM_ISOBJECT(TYPE) IsObject<TYPE>::value

#define VM_CHECK_OBJECT(TYPE) REFLEX_STATIC_ASSERT(VM_ISOBJECT(NonConstT < NonRefT <TYPE> >) ? (IsReference<TYPE>::value || IsPointer<TYPE>::value) : true)

template <class TYPE> REFLEX_INLINE void Reflex::VM::Detail::Push(Stack & stack, const TYPE & value)
{
	REFLEX_STATIC_ASSERT(kSizeOf<TYPE>);

	REFLEX_ASSERT(stack.GetCapacity() > stack.GetSize() + sizeof(TYPE));

	if constexpr (IsObject<TYPE>::value && !IsPointer<TYPE>::value)
	{
		auto top = Extend<kAllocateNone>(stack, sizeof(void *)).data;

		*Reinterpret<UIntNative>(top) = ToUIntNative(&value);
	}
	else if constexpr (IsPointer<TYPE>::value)
	{
		auto top = Extend<kAllocateNone>(stack, sizeof(void *)).data;

		*Reinterpret<UIntNative>(top) = ToUIntNative(value);
	}
	else if constexpr (kSizeOf<TYPE> == 1)
	{
		stack.Push<kAllocateNone>(reinterpret_cast<const UInt8 &>(value));
	}
	else
	{
		*Reinterpret<TYPE>(Extend<kAllocateNone>(stack, sizeof(TYPE)).data) = value;
	}
}

template <class TYPE> struct ReferenceToPointerImpl
{
	using Type = TYPE;

	static TYPE & Cast(Reflex::UInt8 * bytes)
	{
		return *reinterpret_cast<TYPE*>(bytes);
	}
};

template <class TYPE> struct ReferenceToPointerImpl <TYPE&>
{
	using Type = TYPE *;

	static TYPE & Cast(Reflex::UInt8 * bytes)
	{
		return **reinterpret_cast<TYPE**>(bytes);
	}
};

template <class TYPE> using RefToPtr = typename ReferenceToPointerImpl<TYPE>::Type;

template <class TYPE, bool POP> REFLEX_INLINE TYPE & Reflex::VM::Detail::Pop(Stack & stack)
{
	VM_CHECK_OBJECT(TYPE);

	using Convert = ReferenceToPointerImpl <TYPE>;

	using Type = typename Convert::Type;

	REFLEX_ASSERT(stack.GetSize() >= kSizeOf<Type>);

	if constexpr (POP)
	{
		stack.Shrink(kSizeOf<Type>);	//safe because no realloc on size down

		return Convert::Cast(stack.GetData() + stack.GetSize());
	}
	else
	{
		return Convert::Cast(stack.GetData() + stack.GetSize() - kSizeOf<Type>);
	}
}

REFLEX_INLINE Reflex::UInt8 * Reflex::VM::Detail::Pop(Stack & stack, UInt n)
{
	REFLEX_ASSERT(stack.GetSize() >= n);

	stack.Shrink(n);

	return stack.GetData() + stack.GetSize();
}


