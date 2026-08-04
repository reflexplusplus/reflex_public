#pragma once

#include "../../style.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

template <class RANGE, class TYPE> auto * GetObjectByID(RANGE && iterable, Key32 id, TYPE * null = nullptr);

template <class RANGE> Object * GetObjectByClass(RANGE && iterable, Reflex::Detail::DynamicTypeRef classinfo, Object * null = nullptr);

template <class TYPE> inline TYPE * SearchInheritedProperty(Object & object, Key32 id);

const Style * GetChildStyle(const Style & parent, Key32 id, const Style * null = nullptr);

REFLEX_END




//
//impl

template <class RANGE, class TYPE> inline auto * Reflex::GLX::Detail::GetObjectByID(RANGE && range, Key32 id, TYPE * null)
{
	for (auto & i : range)
	{
		if (i.id == id) return &i;
	}

	return null;
}

template <class RANGE> inline Reflex::GLX::Object * Reflex::GLX::Detail::GetObjectByClass(RANGE && range, Reflex::Detail::DynamicTypeRef object_t, Object * null)
{
	for (auto & i : range)
	{
		if (CheckObjectType(i, object_t)) return &i;
	}

	return null;
}

template <class TYPE> inline TYPE * Reflex::GLX::Detail::SearchInheritedProperty(Object & object, Key32 id)
{
	for (auto & i : Object::ParentRange(object))
	{
		if (auto rtn = i.QueryProperty<TYPE>(id)) return rtn;
	}

	return Object::null.QueryProperty<TYPE>(id);
}

inline const Reflex::GLX::Style * Reflex::GLX::Detail::GetChildStyle(const Style & parent, Key32 id, const Style * null)
{
	return Detail::GetObjectByID(Style::ItemRange(RemoveConst(parent)), id, RemoveConst(null));
}
