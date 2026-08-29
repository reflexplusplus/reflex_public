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
	WString GetReflexExecutablePath(WString::View reflex_path);

	CString EncodeUTF8(WString::View text);
	bool RunCommand(const WString & path, ArrayView <WString> args, System::FileHandle * std_out = nullptr, bool allow_window = false);

	TemplateDefinition DecodeTemplate(const Data::PropertySet & config);
	WString GetProjectFolderName(const TemplateDefinition & tmpl, ArrayView<Pair<CString>> inputs);

	void ThrowError(CString::View msg, CString::View error);
	void ThrowError(CString::View msg, WString::View error);
	void Require(bool test, CString::View msg, CString::View error);
	void Require(bool test, CString::View msg, WString::View error);
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

inline void ReflexCLI::Require(bool test, CString::View msg, CString::View error)
{
	if (!test) ThrowError(msg, error);
}

inline void ReflexCLI::Require(bool test, CString::View msg, WString::View error)
{
	if (!test) ThrowError(msg, error);
}
