#include "../../../include/reflex/core/object/interface.h"




//
//

Reflex::Interface::Interface(const TypeID & type_id)
	: type_id(type_id)
	, m_owner(nullptr)
	, m_property(this)
{
}

Reflex::Interface::~Interface()
{
	Retract();
}

void Reflex::Interface::Publish(Object & owner)
{
	Retract();

	REFLEX_ASSERT(!m_property.GetRetainCount());	//must not be shared

	owner.SetProperty(type_id, m_property);

	if (m_property.GetRetainCount() == 1)
	{
		m_owner = &owner;
	}
	else
	{
		//owner did not store the InterfaceRefProperty

		REFLEX_ASSERT(false);
	}
}

void Reflex::Interface::Retract()
{
	if (m_owner)
	{
		m_owner->UnsetProperty({ .id = type_id, .type_id = Detail::TypeIndex<Detail::InterfaceRef>::value });

		m_owner = nullptr;
	}
}

Reflex::Interface * Reflex::Detail::QueryInterface(Object & owner, TypeID type_id, Interface * fallback)
{
	if (auto pinterface_ref = owner.QueryProperty<Detail::InterfaceRef>(type_id))
	{
		return pinterface_ref->value;
	}
	else
	{
		return fallback;
	}
}
