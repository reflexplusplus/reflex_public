#pragma once

#include "tref.h"
#include "objectof.h"
#include "../typeid.h"




//
//Secondary API

namespace Reflex
{

	struct Interface;

	template <class TYPE> struct InterfaceOf;


	template <class TYPE> TYPE * QueryInterface(Object & owner);

	template <class TYPE> const TYPE * QueryInterface(const Object & owner);


	template <class TYPE> TRef <TYPE> GetInterface(Object & owner);					//requires TYPE::null

	template <class TYPE> ConstTRef <TYPE> GetInterface(const Object & owner);		//requires TYPE::null

}




//
//Interface

struct Reflex::Interface
{
public:

	const TypeID & type_id;			//reference because type_ids are set during static init



protected:

	Interface(const TypeID & type_id);

	~Interface();


	void Publish(Object & owner);	//typical usage: call in owner constructor

	void Retract();					//advanced, typically do not use



private:

	Interface(const Interface&) = delete;

	Interface & operator=(const Interface&) = delete;


	Object * m_owner;

	ObjectOf <Interface*> m_property;
};




//
//InterfaceOf 

template <class TYPE>
struct Reflex::InterfaceOf : public Interface
{
protected:

	InterfaceOf()
		: Interface(Detail::TypeIndex<TYPE>::value)
	{
	}
};




//
//impl

REFLEX_NS(Reflex::Detail)

using InterfaceRef = ObjectOf <Interface*>;

Interface * QueryInterface(Object & owner, TypeID type_id, Interface * fallback);

REFLEX_END

template <class TYPE> inline TYPE * Reflex::QueryInterface(Object & owner)
{
	return Cast<TYPE>(Detail::QueryInterface(owner, GetTypeID<TYPE>(), nullptr));
}

template <class TYPE> REFLEX_INLINE const TYPE * Reflex::QueryInterface(const Object & owner)
{
	return QueryInterface<TYPE>(RemoveConst(owner));
}

template <class TYPE> inline Reflex::TRef <TYPE> Reflex::GetInterface(Object & owner)
{
	return Cast<TYPE>(Detail::QueryInterface(owner, GetTypeID<TYPE>(), &TYPE::null));
}

template <class TYPE> REFLEX_INLINE Reflex::ConstTRef <TYPE> Reflex::GetInterface(const Object & owner)
{
	return GetInterface<TYPE>(RemoveConst(owner));
}
