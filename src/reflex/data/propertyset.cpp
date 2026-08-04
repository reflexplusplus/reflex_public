#include "../../../include/reflex/data/propertyset.h"




//
//implementation

REFLEX_BEGIN_INTERNAL(Reflex::Data)

Array <Address> GetKeys(const Data::PropertySet::SequenceType & properties)
{
	Array <Address> uids;

	auto ptr = Extend(uids, properties.GetSize()).data;

	for (auto & i : properties) *ptr++ = i.key;

	return uids;
}

REFLEX_END_INTERNAL

Reflex::Data::PropertySet::PropertySet(UInt16 contextid, Reflex::Allocator & allocator)
	: Object(contextid),
	m_properties(allocator)
{
}

Reflex::Data::PropertySet::PropertySet(Reflex::Allocator & allocator)
	: m_properties(allocator)
{
}

Reflex::Data::PropertySet::PropertySet(const PropertySet & value, Reflex::Allocator & allocator)
	: m_properties(value.m_properties, allocator)
{
}

Reflex::Data::PropertySet::PropertySet(PropertySet && rhs)
{
	m_properties.Swap(rhs.m_properties);
}

Reflex::Data::PropertySet::~PropertySet()
{
	Detail::DestroyProperties(m_properties);
}

void Reflex::Data::PropertySet::UnsetAll(TypeID type_id)
{
	if (auto range = Iterate(type_id))
	{
		Array <Address> uids(range.GetSize());

		auto puid = uids.GetData();

		for (auto & i : range) *puid++ = i.key;

		for (auto & i : uids) OnUnsetProperty(i);
	}
}

void Reflex::Data::PropertySet::Clear()
{
	if (m_properties)
	{
		auto keys = GetKeys(m_properties);

	#if REFLEX_DEBUG
		REFLEX_RFOREACH(i, keys) OnUnsetProperty(i);
	#else
		for (auto & i : keys) OnUnsetProperty(i);
	#endif
	}
}

Reflex::Data::PropertySet & Reflex::Data::PropertySet::operator=(const PropertySet & value)
{
	Clear();

	for (auto & i : value.m_properties) OnSetProperty(i.key, Copy(i.value));

	return *this;
}

Reflex::Data::PropertySet & Reflex::Data::PropertySet::operator=(PropertySet && rhs)
{
	m_properties.Swap(rhs.m_properties);

	return *this;
}

Reflex::Sequence < Reflex::Address, Reflex::Reference <Reflex::Object> >::Range Reflex::Data::PropertySet::Iterate(TypeID type_id) const
{
	return MakeRange(m_properties, { kZeroKey, type_id }, { kZeroKey, type_id + 1 });
}

void Reflex::Data::PropertySet::OnUnsetProperty(Address address)
{
	Detail::ClearObject(m_properties, address);
}

void Reflex::Data::PropertySet::OnSetProperty(Address address, Object & object)
{
	REFLEX_HEAPCHECK(File::output, this, OnSetProperty, object);

	Detail::SetObject(m_properties, address, object);
}

void Reflex::Data::PropertySet::OnQueryProperty(Address address, Object * & fallback) const
{
	REFLEX_STATIC_ASSERT(sizeof(TRef<Object>) == sizeof(Reference<Object>));
	REFLEX_STATIC_ASSERT(sizeof(TRef<Object>) == sizeof(Object*));

	Detail::QueryObject(m_properties, address, fallback);
}

void Reflex::Data::PropertySet::OnReleaseData()
{
	Clear();
}

void Reflex::Data::Assimilate(Data::PropertySet & self, const Data::PropertySet & src)
{
	for (auto & i : src.Iterate())
	{
		self.SetProperty(i.key, i.value);
	}
}
