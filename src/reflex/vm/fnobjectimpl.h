#pragma once

#include "compiler/iprogram.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::VM::Detail)

struct FnObjectImpl : public FnObject
{
	struct Type : public VM::Type
	{
		Type(Bindings & bindings, CString::View name, bool noncircular, TypeRef rtn_t, const ArrayView <TypeRef> & args);

		TypeRef rtn_t;

		Array <TypeRef> args;

		UInt16 arguments_size;
	};


	static TRef <FnObjectImpl> Create(Context & context, const Type & type, ContainerType container, const Function & function, const Pair <UInt,LayoutTemplate> & captures);

	static TRef <FnObjectImpl> CreateContextSafe(Context & ownercontext, const Type & type, Context & fncontext, const ScriptFunction & function);	//for thread


	FnObjectImpl(Context & context, const Type & type, const Function * pfunction);

	virtual void CallInternal(ContextImpl & context) const = 0;

	virtual FnObjectImpl * CrossContextClone(Context & to, const Type & type, bool mt) = 0;



protected:

	template <ContainerType TYPE> struct Captures;

	template <bool INTERNAL, ContainerType TYPE> static void CrossContextCall(Context & context, const FnObjectImpl & impl, const Captures <TYPE> & captures);

	template <bool MT> static Object * ContextCopyFnObject(Context & to, Object & fnobject, TypeRef type, bool mt);


	const Type & type;

	const Function * pfunction;
};

REFLEX_END_INTERNAL
