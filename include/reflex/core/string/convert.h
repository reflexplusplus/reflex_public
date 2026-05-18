#pragma once

#include "string.h"




//
//Primary API

namespace Reflex
{

	//WString -> CString

	CString ToCString(const WString::View & ref);

	CString ToCString(const WString & wstring);



	//CString -> WString

	WString ToWString(const CString::View & ref);

	WString ToWString(const CString & cstring);



	//CString -> IntXX

	UInt32 ToUInt32(const CString::View & string);

	UInt64 ToUInt64(const CString::View & string);


	Int32 ToInt32(const CString::View & string);

	Int64 ToInt64(const CString::View & string);



	//WString -> IntXX

	UInt32 ToUInt32(const WString::View & string);

	UInt64 ToUInt64(const WString::View & string);


	Int32 ToInt32(const WString::View & string);

	Int64 ToInt64(const WString::View & string);



	//CString -> FloatXX

	Float32 ToFloat32(const CString::View & string);

	Float64 ToFloat64(const CString::View & string);



	//WString -> FloatXX

	Float32 ToFloat32(const WString::View & string);

	Float64 ToFloat64(const WString::View & string);



	//IntXX -> CString

	CString ToCString(UInt32 value);

	CString ToCString(UInt64 value);


	CString ToCString(Int32 value);

	CString ToCString(Int64 value);


	CString ToCString(Float32 value, UInt precision, bool discard_zeros = true);

	CString ToCString(Float64 value, UInt precision, bool discard_zeros = true);


	
	//IntXX -> WString

	WString ToWString(UInt32 value);

	WString ToWString(UInt64 value);


	WString ToWString(Int32 value);

	WString ToWString(Int64 value);


	WString ToWString(Float32 value, UInt precision, bool discard_zeros = true);

	WString ToWString(Float64 value, UInt precision, bool discard_zeros = true);

}




//
//impl

REFLEX_NS(Reflex::Detail)

CString::View ToCString(Int64 value, const CString::Region & output);

CString::View ToCString(UInt64 value, const CString::Region & output);

CString::View ToCString(Float64 value, UInt precision, bool discard_zeros, const CString::Region & output);

REFLEX_END

inline Reflex::CString Reflex::ToCString(const WString & wstring)
{
	return ToCString(ToView(wstring));
}

inline Reflex::WString Reflex::ToWString(const CString & cstring)
{
	return ToWString(ToView(cstring));
}

inline Reflex::UInt32 Reflex::ToUInt32(const CString::View & string)
{
	return UInt32(ToUInt64(string));
}

inline Reflex::Int32 Reflex::ToInt32(const CString::View & string)
{
	return Int32(ToInt64(string));
}

inline Reflex::UInt32 Reflex::ToUInt32(const WString::View & string)
{
	return UInt32(ToUInt64(string));
}

inline Reflex::Int32 Reflex::ToInt32(const WString::View & string)
{
	return Int32(ToInt64(string));
}

inline Reflex::Float32 Reflex::ToFloat32(const CString::View & string)
{
	return Float32(ToFloat64(string));
}

inline Reflex::Float32 Reflex::ToFloat32(const WString::View & string)
{
	return Float32(ToFloat64(string));
}

inline Reflex::CString Reflex::ToCString(UInt32 value)
{
	return ToCString(UInt64(value));
}

inline Reflex::CString Reflex::ToCString(UInt64 value)
{
	char buffer[20];

	return Detail::ToCString(value, { buffer, 20 });
}

inline Reflex::CString Reflex::ToCString(Int32 value)
{
	return ToCString(Int64(value));
}

inline Reflex::CString Reflex::ToCString(Int64 value)
{
	char buffer[21];

	return Detail::ToCString(value, { buffer, 21 });
}

inline Reflex::CString Reflex::ToCString(Float32 value, UInt precision, bool discard_zeros)
{
	return ToCString(Float64(value), precision, discard_zeros);
}

inline Reflex::WString Reflex::ToWString(UInt32 value)
{
	return ToWString(UInt64(value));
}

inline Reflex::WString Reflex::ToWString(UInt64 value)
{
	char buffer[20];

	return ToWString(Detail::ToCString(value, { buffer, 20 }));
}

inline Reflex::WString Reflex::ToWString(Int32 value)
{
	return ToWString(Int64(value));
}

inline Reflex::WString Reflex::ToWString(Int64 value)
{
	char buffer[21];

	return ToWString(Detail::ToCString(value, { buffer, 21 }));
}

inline Reflex::WString Reflex::ToWString(Float32 value, UInt precision, bool discard_zeros)
{
	return ToWString(Float64(value), precision, discard_zeros);
}