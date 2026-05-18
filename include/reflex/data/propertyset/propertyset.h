#pragma once

#include "../types.h"




//
//Primary API

namespace Reflex::Data
{

	class PropertySet;

}




//
//PropertySet

class Reflex::Data::PropertySet : public Object
{
public:

	REFLEX_OBJECT(Data::PropertySet, Object);

	static PropertySet & null;



	//types

	template <class TYPE> using PropertyIterator = typename Sequence < Address, Reference <TYPE> >::Range;

	using SequenceType = Sequence < Address, Reference <Object> >;



	//lifetime

	PropertySet(UInt16 contextid, Allocator & allocator = g_default_allocator);

	PropertySet(Allocator & allocator = g_default_allocator);

	PropertySet(const PropertySet & value, Allocator & allocator = g_default_allocator);

	PropertySet(PropertySet && rhs);

	~PropertySet();



	//primary access

	template <class TYPE> PropertyIterator <TYPE> Iterate() const;



	//secondard/advanced (primarily for VM)

	void UnsetAll(TypeID type_id);


	PropertyIterator <Object> Iterate() const { return m_properties; }

	PropertyIterator <Object> Iterate(TypeID type_id) const;



	//assign

	PropertySet & operator=(const PropertySet & value);

	PropertySet & operator=(PropertySet && rhs);



	//operator

	bool Empty() const;

	explicit operator bool() const;



protected:

	void Clear();


	void OnUnsetProperty(Address address) override;

	void OnSetProperty(Address adr, Object & object) override;

	void OnQueryProperty(Address address, Object * & pointer) const override;


	void OnReleaseData() override;


	mutable SequenceType m_properties;



private:
	[[deprecated]] virtual void Store(Data::Archive&) const final {}	//prevent use old API
	[[deprecated]] virtual void Restore(Data::Archive::View&) final {}
};

REFLEX_SET_TRAIT(Data::PropertySet, IsBoolCastable);




//
//impl

REFLEX_NS(Reflex::Data::Detail)

REFLEX_INLINE void DestroyProperties(PropertySet::SequenceType & properties)
{
	PropertySet::SequenceType temp(std::move(properties));	//prevent clients accessing during destruction

	REFLEX_ASSERT(!properties);	//check nothing was added during destruction (callbacks in destructors)
}

REFLEX_INLINE void ClearObject(PropertySet::SequenceType & properties, Address address)
{
	if (auto index = properties.Search(address))
	{
		auto temp = properties[index.value].value;

		properties.Remove(index.value);
	}
}

REFLEX_INLINE void SetObject(PropertySet::SequenceType & properties, Address address, Object & object)
{
	properties.Set(address, object);
}

REFLEX_INLINE void QueryObject(PropertySet::SequenceType & properties, Address address, Object * & fallback)
{
	using Opaque = Sequence < Address, Object* >;

	fallback = *Reinterpret<Opaque>(properties).SearchValue(address, &fallback);
}

REFLEX_END

template <class TYPE> REFLEX_INLINE typename Reflex::Sequence < Reflex::Address, Reflex::Reference<TYPE> >::Range Reflex::Data::PropertySet::Iterate() const
{
	REFLEX_STATIC_ASSERT_OBJECT_TYPE(TYPE);

	using Typed = Sequence < Address, Reference <TYPE> >;

	return Reinterpret<typename Typed::Range>(Iterate(GetTypeID<TYPE>()));
}

REFLEX_INLINE bool Reflex::Data::PropertySet::Empty() const
{
	return m_properties.Empty();
}

REFLEX_INLINE Reflex::Data::PropertySet::operator bool() const
{
	return True(m_properties);
}
