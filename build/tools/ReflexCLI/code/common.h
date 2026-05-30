#pragma once

#include "reflex_ext/bootstrap.h"




//
//declarations

namespace ReflexCLI
{

	using namespace Reflex;


	struct TokenDefinition;

	struct TemplateDefinition;


	Data::PropertySet OpenTemplateCfg(const WString::View & template_folder);

	
	void EncodeTemplate(const TemplateDefinition & tmpl, Data::PropertySet & config);

	TemplateDefinition DecodeTemplate(const Data::PropertySet & config);


	WString GetReflexPath();

	WString GetReflexExecutablePath(const WString::View & reflex_path);

}




//
//TokenDefinition

struct ReflexCLI::TokenDefinition
{
	void Serialize(Data::Archive & stream) const;
	void Deserialize(Data::Archive::View & stream);
	bool operator==(const TokenDefinition & value) const = default;

	CString id;
	CString token;
	CString name;
};




//
//TemplateDefinition

struct ReflexCLI::TemplateDefinition
{
	void Serialize(Data::Archive & stream) const;
	void Deserialize(Data::Archive::View & stream);
	bool operator==(const TemplateDefinition & value) const = default;

	WString folder;
	CString name;
	Data::Archive description_utf8;
	Array <TokenDefinition> paths;
	Array <TokenDefinition> strings;
};




//
//impl

inline void ReflexCLI::TokenDefinition::Serialize(Data::Archive & stream) const
{
	Data::Serialize(stream, id, token, name);
}

inline void ReflexCLI::TokenDefinition::Deserialize(Data::Archive::View & stream)
{
	Data::Deserialize(stream, id, token, name);
}

inline void ReflexCLI::TemplateDefinition::Serialize(Data::Archive & stream) const
{
	Data::SerializeUTF8(stream, folder);

	Data::Serialize(stream, name, description_utf8, paths, strings);
}

inline void ReflexCLI::TemplateDefinition::Deserialize(Data::Archive::View & stream)
{
	Data::DeserializeUTF8(stream, folder);

	Data::Deserialize(stream, name, description_utf8, paths, strings);
}

inline Reflex::Data::PropertySet ReflexCLI::OpenTemplateCfg(const WString::View & template_folder)
{
	auto install_cfg_path = Join(template_folder, L"install.cfg");

	auto config = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(install_cfg_path));

	if (auto inherit = Data::GetCString(config, "inherit"))
	{
		auto inherit_path = File::ResolveIncludePath(template_folder, ToWString(inherit));
		auto parent = Data::DecodePropertySet(Data::kPropertySheetFormat, File::Open(inherit_path));

		Data::Assimilate(parent, config);

		config = parent;
	}

	Data::SetWString(config, "folder", template_folder);

	return config;
}

inline void ReflexCLI::EncodeTemplate(const TemplateDefinition & tmpl, Data::PropertySet & config)
{
	REFLEX_ASSERT(tmpl.folder);
	Data::SetWString(config, "folder", tmpl.folder);
	Data::SetCString(config, "name", tmpl.name);
	Data::SetCString(config, "description", Data::Unpack<CString::View>(tmpl.description_utf8));

	auto input = Data::AcquirePropertySet(config, "input");

	const Pair <const Array <TokenDefinition> &, Key32> groups[] = { { tmpl.paths, K32("paths") }, { tmpl.strings, K32("strings") } };

	for (auto & [src, id] : groups)
	{
		auto values = Data::AcquirePropertySetArray(input, id);

		for (auto & token : src)
		{
			auto value = Data::AddPropertySet(values);

			if (token.id) Data::SetCString(value, "id", token.id);

			Data::SetCString(value, "name", token.name);
			Data::SetCString(value, "token", token.token);
		}
	}
}

inline ReflexCLI::TemplateDefinition ReflexCLI::DecodeTemplate(const Data::PropertySet & config)
{
	TemplateDefinition tmpl;

	tmpl.folder = Data::GetWString(config, "folder");
	tmpl.name = Data::GetCString(config, "name");
	tmpl.description_utf8 = Data::Pack(Data::GetCString(config, "description"));

	auto input = Data::GetPropertySet(config, "input");

	const Pair <Array <TokenDefinition> &, Key32> groups[] = { { tmpl.paths, K32("paths") }, { tmpl.strings, K32("strings") } };

	for (auto & [dst, id] : groups)
	{
		for (auto & value : Data::GetPropertySetArray(input, id))
		{
			dst.Push({ .id = Data::GetCString(value, "id"), .token = Data::GetCString(value, "token"), .name = Data::GetCString(value, "name") });
		}
	}

	return tmpl;
}

inline Reflex::WString ReflexCLI::GetReflexPath()
{
	constexpr WString::View kRepositories[] = { L"reflex_public", L"reflex" };

	auto exepath = System::GetExecutablePath();
	auto parts = Split(exepath, File::kStroke);

	for (auto i : kRepositories)
	{
		if (auto idx = ReverseSearch(parts, i))
		{
			auto path = Merge(Left(parts, idx.value + 1), File::kStroke);

			return Join(path, File::kStroke);
		}
	}

	REFLEX_ASSERT(false);

	return {};
}

inline Reflex::WString ReflexCLI::GetReflexExecutablePath(const WString::View & reflex_path)
{
#if defined(REFLEX_OS_WINDOWS)
	return Join(reflex_path, L"bin/tools/win/reflex.exe");
#elif defined(REFLEX_OS_LINUX)
	return Join(reflex_path, L"bin/tools/linux/reflex");
#elif defined(REFLEX_OS_MACOS)
	return Join(reflex_path, L"bin/tools/macos/reflex");
#else
	REFLEX_ASSERT(false);
	return {};
#endif
}
