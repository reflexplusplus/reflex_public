#pragma once

#include "detail/fnobject.h"
#include "program.h"




//
//Context

class Reflex::VM::Context :
	public Data::PropertySet,
	public Detail::Circular::TrackingToken::List
{
public:

	REFLEX_OBJECT(VM::Context, Data::PropertySet);

	static Context & null;

	struct Scope :
		public Reflex::Detail::ScopeOf <Context*,true>,
		public Reflex::Detail::ContextScope
	{
		Scope(Context & context);

		using Reflex::Detail::ScopeOf<Context*,true>::GetCurrent;
	};



	//info

	[[nodiscard]] static TRef <Context> Create(UInt16 contextid = Reflex::Detail::AcquireContextID());



	//types

	virtual Object * GetNullByRTTID(UInt32 type_id, Object * fallback) const = 0;



	//run once

	void Initialise(const Program & executable, Object & clientdata = Object::null);		//regular use USE THIS



	//call script functions
	
	template <class ... ARGS> void Call(const ScriptFunction & fn, const ARGS & ... args);

	template <class ... ARGS> void Call(const Detail::FnObject & fn, const ARGS & ... args);



	//info

	operator bool() const { return *program; }



	//ADVANCED

	virtual void InitialiseNulls(const Program & executable) = 0;	//ADVANCED create nulls etc

	virtual void InitialiseGlobals(Object & clientdata) = 0;		//ADVANCED runs global scope, required to initialise globals


	virtual bool DoCall(const ScriptFunction & scriptfunction) = 0;		//ADVANCED use helper functions


	virtual void AdoptCirculars(Context & context, Reflex::Detail::DynamicTypeRef object_t) = 0;



	const ConstTRef <Program> program;

	const TRef <Object> clientdata;

	Data::Archive workspace;	//any (non-calling) function can use this because contexts are single thread

	Detail::Stack stack;

	const Detail::Instruction * instructionptr;



protected:

	using Data::PropertySet::PropertySet;

};




//
//impl

REFLEX_NS(Reflex::VM)

inline UInt32 GetNumberArgs(Context & context)
{
	REFLEX_ASSERT(context.instructionptr->opcode == OPCODE(CallExternal));

	return context.instructionptr->param32;
}

inline const Data::Archive & GetFunctionData(Context & context)
{
	auto instructionptr = ++context.instructionptr;

	REFLEX_ASSERT(instructionptr->opcode == OPCODE(Data));

	return *ToPointer<Data::Archive>(UIntNative(instructionptr->param64));
}

template <class TYPE> inline const TYPE & ReadFunctionData(Context & context)
{
	return *Reinterpret<TYPE>(GetFunctionData(context).GetData());
}

template <class TYPE> REFLEX_INLINE ArrayView <TYPE> PopVaradic(Context & context, UInt head = 0)
{
	REFLEX_ASSERT(context.instructionptr->opcode == OPCODE(CallExternal));

	auto n = context.instructionptr->param32 - head;

	auto & stack = context.stack;

	stack.Shrink(n * sizeof(TYPE));

	return { Reinterpret<TYPE>(stack.GetData() + stack.GetSize()), n };
}

REFLEX_END

REFLEX_NS(Reflex::VM::Detail)

REFLEX_INLINE void Push(Stack & stack)
{
}

template <class P1, class... ARGS> REFLEX_INLINE void Push(Stack & stack, const P1 & p1, const ARGS & ... args)
{
	if constexpr (kSizeOf<P1>)
	{
		Push(stack, p1);

		Push(stack, args...);
	}
}

REFLEX_END

inline Reflex::VM::Context::Scope::Scope(Context & context)
	: ScopeOf<Context *, true>(&context),
	Reflex::Detail::ContextScope(context.GetContextID())
{
}

REFLEX_INLINE void Reflex::VM::Context::Initialise(const Program & executable, Object & clientdata)
{
	Scope scope(*this);

	InitialiseNulls(executable);

	InitialiseGlobals(clientdata);
}

template <class ... ARGS> REFLEX_INLINE void Reflex::VM::Context::Call(const ScriptFunction & fn, const ARGS & ... args)
{
	Detail::Push(stack, args...);

	DoCall(fn);
}

template <class ... ARGS> REFLEX_INLINE void Reflex::VM::Context::Call(const Detail::FnObject & fnobject, const ARGS & ... args)
{
	REFLEX_ASSERT(fnobject.GetRetainCount());

	Detail::Push(stack, args...);

	return fnobject(*this);
}

template <class TYPE> REFLEX_INLINE TYPE & Reflex::VM::Detail::FinaliseObject(TYPE & object, TypeRef type)
{
	Reflex::Detail::SetDynamicCastableTypeInfo(&object, *type->object_t);

	return object;
}

REFLEX_INLINE void Reflex::VM::Detail::FnObject::operator()(Context & current) const
{
	Reflex::Detail::ContextScope scope(current.GetContextID());

	Invoke(current);
}
