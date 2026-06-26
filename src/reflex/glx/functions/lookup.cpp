#include "../../../../include/reflex/glx/functions/lookup.h"




//
//

Reflex::GLX::Object * Reflex::GLX::Detail::QueryParentByType(Object & object, Reflex::Detail::DynamicTypeRef classinfo, Object * fallback)
{
	return GetObjectByClass(Object::ParentRange(*object.GetParent()), classinfo, fallback);
}

Reflex::GLX::Object * Reflex::GLX::Detail::QueryChildByType(Object & object, Reflex::Detail::DynamicTypeRef classinfo, Object * fallback)
{
	return GetObjectByClass(Object::ItemRange(object), classinfo, fallback);
}

Reflex::GLX::Object * Reflex::GLX::Detail::QueryElementByType(Object & object, Reflex::Detail::DynamicTypeRef classinfo, Object * fallback)
{
	return GetObjectByClass(Object::BranchIterator(object), classinfo, fallback);
}

Reflex::GLX::Object * Reflex::GLX::QueryElementById(Object & object, Key32 id, Object * null)
{
	return Detail::GetObjectByID(Object::BranchIterator(object), id, null);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::LookupChildAtIndex(Object & parent, UInt idx)
{
	if (idx < parent.GetNumItem())
	{
		auto itr = parent.GetFirst();

		while (idx--) itr = itr->GetNext();

		return itr;
	}

	return {};
}

Reflex::Idx Reflex::GLX::LookupIndex(const Object & child)
{
	return Reflex::LookupIndex(child);
}

bool Reflex::GLX::BranchContains(const Object & parent, const Object & child)
{
	return Reflex::BranchContains(parent, child);
}

Reflex::Idx Reflex::GLX::LookupBranchIndex(const Object & parent, const Object & child)
{
	return Reflex::LookupBranchIndex(parent, child);
}

Reflex::Pair <Reflex::GLX::Point,Reflex::GLX::Scale> Reflex::GLX::CalculateAbs(const Object & object)
{
	auto pos = object.GetRect().origin;

	auto scale = object.GetScale();

	auto itr = object.GetParent();

	while (itr)
	{
		Scale t = itr->GetScale();

		pos *= t;

		pos += itr->GetRect().origin;

		scale *= t;

		itr = itr->GetParent();
	}

	return { pos, scale };
}

Reflex::Pair <Reflex::GLX::Point,Reflex::GLX::Scale> Reflex::GLX::CalculateAbs(const Object & parent, const Object & object)
{
	auto pos = object.GetRect().origin;

	auto object_scale = object.GetScale();

	auto scale = object_scale;

	auto itr = object.GetParent();

	while (itr != parent)
	{
		if (itr)
		{
			Scale t = itr->GetScale();

			pos *= t;

			pos += itr->GetRect().origin;

			scale *= t;

			itr = itr->GetParent();
		}
		else
		{
			return { kOrigin, object_scale };
		}
	}

	return { pos, scale };
}
