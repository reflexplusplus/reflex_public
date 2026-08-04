#pragma once

#include "../traits.h"
#include "../tref.h"
#include "../functions/null.h"
#include "../../list.h"
#include "../../string/hash.h"




//
//Detail

namespace Reflex::Detail
{

	class AbstractWeakRef;


	template <class TYPE> class WeakRef;

	template <class TYPE> using ConstWeakRef = WeakRef <const TYPE>;

}




//
//Detail::AbstractWeakRef 

class Reflex::Detail::AbstractWeakRef : public Item <AbstractWeakRef,false>
{
public:

	REFLEX_NONCOPYABLE(AbstractWeakRef);

	static constexpr UInt32 kWeakReferences = MakeKey32("WeakReferences");

	struct List;



	AbstractWeakRef(DynamicTypeRef object_t, Object & null, Object & init_target);

	void Clear();

	bool Store(Object & target);

	TRef <Object> Load() const;


	bool Compare(const Object * adr) const { return m_target == adr; }	//fast compare on cached adr, object may have died



private:

	friend Item;

	void OnDetach(Item::List & list);


	const DynamicTypeRef m_object_t;

	Object * const m_null;

	Object * m_target;
};




//
//WeakRef

template <class TYPE>
class Reflex::Detail::WeakRef : public Detail::AbstractWeakRef
{
public:

	using Base = Detail::AbstractWeakRef;

	REFLEX_STATIC_ASSERT(kIsNullable<NonConstT<TYPE>>);

	
	WeakRef() : Base(TYPE::kDynamicTypeInfo, GetNullInstance<NonConstT<TYPE>>(), GetNullInstance<NonConstT<TYPE>>()) {}

	WeakRef(TYPE & target) : Base(TYPE::kDynamicTypeInfo, GetNullInstance<NonConstT<TYPE>>(), target) {}

	WeakRef(const WeakRef & weakref) = delete;

	WeakRef(WeakRef && weakref) = delete;


	using Base::Clear;

	bool Store(TYPE & ref) { return Base::Store(RemoveConst(ref)); }

	TRef <TYPE> Load() const { return Cast<TYPE>(Base::Load()); }


	WeakRef & operator=(const WeakRef & weakref) = delete;

	WeakRef & operator=(WeakRef && weakref) = delete;

};




//
//impl

struct Reflex::Detail::AbstractWeakRef::List :
	public Object,
	public Item <AbstractWeakRef, false>::List
{
	List(Object * owner)
		: owner(owner)
	{
	}

	using Item <AbstractWeakRef,false>::List::Clear;

	const Object * owner;
};
