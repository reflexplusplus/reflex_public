#pragma once

#include "object2string.h"
#include "../../../detail/tokeniser.h"




//
//Detail

REFLEX_NS(Reflex::Data::Detail)

struct PropertySheetInterface : public Object	//TODO Interface not object
{
	using LineScope = Reflex::Detail::ScopeOf <UInt*,true>;

	using ObjectWithType = Pair < Reference <Object>, TypeID >;

	enum TokenType : UInt8
	{
		kTokenTypeWord = Detail::Tokeniser::kValueTypeWord,
		kTokenTypeInt = Detail::Tokeniser::kValueTypeInt,
		kTokenTypeFloat = Detail::Tokeniser::kValueTypeFloat,
		kTokenTypeHex = Detail::Tokeniser::kValueTypeHex,

		kTokenTypeSingleQuotedString = Detail::Tokeniser::kValueTypeSingleQuotedString,
		kTokenTypeDoubleQuotedString = Detail::Tokeniser::kValueTypeDoubleQuotedString,

		kTokenTypeReference,
		kTokenTypeBool
	};


	virtual bool Begin(PropertySet & root) const { return true; }

	virtual void End(PropertySet & root) const { }

	virtual PropertySheetInterface & GetInterface(ObjectWithType & object) { return *this; }


	virtual bool OnSetOption(Key32 id, const CString::View & value) const = 0;

	virtual bool OnCondition(const ArrayView < Pair <TokenType,CString::View> > & condition) const = 0;

	virtual ObjectWithType CreateObject(Object & parent, const CString::View & type, Key32 id, bool is_stub) const = 0;

	virtual ObjectWithType CreateObjectArray(const CString::View & type, const Array <ObjectWithType> & values) const = 0;

	virtual ObjectWithType CreateValue(KeyMap & keymap, const CString::View & type, TokenType value_t, const CString::View & value) const = 0;

	virtual ObjectWithType CreateValueArray(KeyMap & keymap, const CString::View & type, TokenType tokentype, const Array <CString::View> & values) const = 0;
};

struct StandardPropertySheetInterface : public PropertySheetInterface
{
	//PROVISIONAL API

	struct TypeInfo
	{
		CString::View type_name;

		TypeID type_ids[2];	//object, array
	};

	struct PropertySetType : public TypeInfo
	{
		FunctionPointer <PropertySheetInterface::ObjectWithType(Object & parent, Key32 id)> object_ctr;

		FunctionPointer <PropertySheetInterface::ObjectWithType(const Array <PropertySheetInterface::ObjectWithType> &)> array_ctr;
	};

	struct ValueType : public TypeInfo
	{
		FunctionPointer <PropertySheetInterface::ObjectWithType(KeyMap&, const CString::View&)> object_ctr;

		FunctionPointer <PropertySheetInterface::ObjectWithType(KeyMap&, const Array <CString::View>&)> array_ctr;

		ObjectToStringFn to_string[2];	//Object, ArrayOf@Object
	};

	virtual void RegisterObjectTypeHandler(Key32 type_name, const PropertySetType & type) = 0;

	virtual void RegisterValueTypeHandler(Key32 type_name, TokenType tokentype, const ValueType & type) = 0;
};

TRef <Format> CreateCustomFormat(PropertySheetInterface & callbacks, const ArrayView <TypeID> & supported);

template <class TYPE> inline PropertySheetInterface::ObjectWithType MakeObjectWithType(TYPE & object = *REFLEX_CREATE(TYPE))
{
	REFLEX_STATIC_ASSERT(kIsObject<TYPE>);

	return { object, GetTypeID<TYPE>() };
}

template <class TYPE, class ... ARGS> inline PropertySheetInterface::ObjectWithType CreateObjectWithType(ARGS && ... args)
{
	return { New<TYPE>(std::forward<ARGS>(args)...), GetTypeID<TYPE>() };
}




//
//typed

extern const ConstTRef <StandardPropertySheetInterface> g_standard_propertysheet_interface;

REFLEX_END
