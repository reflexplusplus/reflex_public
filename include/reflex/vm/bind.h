#pragma once


#include "bind/bind.h"
#include "bind/bindtype.h"
#include "bind/binddefine.h"
#include "bind/template.h"
#include "bind/method.h"




//
//Experimental API

REFLEX_NS(Reflex::VM)

const ExternalFunction & AddFunction(Bindings & b, Key32 ns, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, ExternalFunctionPtr fnptr);

const ExternalFunction & AddMethod(Bindings & b, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, ExternalFunctionPtr fnptr);

const ExternalFunction & AddConstructor(Bindings & b, TypeRef type, const ArrayView <Argument> & arguments, ExternalFunctionPtr fnptr);


const ExternalFunction & AddFunction(Bindings & b, Key32 ns, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, const ArrayView <Argument> & targs, const Data::Archive::View & clientdata, ExternalFunctionPtr fnptr);

const ExternalFunction & AddMethod(Bindings & b, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, const ArrayView <Argument> & targs, const Data::Archive::View & clientdata, ExternalFunctionPtr fnptr);

const ExternalFunction & AddConstructor(Bindings & b, TypeRef type, const ArrayView <Argument> & arguments, const Data::Archive::View & clientdata, ExternalFunctionPtr fnptr);

REFLEX_END




//
//impl

inline const Reflex::VM::ExternalFunction & Reflex::VM::AddFunction(Bindings & b, Key32 ns, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, ExternalFunctionPtr fnptr)
{
	return b.RegisterFunction(ns, name, rtn, arguments, {}, {}, 0, fnptr);
}

inline const Reflex::VM::ExternalFunction & Reflex::VM::AddMethod(Bindings & b, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, ExternalFunctionPtr fnptr)
{
	return b.RegisterFunction(arguments[0].type->symbol.a, name, rtn, arguments, {}, {}, kMemberFunction, fnptr);
}

inline const Reflex::VM::ExternalFunction & Reflex::VM::AddConstructor(Bindings & b, TypeRef type, const ArrayView <Argument> & arguments, ExternalFunctionPtr fnptr)
{
	return b.RegisterFunction(type->symbol.a, Compiler::opCreate, type, arguments, { type }, {}, 0, fnptr);
}

inline const Reflex::VM::ExternalFunction & Reflex::VM::AddFunction(Bindings & b, Key32 ns, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, const ArrayView <Argument> & targs, const Data::Archive::View & clientdata, ExternalFunctionPtr fnptr)
{
	return b.RegisterFunction(ns, name, rtn, arguments, targs, clientdata, 0, fnptr);
}

inline const Reflex::VM::ExternalFunction & Reflex::VM::AddMethod(Bindings & b, StaticString name, const Argument & rtn, const ArrayView <Argument> & arguments, const ArrayView <Argument> & targs, const Data::Archive::View & clientdata, ExternalFunctionPtr fnptr)
{
	return b.RegisterFunction(arguments[0].type->symbol.a, name, rtn, arguments, targs, clientdata, kMemberFunction, fnptr);
}

inline const Reflex::VM::ExternalFunction & Reflex::VM::AddConstructor(Bindings & b, TypeRef type, const ArrayView <Argument> & arguments, const Data::Archive::View & clientdata, ExternalFunctionPtr fnptr)
{
	return b.RegisterFunction(type->symbol.a, Compiler::opCreate, type, arguments, { type }, clientdata, 0, fnptr);
}
