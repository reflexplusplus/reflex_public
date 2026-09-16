#pragma once

#include "tref.h"
#include "objectof.h"
#include "../typeid.h"




//
//Secondary API

namespace Reflex
{

	template <class TYPE> void PublishInterface(Object & owner, TYPE * address);

	template <class TYPE, class SELF> void PublishInterface(SELF && object);

	template <class TYPE> void RetractInterface(Object & owner, TYPE * address);


	template <class TYPE> TYPE * QueryInterface(Object & owner);

	template <class TYPE> const TYPE * QueryInterface(const Object & owner);


	template <class TYPE> AlreadyRetained <TYPE> GetInterface(Object & owner);					//requires TYPE::null

	template <class TYPE> ConstAlreadyRetained <TYPE> GetInterface(const Object & owner);		//requires TYPE::null

}




//
//InterfaceRef

REFLEX_NS(Reflex::Detail)

struct InterfaceRef : public Object
{
	REFLEX_OBJECT(InterfaceRef, Object);

	InterfaceRef(void * address);

	void * const address;
};

REFLEX_END




//
//impl

REFLEX_NS(Reflex)

struct AbstractInterface
{
public:

	const TypeID & type_id;			//reference because type_ids are set during static init



protected:

	AbstractInterface(const TypeID & type_id);

	~AbstractInterface();


	void Publish(Object & owner);	//typical usage: call in owner constructor

	void Retract();					//advanced, typically do not use



private:

	AbstractInterface(const AbstractInterface&) = delete;

	AbstractInterface & operator=(const AbstractInterface&) = delete;


	Object * m_owner;

	Detail::InterfaceRef m_property;
};

[[deprecated("use PublishInterface and RetractInterface")]] typedef AbstractInterface Interface;

template <class TYPE>
struct InterfaceOf : public AbstractInterface
{
protected:

	[[deprecated("use PublishInterface and RetractInterface")]] InterfaceOf()
		: AbstractInterface(Detail::TypeIndex<TYPE>::value)
	{
	}
};

REFLEX_END

REFLEX_NS(Reflex::Detail)

void PublishInterface(Object & owner, TypeID type_id, void * address);

void RetractInterface(Object & owner, TypeID type_id, void * address);

void UnsetInterface(Object & owner, TypeID type_id);

void * QueryInterface(Object & owner, TypeID type_id, void * fallback);

REFLEX_END

inline Reflex::Detail::InterfaceRef::InterfaceRef(void * address)
	: address(address)
{
	REFLEX_ASSERT(address);
}

template <class TYPE> inline void Reflex::PublishInterface(Object & owner, TYPE * address)
{
	Detail::PublishInterface(owner, GetTypeID<TYPE>(), address);
}

template <class TYPE, class SELF> inline void Reflex::PublishInterface(SELF && objectref)
{
	auto & obj = Deref(objectref);

	PublishInterface<TYPE>(obj, Cast<TYPE>(&obj));
}

template <class TYPE> inline void Reflex::RetractInterface(Object & owner, TYPE * address)
{
	Detail::RetractInterface(owner, GetTypeID<TYPE>(), address);
}

template <class TYPE> inline TYPE * Reflex::QueryInterface(Object & owner)
{
	return static_cast<TYPE*>(Detail::QueryInterface(owner, GetTypeID<TYPE>(), nullptr));
}

template <class TYPE> REFLEX_INLINE const TYPE * Reflex::QueryInterface(const Object & owner)
{
	return QueryInterface<TYPE>(RemoveConst(owner));
}

template <class TYPE> inline Reflex::AlreadyRetained <TYPE> Reflex::GetInterface(Object & owner)
{
	return static_cast<TYPE*>(Detail::QueryInterface(owner, GetTypeID<TYPE>(), &TYPE::null));
}

template <class TYPE> REFLEX_INLINE Reflex::ConstAlreadyRetained <TYPE> Reflex::GetInterface(const Object & owner)
{
	return GetInterface<TYPE>(RemoveConst(owner));
}
