#pragma once

#include "reflex_ext/bootstrap.h"

namespace ReflexCLI
{
	using namespace Reflex;

	constexpr WString::View kBat = L"bat";
	constexpr WString::View kCommand = L"command";

	struct TokenDefinition;
	
	struct TemplateDefinition;

	WString GetReflexPath();

	WString MakeReflexExecutablePath(WString::View reflex_path);

	CString EncodeUTF8(WString::View text);
	WString DecodeUTF8(CString::View text);
		
	bool RunCommand(const WString & path, ArrayView <WString> args, System::FileHandle * std_out = nullptr);

	TemplateDefinition DecodeTemplate(const Data::PropertySet & config);
	WString GetProjectFolderName(const TemplateDefinition & tmpl, ArrayView<Pair<CString>> inputs);

	void ThrowError(CString::View msg, CString::View error);
	void ThrowError(CString::View msg, WString::View error);
	void Require(bool test, CString::View msg, CString::View error);
	void Require(bool test, CString::View msg, WString::View error);

	template <typename TYPE> Array <Array <TYPE>> CopyOwned(ArrayView <ArrayView <TYPE>> items);
	template <typename TYPE> Array <Array <TYPE>> CopyOwned(const Array <ArrayView <TYPE>> & items);

	template <typename TYPE> Array <ArrayView <TYPE>> CopyUnowned(ArrayView < Array <TYPE> > items);
	template <typename TYPE> Array <ArrayView <TYPE>> CopyUnowned(const Array <Array <TYPE>> & items);
}

struct ReflexCLI::TokenDefinition
{
	void Serialize(Data::Archive & stream) const;
	void Deserialize(Data::Archive::View & stream);
	bool operator==(const TokenDefinition & value) const = default;

	CString id;
	CString token;
	CString name;
};

struct ReflexCLI::TemplateDefinition
{
	void Serialize(Data::Archive & stream) const;
	void Deserialize(Data::Archive::View & stream);
	bool operator==(const TemplateDefinition & value) const = default;

	WString folder;
	CString name;
	Data::Archive description_utf8;
	Array<CString> platforms;
	Array<TokenDefinition> paths;
	Array<TokenDefinition> strings;
};




//
//impl

inline Reflex::CString ReflexCLI::EncodeUTF8(WString::View text)
{
	return Data::Unpack<CString::View>(Data::EncodeUTF8(text));
}

inline Reflex::WString ReflexCLI::DecodeUTF8(CString::View text)
{
	return Data::DecodeUTF8(Data::Pack(text));
}

inline void ReflexCLI::Require(bool test, CString::View msg, CString::View error)
{
	if (!test) ThrowError(msg, error);
}

inline void ReflexCLI::Require(bool test, CString::View msg, WString::View error)
{
	if (!test) ThrowError(msg, error);
}

template <typename TYPE> inline Reflex::Array <Reflex::Array <TYPE>> ReflexCLI::CopyOwned(ArrayView <ArrayView <TYPE>> items)
{
	Array < Array <TYPE> > rtn;

	rtn.Allocate(items.size);

	for (auto & i : items) rtn.Push(i);

	return rtn;
}

template <typename TYPE> inline Reflex::Array <Reflex::Array <TYPE>> ReflexCLI::CopyOwned(const Array <ArrayView <TYPE>> & items)
{
	return CopyOwned(ToView(items));
}

template <typename TYPE> inline Reflex::Array <Reflex::ArrayView <TYPE>> ReflexCLI::CopyUnowned(ArrayView < Array <TYPE> > items)
{
	Array < ArrayView <TYPE> > rtn;

	rtn.Allocate(items.size);

	for (auto & i : items) rtn.Push(i);

	return rtn;
}

template <typename TYPE> inline Reflex::Array <Reflex::ArrayView <TYPE>> ReflexCLI::CopyUnowned(const Array <Array <TYPE>> & items)
{
	return CopyUnowned(ToView(items));
}
