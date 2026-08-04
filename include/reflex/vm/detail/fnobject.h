#pragma once

#include "../bindings.h"



//
//

REFLEX_NS(Reflex::VM::Detail)

class FnObject;

REFLEX_END





//
//

class Reflex::VM::Detail::FnObject : public Object
{
public:

	static TypeRef RegisterType(Bindings & bindings, StaticString name, const ArrayView <TypeRef> & targs);

	[[nodiscard]] static TRef <FnObject> CreateExternalMethod(Context & context, TypeRef fnobject_t, TypeID object_t, TRef <Object> object, const FunctionPointer <void(Object&,Context&)> & function);

	[[nodiscard]] static TRef <FnObject> CreateExternalMethod(Context & context, TypeRef fnobject_t, TRef <Object> object, const FunctionPointer <void(Object &,Context&)> & function);



	//invoke

	void operator()(Context & context) const;


	//context

	Context & context;



protected:

	FnObject(Context & context);

	virtual void Invoke(Context & context) const = 0;

};

REFLEX_SET_TRAIT(VM::Detail::FnObject, IsSingleThreadExclusive);

inline Reflex::TRef <Reflex::VM::Detail::FnObject> Reflex::VM::Detail::FnObject::CreateExternalMethod(Context & context, TypeRef fnobject_t, TRef <Object> object, const FunctionPointer <void(Object&,Context&)> & function)
{
	return CreateExternalMethod(context, fnobject_t, object->object_t->type_id, object, function);
}

