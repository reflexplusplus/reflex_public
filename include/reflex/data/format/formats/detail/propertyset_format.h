#pragma once

#include "../../streamable_format.h"




//
//Detail

REFLEX_NS(Reflex::Data::Detail)

struct PropertySetFormat : public SerializableFormat
{
	REFLEX_OBJECT(PropertySetFormat, SerializableFormat);

	struct TypeHandler
	{
		const TypeID & type;

		UInt8 persistentid;

		const char * desc;

		FunctionPointer <void(const SerializableFormat & format, Archive &, const Object &)> store;

		FunctionPointer <TRef<Object>(const SerializableFormat & format, Archive::View &)> restore;

		FunctionPointer <bool(const PropertySetFormat & format, const Object & a, const Object & b)> compare;

		FunctionPointer <TRef<Object>()> null;
	};

	[[nodiscard]] static TRef <PropertySetFormat> Create(UInt32 magic);

	[[nodiscard]] virtual TRef <PropertySetFormat> Clone(UInt32 magic) const = 0;

	virtual void SetTypeHandler(const TypeHandler & type) = 0;

	virtual const TypeHandler * QueryTypeHandler(TypeID type) const = 0;

	virtual Array <Address> Compare(const PropertySet & a, const PropertySet & b) const = 0;	//returns addresses for matching values
};

REFLEX_END
