#include "common.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI)

WString StripProjectFolderValue(WString value)
{
	auto idx = value.GetSize();
	while (idx--)
	{
		auto w = value[idx];
		if (w < 255 && !(Data::Detail::IsAlphaNumericCharacter(char(w)) || w == '-')) value.Remove(idx);
	}
	return value;
}

REFLEX_END_INTERNAL

ReflexCLI::TemplateDefinition ReflexCLI::DecodeTemplate(const Data::PropertySet & config)
{
	TemplateDefinition tmpl;
	tmpl.folder = Data::GetWString(config, "folder");
	tmpl.name = Data::GetCString(config, "name");
	tmpl.description_utf8 = Data::Pack(Data::GetCString(config, "description"));
	tmpl.platforms = Data::GetCStringArray(config, "platforms");
	auto input = Data::GetPropertySet(config, "input");
	const Pair<Array<TokenDefinition> &, Key32> groups[] = { { tmpl.paths, K32("paths") }, { tmpl.strings, K32("strings") } };
	for (auto & [dst, id] : groups)
		for (auto & value : Data::GetPropertySetArray(input, id))
			dst.Push({ .id = Data::GetCString(value, "id"), .token = Data::GetCString(value, "token"), .name = Data::GetCString(value, "name") });
	return tmpl;
}

Reflex::WString ReflexCLI::GetProjectFolderName(const TemplateDefinition & tmpl, ArrayView<Pair<CString>> inputs)
{
	for (auto & token : tmpl.strings)
	{
		if (token.token != "PRODUCT-NAME") continue;
		for (auto & input : inputs)
		{
			if (input.a == token.id) return Lowercase(StripProjectFolderValue(Replace(ToWString(input.b), L' ', L'_')));
		}
	}
	return {};
}

Reflex::WString ReflexCLI::GetReflexPath()
{
	constexpr WString::View kRepositories[] = { L"reflex_public", L"reflex", L"reflex_master" };
	auto executable_path = System::GetExecutablePath();
	auto parts = Split(File::SplitFilename(executable_path).a, File::kStroke);
	for (auto i : kRepositories)
	{
		if (auto idx = ReverseSearch(parts, i)) return Join(Merge(Left(parts, idx.value + 1), File::kStroke), File::kStroke);
	}
	return {};
}

Reflex::WString ReflexCLI::GetReflexExecutablePath(WString::View reflex_path)
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

Reflex::CString ReflexCLI::EncodeUTF8(WString::View text)
{
	return Data::Unpack<CString::View>(Data::EncodeUTF8(text));
}

bool ReflexCLI::RunCommand(const WString & path, ArrayView <WString> args, System::FileHandle * std_out, bool allow_window)
{
	auto process = Make<System::Process>(path, args, System::Process::Options{ .std_out = std_out, .allow_window = allow_window });

	process->Wait();

	auto exit_code = process->GetExitCode();
	
	return exit_code && exit_code.value == 0;
}

void ReflexCLI::ThrowError(CString::View msg, CString::View error)
{
	Bootstrap::CLI::ThrowError(Join(msg, ':', ' ', error));
}

void ReflexCLI::ThrowError(CString::View msg, WString::View error)
{
	ThrowError(msg, Data::Unpack<CString::View>(Data::EncodeUTF8(error)));
}
