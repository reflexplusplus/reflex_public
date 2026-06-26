#pragma once

#include "[require].h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

enum PropertyType
{
	kPropertyTypeBool,
	kPropertyTypeKey,
	kPropertyTypeFloat,
	kPropertyTypeInt,
	kPropertyTypeSize,
	kPropertyTypePoint,
	kPropertyTypeMargin,
	kPropertyTypeColour,

	kPropertyTypeImage,
	kPropertyTypeImageSet,
};

typedef Pair <PropertyType,Key32> PropertyDef;

template <class TYPE> inline ObjectType <TYPE> * GetNull()
{
	typedef ObjectType <TYPE> Type;

	return &REFLEX_NULL(Type);
}

REFLEX_END_INTERNAL
