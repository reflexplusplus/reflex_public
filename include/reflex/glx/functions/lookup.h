#pragma once

#include "../detail/functions/lookup.h"




//
//

namespace Reflex::GLX
{

	Object * QueryChildById(Object & parent, Key32 id, Object * null = nullptr);

	Object * QueryElementById(Object & parent, Key32 id, Object * null = nullptr);


	template <class TYPE> TYPE * QueryParentByType(Object & child, TYPE * null = nullptr);

	template <class TYPE> TYPE * QueryChildByType(Object & parent, TYPE * null = nullptr);

	template <class TYPE> TYPE * QueryElementByType(Object & parent, TYPE * null = nullptr);


	TRef <Object> LookupChildAtIndex(Object & parent, UInt idx);

	Idx LookupIndex(const Object & child);


	bool BranchContains(const Object & parent, const Object & child);

	Idx LookupBranchIndex(const Object & parent, const Object & child);


	Pair <Point,Scale> CalculateAbs(const Object & object);

	Rect CalculateAbsoluteRect(const Object & object);


	Pair <Point,Scale> CalculateAbs(const Object & parent, const Object & object);

	Rect CalculateRelativeRect(const Object & parent, const Object & object);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

Object * QueryParentByType(Object & child, Reflex::Detail::DynamicTypeRef classid, Object * fallback);

Object * QueryChildByType(Object & child, Reflex::Detail::DynamicTypeRef classid, Object * fallback);

Object * QueryElementByType(Object & child, Reflex::Detail::DynamicTypeRef classid, Object * fallback);

REFLEX_END

inline Reflex::GLX::Object * Reflex::GLX::QueryChildById(Object & object, Key32 id, Object * null)
{
	return Detail::GetObjectByID(Object::ItemRange(object), id, null);
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::GLX::QueryParentByType(Object & object, TYPE * null)
{
	return Cast<TYPE>(Detail::QueryParentByType(object, TYPE::kDynamicTypeInfo, null));
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::GLX::QueryChildByType(Object & object, TYPE * null)
{
	return Cast<TYPE>(Detail::QueryChildByType(object, TYPE::kDynamicTypeInfo, null));
}

template <class TYPE> REFLEX_INLINE TYPE * Reflex::GLX::QueryElementByType(Object & object, TYPE * null)
{
	return Cast<TYPE>(Detail::QueryElementByType(object, TYPE::kDynamicTypeInfo, null));
}

inline Reflex::GLX::Rect Reflex::GLX::CalculateAbsoluteRect(const Object & object)
{
	auto [position, scale] = CalculateAbs(object);

	return { position, object.GetRect().size * scale };
}

inline Reflex::GLX::Rect Reflex::GLX::CalculateRelativeRect(const Object & parent, const Object & object)
{
	auto [position, scale] = CalculateAbs(parent, object);

	return { position, object.GetRect().size * scale };
}
