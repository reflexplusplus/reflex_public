#pragma once

#include "../../../../include/reflex/glx/style.h"
#include "../../../../include/reflex/glx/detail/resource.h"
#include "../../../../include/reflex/glx/functions/lookup.h"




//
//declarations

REFLEX_NS(Reflex::GLX)

struct StyleAccessor : public Style
{
	using Style::Style;
	using Data::PropertySet::Clear;
	using Style::OnSetProperty;
	using Style::OnQueryProperty;
	using Style::m_is_root_style;
	using Style::m_non_virtual;
	using Style::m_states;

	template <bool VIRTUAL> bool QueryProperty(Address address, Reflex::Object * & in_out) const;
};

struct ResourceParser : public Data::Detail::PropertySheetInterface
{
	bool Begin(Data::PropertySet & root) const override { return true; }

	bool OnSetOption(Key32 id, const CString::View & value) const override { return false; }

	bool OnCondition(const ArrayView < Pair <TokenType, CString::View> > & condition) const override;

	ObjectWithType CreateObject(Reflex::Object & parent, const CString::View & type, Key32 id, bool is_stub) const override;

	ObjectWithType CreateObjectArray(const CString::View & type, const Array <ObjectWithType> & objects) const override;

	ObjectWithType CreateValue(Data::KeyMap & keymap, const CString::View & type, TokenType value_t, const CString::View & value) const override;

	ObjectWithType CreateValueArray(Data::KeyMap & keymap, const CString::View & type, TokenType tokentype, const Array <CString::View> & values) const override;
};

struct StyleSheetParser : public ResourceParser
{
	bool Begin(Data::PropertySet & root) const override;

	PropertySheetInterface & GetInterface(ObjectWithType & object) override;

	bool OnSetOption(Key32 id, const CString::View & value) const override;
};

struct StyleSheetContext
{
	WString::View path;
	StyleSheet * sheet;
	const Data::PropertySet * options;
};

WString GetPathProperty(const Data::PropertySet & propertyset);

void ThrowStylesheetParseError(const CString::View & error_type, const CString::View & error_value);

#if REFLEX_DEBUG
inline void ThrowInvalidPropertyError(const CString::View & cls, Key32 id, AbstractProperty & property, bool has_property)
{
	CString::View property_name = GetKey(id);

	if (property_name)
	{
		if (property_name[0] == '_') return;	//intentional 
	}
	else
	{
		property_name = "missing";
	}

	if (has_property)	//type mismatch
	{
		ThrowStylesheetParseError("invalid property", Join(cls, ' ', kSingleQuote, property_name, "' with ", property.object_t->tname));
	}
	else
	{
		ThrowStylesheetParseError("invalid property", Join(cls, ' ', kSingleQuote, property_name, kSingleQuote));
	}

}
#endif


extern StyleSheetContext g_current_stylesheet;

constexpr CString::View kInvalidType = "invalid type";

REFLEX_END




//
//impl

#if (!REFLEX_DEBUG)
inline void Reflex::GLX::ThrowStylesheetParseError(const CString::View & error, const CString::View & error_value) {}
#endif

