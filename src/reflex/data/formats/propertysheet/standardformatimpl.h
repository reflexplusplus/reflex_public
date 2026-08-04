#pragma once

#include "formatimpl.h"




REFLEX_NS(Reflex::Data)

typedef Pair <Detail::PropertySheetInterface::TokenType,Key32> ValueHandlerID;

struct StandardPropertySheetFormatImpl :
	public PropertySheetFormatImpl,
	public Detail::StandardPropertySheetInterface
{
public:

	typedef ObjectArray <Object> GenericObjectArray;

	StandardPropertySheetFormatImpl();


	void RegisterObjectTypeHandler(Key32 type_name, const PropertySetType & type) override;

	void RegisterValueTypeHandler(Key32 type_name, TokenType tokentype, const ValueType & type) override;

	void RegisterImplicitValueType(Key32 type_name, TokenType tokentype, const ValueType & type);



private:

	//interface

	bool Begin(PropertySet & root) const override { return true; }

	bool OnSetOption(Key32 id, const CString::View & value) const override { return false; }

	bool OnCondition(const ArrayView < Pair <TokenType, CString::View> > & condition) const override { return true; }

	ObjectWithType CreateObject(Object & parent, const CString::View & type, Key32 id, bool is_stub) const override;

	ObjectWithType CreateObjectArray(const CString::View & type, const Array <ObjectWithType> & values) const override;

	ObjectWithType CreateValue(KeyMap & keymap, const CString::View & type, TokenType tokentype, const CString::View & value) const override;

	ObjectWithType CreateValueArray(KeyMap & keymap, const CString::View & type, TokenType tokentype, const Array <CString::View> & values) const override;


	//format

	bool Export(const PropertySet & cpropertyset, ExportState & indent, Archive & out) const override;


	Sequence <Key32,PropertySetType> m_map_types;

	mutable Sequence <UInt64,ValueType> m_value_handlers;

};

REFLEX_END
