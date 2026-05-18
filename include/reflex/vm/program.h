#pragma once

#include "detail/stack.h"
#include "detail/type.h"
#include "detail/circular.h"
#include "bindings.h"
#include "library.h"




//
//Experimental API

namespace Reflex::VM
{

	class Program;	//owns bindings

	class Context;	//owns globals & stack

	class String;

}




//
//Program

class Reflex::VM::Program : public Reflex::Object
{
public:

	REFLEX_OBJECT(VM::Program, Object);

	static Program & null;



	//types

	struct Source
	{
		Reference <String> path;

		WString::View pathview;

		Address address;

		Reference <Object> object;
	};



	//lifetime

	[[nodiscard]] static TRef <Program> Create(const Bindings & bindings);



	//lookup

	virtual Sequence<UInt64,ScriptFunction>::ConstRange GetScriptFunctions(Symbol symbol) const = 0;



	//info

	virtual operator bool() const = 0;



	//links

	const ConstTRef <Bindings> bindings;

	const Array <Source> sources;



protected:

	Program(const Bindings & bindings);

};




REFLEX_NS(Reflex::VM::Detail)

class FnObject;

template <class TYPE> TYPE & FinaliseObject(TYPE & object, TypeRef type);

REFLEX_INLINE TRef <Object> CreateObject(VM_CTR_PARAMS)
{
	return type->ctr(context, type);
}

REFLEX_INLINE Object & GetNull(VM_CTR_PARAMS)
{
	return type->null(context, type);
}

REFLEX_END
