#pragma once

#include "reflex/data/format/formats/detail/propertysheet_interface.h"
#include "../../tokeniser.h"




REFLEX_NS(Reflex::Data::Detail)

template <class TYPE> struct Stringizer {};

template <>
struct Stringizer <bool>
{
	static bool FromString(KeyMap & keymap, const CString::View & value)
	{
		return value == Reflex::Detail::kFalseTrue[true];
	}

	static constexpr ObjectToStringFn ToString = &BoolToString;
};

template <>
struct Stringizer <Int32>
{
	static Int32 FromString(KeyMap & keymap, const CString::View & value)
	{
		return ToInt32(value);
	}

	static constexpr ObjectToStringFn ToString = &Int32ToString;
};

template <>
struct Stringizer <Int64>
{
	static Int64 FromString(KeyMap & keymap, const CString::View & value)
	{
		return ToInt64(value);
	}

	static constexpr ObjectToStringFn ToString = &Int64ToString;
};

template <>
struct Stringizer <UInt32>
{
	static UInt32 FromString(KeyMap & keymap, const CString::View & value)
	{
		return UInt32(ToUInt64(value));
	}

	static constexpr ObjectToStringFn ToString = &UInt32ToString;
};

template <>
struct Stringizer <UInt64>
{
	static UInt64 FromString(KeyMap & keymap, const CString::View & value)
	{
		return ToUInt64(value);
	}

	static constexpr ObjectToStringFn ToString = &UInt64ToString;
};

template <>
struct Stringizer <Float32>
{
	static Float32 FromString(KeyMap & keymap, const CString::View & value)
	{
		return ToFloat32(value);
	}

	static constexpr ObjectToStringFn ToString = &Float32ToString;
};

template <>
struct Stringizer <Float64>
{
	static Float64 FromString(KeyMap & keymap, const CString::View & value)
	{
		return ToFloat64(value);
	}

	static constexpr ObjectToStringFn ToString = &Float64ToString;
};

template <>
struct Stringizer <Key32>
{
	static Key32 FromString(KeyMap & keymap, const CString::View & value)
	{
		return RegisterKey(keymap, value);
	}

	static constexpr ObjectToStringFn ToString = &Key32ToString;
};

template <>
struct Stringizer <Data::Archive>
{
	static Data::Archive FromString(KeyMap & keymap, const CString::View & value)
	{
		Data::Archive bytes(value.size / 2);

		Data::Detail::HexToBytes(bytes, value);

		return bytes;
	}

	static constexpr ObjectToStringFn ToString = &BinaryToString;
};

template <>
struct Stringizer <CString>
{
	static CString FromString(KeyMap & keymap, const CString::View & value)
	{
		return value;
	}

	static constexpr ObjectToStringFn ToString = &CStringToQuotedString;
};

template <>
struct Stringizer <WString>
{
	static WString FromString(KeyMap & keymap, const CString::View & value)
	{
		return DecodeUTF8(Reinterpret<Data::Archive::View>(value));
	}

	static constexpr ObjectToStringFn ToString = &WStringToQuotedString;
};

REFLEX_END
