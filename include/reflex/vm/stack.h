#pragma once

#include "detail/stack.h"




//
//Experimental API

#define VM_POP_SAFE(...) auto args = VM::Pop<__VA_ARGS__>(context.stack)	//copies args, safe for re-entrant
#define VM_POP(...) auto & args = VM::Pop<__VA_ARGS__>(context.stack)	//ONLY USE when the function is not re-entrant, i.e. will NOT MODIFY THE STACK
#define VM_POP1(TYPE) auto & arg = VM::Detail::Pop<TYPE>(context.stack)
#define VM_RTN(rtn) VM::Detail::Push(context.stack, rtn)

namespace Reflex::VM
{

	#pragma pack (push,1)

	template <class ... VARGS> struct StackArgsImpl {};

	template <class A, class B, class C, class D, class E, class F>
	struct StackArgsImpl <A,B,C,D,E,F>
	{
		VM_CHECK_OBJECT(A);
		VM_CHECK_OBJECT(B);
		VM_CHECK_OBJECT(C);
		VM_CHECK_OBJECT(D);
		VM_CHECK_OBJECT(E);
		VM_CHECK_OBJECT(F);

		static const UInt size = sizeof(RefToPtr<A>) + sizeof(RefToPtr<B>) + sizeof(RefToPtr<C>) + sizeof(RefToPtr<D>) + sizeof(RefToPtr<E>) + sizeof(RefToPtr<F>);

		A a;
		B b;
		C c;
		D d;
		E e;
		F f;
	};

	template <class A, class B, class C, class D, class E>
	struct StackArgsImpl <A,B,C,D,E>
	{
		VM_CHECK_OBJECT(A);
		VM_CHECK_OBJECT(B);
		VM_CHECK_OBJECT(C);
		VM_CHECK_OBJECT(D);
		VM_CHECK_OBJECT(E);

		static const UInt size = sizeof(RefToPtr<A>) + sizeof(RefToPtr<B>) + sizeof(RefToPtr<C>) + sizeof(RefToPtr<D>) + sizeof(RefToPtr<E>);

		A a;
		B b;
		C c;
		D d;
		E e;
	};

	template <class A, class B, class C, class D>
	struct StackArgsImpl <A,B,C,D>
	{
		VM_CHECK_OBJECT(A);
		VM_CHECK_OBJECT(B);
		VM_CHECK_OBJECT(C);
		VM_CHECK_OBJECT(D);

		static const UInt size = sizeof(RefToPtr<A>) + sizeof(RefToPtr<B>) + sizeof(RefToPtr<C>) + sizeof(RefToPtr<D>);

		A a;
		B b;
		C c;
		D d;
	};

	template <class A, class B, class C>
	struct StackArgsImpl <A,B,C>
	{
		VM_CHECK_OBJECT(A);
		VM_CHECK_OBJECT(B);
		VM_CHECK_OBJECT(C);

		static const UInt size = sizeof(RefToPtr<A>) + sizeof(RefToPtr<B>) + sizeof(RefToPtr<C>);

		A a;
		B b;
		C c;
	};

	template <class A, class B>
	struct StackArgsImpl <A,B>
	{
		VM_CHECK_OBJECT(A);
		VM_CHECK_OBJECT(B);

		static const UInt size = sizeof(RefToPtr<A>) + sizeof(RefToPtr<B>);

		A a;
		B b;
	};

	template <class A>
	struct StackArgsImpl <A>
	{
		VM_CHECK_OBJECT(A);

		static const UInt size = sizeof(RefToPtr<A>);

		A a;
	};
	#pragma pack (pop)

	template <class ... ARGS> using StackArgs = StackArgsImpl<ARGS...>;

	template <typename First, typename... Rest> using FirstArgType = First;

	template <class ... ARGS> inline auto & Pop(Detail::Stack & stack)
	{
		using Args = StackArgs <ARGS...>;

		REFLEX_STATIC_ASSERT(sizeof...(ARGS) != 1);

		return *Reinterpret<Args>(Detail::Pop(stack, Args::size));
	}

	inline Argument ByRef(TypeRef type)
	{
		return {type, true};
	}

};
