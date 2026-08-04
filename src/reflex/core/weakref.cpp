#include "../../../include/reflex/core/object/detail/weakref.h"




//
//impl

Reflex::Detail::AbstractWeakRef::AbstractWeakRef(DynamicTypeRef object_t, Object & null, Object & init_target)
	: m_object_t(object_t)
	, m_null(&null)
	, m_target(&init_target)
{
	REFLEX_ASSERT(CheckObjectType(init_target, m_object_t));
}

void Reflex::Detail::AbstractWeakRef::Clear()
{
	Detach();
}

bool Reflex::Detail::AbstractWeakRef::Store(Object & target)
{
	REFLEX_ASSERT(CheckObjectType(target, m_object_t));

	if (SetFiltered(m_target, &target))
	{
		auto list = AcquireProperty<List>(target, kWeakReferences, &target);

		REFLEX_ASSERT(list->owner == &target);	//this must not be shared

		if (list->owner == &target)
		{
			Attach(list);

			m_target = &target;
		}

		return true;
	}
	else
	{
		return false;
	}
}

Reflex::TRef <Reflex::Object> Reflex::Detail::AbstractWeakRef::Load() const
{
	if (CheckObjectType(*m_target, m_object_t))
	{
		return *m_target;
	}
	else
	{
		return *m_null;
	}
}

void Reflex::Detail::AbstractWeakRef::OnDetach(Item::List & list)
{
	m_target = m_null;
}
