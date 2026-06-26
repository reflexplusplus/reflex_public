#pragma once

#include "../../../../include/reflex/data/format/formats/xml.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct AbstractMarkupFormat : public Format
{
	AbstractMarkupFormat(const Detail::StandardPropertySheetInterface & standard);


	void OnReset(PropertySet & node) const override;

	bool SupportsType(TypeID type_id) const override;

	bool OnEncode(Archive & out, const PropertySet & root, UInt32 options) const override;


	void Validate(const PropertySet & root);

	
	typedef Pair <TypeID,Detail::ObjectToStringFn> TypeHandler;

	TypeHandler m_types[4];


	static constexpr char kValueOpenData[2] = { '=', '"' };

	static constexpr CString::View kValueOpen = { kValueOpenData, 2 };

	static TypeID const * const kSupportedTypes[];
};

struct XML : public AbstractMarkupFormat
{
	REFLEX_OBJECT(XML, AbstractMarkupFormat);

	using AbstractMarkupFormat::AbstractMarkupFormat;

	bool OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const override;
};

struct Markup : public AbstractMarkupFormat
{
	REFLEX_OBJECT(Markup, AbstractMarkupFormat);

	using AbstractMarkupFormat::AbstractMarkupFormat;

	bool OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const override;
	
	bool OnEncode(Archive & out, const PropertySet & root, UInt32 options) const override;
};

REFLEX_END_INTERNAL
