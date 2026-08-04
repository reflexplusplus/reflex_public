#include "reflex/data/format/formats/detail/object2string.h"




//
//impl

Reflex::CString Reflex::Data::Detail::BoolToString(const KeyMap & keymap, const Object & object)
{
	return Reflex::Detail::kFalseTrue[Cast<BoolProperty>(object)->value];
}

Reflex::CString Reflex::Data::Detail::Int32ToString(const KeyMap & keymap, const Object & object)
{
	return ToCString(Cast<Int32Property>(object)->value);
}

Reflex::CString Reflex::Data::Detail::Int64ToString(const KeyMap & keymap, const Object & object)
{
	return ToCString(Cast<Int64Property>(object)->value);
}

Reflex::CString Reflex::Data::Detail::UInt32ToString(const KeyMap & keymap, const Object & object)
{
	return ToCString(Cast<UInt32Property>(object)->value);
}

Reflex::CString Reflex::Data::Detail::UInt64ToString(const KeyMap & keymap, const Object & object)
{
	return ToCString(Cast<UInt64Property>(object)->value);
}

Reflex::CString Reflex::Data::Detail::Float64ToString(Float64 value, UInt max_precision)
{
	CString string = Reflex::TrimRight<const char>(Reflex::ToCString(value, max_precision), [](char c) { return c == '0'; });

	if (string.GetLast() == '.') string.Push('0');

	return string;
}

Reflex::CString Reflex::Data::Detail::Float32ToString(const KeyMap & keymap, const Object & object)
{
	return Float64ToString(Cast<Float32Property>(object)->value, 8);
}

Reflex::CString Reflex::Data::Detail::Float64ToString(const KeyMap & keymap, const Object & object)
{
	return Float64ToString(Cast<Float64Property>(object)->value, 16);
}

Reflex::CString Reflex::Data::Detail::Key32ToString(const KeyMap & keymap, const Object & object)
{
	auto key = Cast<Key32Property>(object)->value;

	if (auto string = GetKey(keymap, key))
	{
		CString temp;

		for (auto c : string)
		{
			if (!Detail::IsAlphaNumericCharacter(c))
			{
				return Join(char(39), string, char(39));
			}
		}

		return string;
	}

	return {};
}

Reflex::CString Reflex::Data::Detail::BinaryToString(const KeyMap & keymap, const Object & object)
{
	return Join("$", Data::BytesToHex(Cast<BinaryProperty>(object)->value));
}

Reflex::CString Reflex::Data::Detail::CStringToQuotedString(const KeyMap & keymap, const Object & object)
{
	return Join('"', Cast<CStringProperty>(object)->value, '"');
}

Reflex::CString Reflex::Data::Detail::WStringToQuotedString(const KeyMap & keymap, const Object & object)
{
	auto utf8 = EncodeUTF8(Cast<WStringProperty>(object)->value);

	return Join('"', Unpack<CString::View>(utf8), '"');
}
