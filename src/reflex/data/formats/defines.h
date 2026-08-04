#pragma once

#include "propertysheet/stringtizer.h"




REFLEX_NS(Reflex::Data)

typedef Tuple <UInt, CString> ParseError;

typedef Detail::StandardPropertySheetInterface::ValueType ValueTypeHandler;

template <class TYPE, class ... VARGS> inline Detail::PropertySheetInterface::ObjectWithType CreateWithType(VARGS && ... vargs)
{
	return Detail::MakeObjectWithType(*REFLEX_CREATE(TYPE, std::forward<VARGS>(vargs)...));
}

template <class TYPE> Detail::StandardPropertySheetInterface::PropertySetType MakePropertySetType(const CString::View & tname);

template <class TYPE> ValueTypeHandler MakeValueType(const CString::View & tname);

void Write(Data::Archive & archive, const CString::View & string);


extern const ValueTypeHandler & kPropertySheetBool;
extern const ValueTypeHandler & kPropertySheetUInt32;
extern const ValueTypeHandler & kPropertySheetUInt64;
extern const ValueTypeHandler & kPropertySheetInt32;
extern const ValueTypeHandler & kPropertySheetInt64;
extern const ValueTypeHandler & kPropertySheetFloat32;
extern const ValueTypeHandler & kPropertySheetFloat64;
extern const ValueTypeHandler & kPropertySheetCString;
extern const ValueTypeHandler & kPropertySheetWString;

constexpr CString::View kArrayComma = ", ";

REFLEX_END




//
//impl

template <class TYPE> inline Reflex::Data::Detail::StandardPropertySheetInterface::PropertySetType Reflex::Data::MakePropertySetType(const CString::View & tname)
{
	typedef Detail::PropertySheetInterface::ObjectWithType ObjectWithType;

	typedef ObjectOf < Array < Reference <TYPE> > > ArrayType;

	Detail::StandardPropertySheetInterface::PropertySetType type = { tname, { REFLEX_TYPEID(TYPE), REFLEX_TYPEID(ArrayType) } };

	type.object_ctr = [](Object & parent, Key32 id) -> ObjectWithType
	{
		return CreateWithType<TYPE>();
	};

	type.array_ctr = [](const Array <ObjectWithType> & objects) -> ObjectWithType
	{
		auto rtn = REFLEX_CREATE(ArrayType);

		auto & values = rtn->value;
		
		values.Allocate(objects.GetSize());

		for (auto & i : objects) values.template Push<kAllocateNone>(Cast<TYPE>(i.a));

		return Detail::MakeObjectWithType(*rtn);
	};

	return type;
}

template <class TYPE> inline Reflex::Data::ValueTypeHandler Reflex::Data::MakeValueType(const CString::View & tname)
{
	typedef ObjectOf <TYPE> ObjectOfType;

	typedef ObjectOf < Array <TYPE> > ArrayType;

	ValueTypeHandler type = { tname, { REFLEX_TYPEID(ObjectOfType), REFLEX_TYPEID(ArrayType) } };

	type.object_ctr = [](KeyMap & keymap, const CString::View & value)
	{
		return CreateWithType<ObjectOfType>(Detail::Stringizer<TYPE>::FromString(keymap, value));
	};

	type.array_ctr = [](KeyMap & keymap, const Array <CString::View> & values)
	{
		Array <TYPE> a;

		a.Allocate(values.GetSize());

		for (auto & i : values) a.template Push<kAllocateNone>(Detail::Stringizer<TYPE>::FromString(keymap, i));

		return CreateWithType<ArrayType>(std::move(a));
	};

	type.to_string[0] = Detail::Stringizer<TYPE>::ToString;

	type.to_string[1] = [](const KeyMap & keymap, const Object & object)
	{
		CString output;

		output.Push('[');

		auto & values = Cast<ArrayType>(object)->value;

		if (values)
		{
			auto fn = Detail::Stringizer<TYPE>::ToString;

			ObjectOf <TYPE> objectof;

			for (auto & i : values)
			{
				objectof.value = i;

				output.Append(fn(keymap, objectof));

				output.Append(kArrayComma);
			}

			output.Shrink(2);
		}

		output.Push(']');

		return output;
	};

	return type;
}
