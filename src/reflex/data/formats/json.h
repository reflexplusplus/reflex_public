#pragma once

#include "defines.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct JsonFormat : public Format
{
	static constexpr CString::View kNull = "null";

	REFLEX_OBJECT(JsonFormat, Format);


	JsonFormat();

	bool SupportsType(TypeID type_id) const override;

	void OnReset(PropertySet & data) const override;

	bool OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const override;

	bool OnEncode(Archive & out, const PropertySet & in, UInt32 options) const override;


	static void OutputKey(const CString::View & key, Archive & out);

	static void OutputValues(const KeyMap & keymap, const PropertySet & in, TypeID type_id, Archive & out, FunctionPointer <void(Archive&, const Object&)> packer);

	static void OutputArray(const KeyMap & keymap, Key32 id, const void * ptr, UInt n, Archive & out, FunctionPointer <CString(const KeyMap &, const void*&)> stringify);

	void OutputLevel(const KeyMap & keymap, const PropertySet & in, Archive & out) const;


	Array <ValueTypeHandler> m_valuehandlers;
};

REFLEX_END_INTERNAL
