
#include "vm/bindings.cpp"
#include "vm/contextimpl.cpp"
#include "vm/scriptobject.cpp"
#include "vm/fnobjectimpl.cpp"
#include "vm/mt.cpp"

#include "vm/library.cpp"

#include "vm/compiler.cpp"
#include "vm/optimiser.cpp"

#include "vm/programimpl.cpp"
#include "vm/list.cpp"

#include "vm/bindings/core.cpp"
#include "vm/bindings/system.cpp"
#include "vm/bindings/data.cpp"
#include "vm/bindings/file.cpp"
#include "vm/bindings/vm.cpp"




//
//glx bridge

REFLEX_NS(Reflex::VM)

const UInt8 * CastBool(const Object & object)
{
	if (auto p = DynamicCast< ObjectOf <UInt8> >(object))
	{
		return &p->value;
	}

	return 0;
}

REFLEX_END




//
// TODO
//

//
// REFACTOR Parse Namespaced Symbol
//
// unified call inside ParseExpression, which looks for: [function (script,opcode,external) | variable | global const/extern | type]
// iterates up thru namespace scope and looks for all of the above
// this can allow templated subclasses to be resolved
// also needs to allow for ::Object for global
// if finding a type, but then next token is ::, it must push the class ns, and continue parsing

//FIX
//FIX bug that Array::nudge can result in no data left, so that access[0] can crash!!!
//always allocate 1 more than size, so that size = 0, wrap = 1 will always work

//TODO
//Array SetBounds(1, 4)	-> solves Search and solves the waveform case
//Array Append

//CLEANUP
//get rid Allocation as public type
//internally, only need ValueAllocation and ObjectAllocation


// OPTIMISATION FUNCTION INLINING

// STATIC variables (naemspace / class scope)
// need to use resolve symbol mechanism on variables
// and scoped variables need to be symbol not just name OR somehow have to keep namespace scopes on stack (but then it wont be a list, would be a tree)


//
// named constructors
//

//
// typed functions
//

//template <TYPE> join(any, any)
//{
//
//}


//
//optimisations -> basic
//
// cojoin adjacent push'es (for example push local a and push local b -> one push64
//
// cojoin intrinsicNullObject + intrinsicInequalObject -> if (object) {}
//
// assign with no pop for assign + push (store pop size in flags, so can be zero)
//
// not and logic optimisations (not + jumpiffalse => jump if true)
//
// function inlining
//
// copy ellision -> when a variable is referenced only once, and copied, it can be removed
//
//
