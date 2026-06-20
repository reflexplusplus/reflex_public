#include "tasks.h"
#include "common.h"


using namespace Reflex;

REFLEX_BEGIN_INTERNAL(ReflexCLI)

void ThrowError(CString::View msg, WString::View path)
{
	Bootstrap::CLI::ThrowError(Join(msg, ':', ' ', ToCString(path)));
}

struct Generator
{
	CString token;
	Key32 op = {};
	CString param;
};

struct Targets
{
	Array <WString> exclude_folders;
	Array <WString> exclude_files;
	Array <WString> replace_types;
	Array <CString> replace_vars;
	Array <CString> rename_vars;
};

struct TemplateDefinitionEx : public TemplateDefinition
{
	Array <Generator> path_generators;
	Array <Generator> string_generators;
	Targets targets;
};

struct Substitution
{
	Data::Archive from;
	Data::Archive to;
};

void AppendCStringArray(const Data::PropertySet & config, Array <CString> & generators, Key32 id)
{
	for (auto & value : Data::GetCStringArray(config, id)) generators.Push(value);
}

void AppendCStringArray(const Data::PropertySet & config, Array <WString> & generators, Key32 id)
{
	for (auto & value : Data::GetCStringArray(config, id)) generators.Push(ToWString(value));
}

void AppendGeneratorArray(Array <Generator> & generators, const ArrayView < ConstReference <Data::PropertySet> > & values)
{
	for (auto & value : values)
	{
		generators.Push
		({
			Data::GetCString(value, "token"),
			Data::GetKey32(value, "op"),
			Data::GetCString(value, "param")
		});
	}
}

TemplateDefinitionEx OpenTemplateDefinitionEx(const WString::View & template_folder)
{
	TemplateDefinitionEx tmpl;

	auto config = OpenTemplateCfg(template_folder);

	Cast<TemplateDefinition>(tmpl) = DecodeTemplate(config);

	auto generate = Data::GetPropertySet(config, "generate");
	AppendGeneratorArray(tmpl.path_generators, Data::GetPropertySetArray(generate, "paths"));
	AppendGeneratorArray(tmpl.string_generators, Data::GetPropertySetArray(generate, "strings"));

	auto exclude = Data::GetPropertySet(config, "exclude");
	AppendCStringArray(exclude, tmpl.targets.exclude_folders, "folders");
	AppendCStringArray(exclude, tmpl.targets.exclude_files, "files");

	auto replace = Data::GetPropertySet(config, "replace");
	AppendCStringArray(replace, tmpl.targets.replace_types, "types");
	AppendCStringArray(replace, tmpl.targets.replace_vars, "vars");

	auto rename = Data::GetPropertySet(config, "rename");
	AppendCStringArray(rename, tmpl.targets.rename_vars, "vars");

	return tmpl;
}

void AddTargetExcludes(TemplateDefinitionEx & tmpl, ArrayView <CString> targets)
{
	if (!targets) Bootstrap::CLI::ThrowError("invalid --targets value");
	
	if (!Search<StringCompare>(targets, "cmake"))
	{
		tmpl.targets.exclude_files.Push(L"CMakeLists.txt");
	}

	if (!Search<StringCompare>(targets, "visual_studio")) tmpl.targets.exclude_folders.Push(Join(L"win", File::kStroke));
	
	if (!Search<StringCompare>(targets, "android_studio")) tmpl.targets.exclude_folders.Push(Join(L"android", File::kStroke));

	if (!Search<StringCompare>(targets, "xcode"))
	{
		tmpl.targets.exclude_folders.Push(Join(L"macos", File::kStroke));
		tmpl.targets.exclude_folders.Push(Join(L"ios", File::kStroke));
	}
}

WString::View FindVariable(const Array <Variable> & variables, CString::View token)
{
	struct Policy
	{
		static bool eq(const Variable & var, const CString::View & token)
		{
			return var.token == token;
		}
	};
	
	if (auto pvar = SearchValue<Policy>(variables, token))
	{
		return pvar->value;
	}

	return {};
}

WString StripValue(WString value, char allowed_dash)
{
	auto idx = value.GetSize();

	while (idx--)
	{
		auto w = value[idx];

		if (w < 255)
		{
			auto c = char(w);

			if (!(Data::Detail::IsAlphaNumericCharacter(c) || c == allowed_dash))
			{
				value.Remove(idx);
			}
		}
	}

	if (value.Empty()) Bootstrap::CLI::ThrowError("strip characters resulting in empty string");

	return value;
}

WString Generate4CC(const WString::View & value)
{
	auto bytes = Data::Pack(Key32(value).value);
	
	UInt8 out[2] = { UInt8(bytes[0] ^ bytes[1]), UInt8(bytes[2] ^ bytes[3]) };

	return ToWString(Data::BytesToHex({ out, 2 }));
}

Array <Variable> ExpandPathVariables(const TemplateDefinitionEx & tmpl, ArrayView <Variable> paths)
{
	Array <Variable> expanded;

	for (auto & generator : tmpl.path_generators)
	{
		switch (generator.op.value)
		{
			case K32("Relative"):
			{
				auto store = File::ResolveIncludePath(tmpl.folder, ToWString(generator.param));
				expanded.Push({ generator.token, File::RemoveTrailingStroke(store) });
			}
			break;

		default:
			Bootstrap::CLI::ThrowError("Unknown generator op");
		}
	}

	for (auto & path : paths) expanded.Push({ path.token, File::RemoveTrailingStroke(path.value) });

	return expanded;
}

Array <Variable> ExpandStringVariables(const TemplateDefinitionEx & tmpl, ArrayView <Variable> strings)
{
	Array <Variable> expanded = strings;

	for (auto & generator : tmpl.string_generators)
	{
		WString value = FindVariable(strings, generator.param);

		switch (generator.op.value)
		{
		case K32("Strip"):
			expanded.Push({ generator.token, StripValue(value, '_')});
			break;

		case K32("PackageIdentifier"):
			expanded.Push({ generator.token, Lowercase(StripValue(Replace(value, L'_', L'-'), '-'))});
			break;

		case K32("Generate4CC"):
			expanded.Push({ generator.token, Generate4CC(value) });
			break;

		case K32("Constant"):
			expanded.Push({ generator.token, ToWString(generator.param) });
			break;

		default:
			Bootstrap::CLI::ThrowError("Unknown generator op");
		}
	}

	return expanded;
}

Array <Substitution> BuildSubstitutionList(const Array <CString> & tokens, const Array <Variable> & variables)
{
	Array <CString> ordered = tokens;
		
	Sort(ordered, [](const CString &a, const CString & b)
	{
		if (a.GetSize() == b.GetSize())
		{
			return a < b;
		}
		else
		{
			return a.GetSize() > b.GetSize();
		}
	});

	Array <Substitution> substitutions;

	for (auto & token : ordered)
	{
		if (auto value = FindVariable(variables, token))
		{
			substitutions.Push({ Data::Pack(Join("_", token, "_")), Data::EncodeUTF8(value) });
		}
		else
		{
			Bootstrap::CLI::ThrowError(Join("missing value for ", token));
		}
	}

	return substitutions;
}

Data::Archive ReplaceAll(const Data::Archive::View & input, const Array <Substitution> & substitutions)
{
	Data::Archive result = input;

	for (auto & substitution : substitutions)
	{
		Data::Archive next;
		auto remaining = ToView(result);

		while (auto idx = Search(remaining, ToView(substitution.from)))
		{
			auto prefix = ArrayView<UInt8>(remaining.data, idx.value);

			next.Append(prefix);
			next.Append(ToView(substitution.to));

			remaining = Nudge(remaining, idx.value + substitution.from.GetSize());
		}

		next.Append(remaining);
		result = std::move(next);
	}

	return result;
}

bool HasListedExtension(const WString::View & path, const Array <WString> & extensions)
{
	for (auto & extension : extensions)
	{
		if (File::CheckExtension(path, extension))
		{
			return true;
		}
	}

	return false;
}

WString RenamePath(const WString::View & path, const Array <Substitution> & substitutions)
{
	auto renamed = ReplaceAll(Data::EncodeUTF8(path), substitutions);
			
	return Data::DecodeUTF8(renamed);
}

WString InstallFolder(const TemplateDefinitionEx & tmpl, const Array <Variable> & variables, const WString::View & src_path, const WString::View & dest_path, const WString::View & dest_name, bool overwrite, System::FileHandle & std_out, bool & wrote_files)
{
	auto content_substitutions = BuildSubstitutionList(tmpl.targets.replace_vars, variables);
	auto rename_substitutions = BuildSubstitutionList(tmpl.targets.rename_vars, variables);

	auto dst = Join(dest_path, dest_name);

	bool wrote_files_here = false;

	auto [folders, files] = File::List(src_path, true);

	for (auto & folder : folders)
	{
		if (!Search(tmpl.targets.exclude_folders, folder.key))
		{
			auto folder_path = Join(src_path, folder.key);
			auto folder_renamed = File::CorrectTrailingStroke(RenamePath(File::RemoveTrailingStroke(folder.key), /*tmpl.targets.rename_types,*/ rename_substitutions));

			bool wrote_files_beneath = false;

			InstallFolder(tmpl, variables, folder_path, dst, folder_renamed, overwrite, std_out, wrote_files_beneath);

			wrote_files_here |= wrote_files_beneath;
		}
	}

	for (auto & file : files)
	{
		if (!Search(tmpl.targets.exclude_files, file.key))
		{
			auto file_path = Join(src_path, file.key);
			auto file_renamed = RenamePath(file.key, /*tmpl.targets.rename_types,*/ rename_substitutions);
			auto dst_path = Join(dst, file_renamed);

			if (System::Exists(dst_path) && !overwrite)
			{
				File::WriteLine(std_out, Join(L"skipping: ", dst_path));
			}
			else
			{
				if (SetFiltered(wrote_files_here, true))
				{
					File::MakePath(dst);

					if (!System::IsDirectory(dst))
					{
						ThrowError("could not create", dst);
					}
				}

				if (HasListedExtension(file_renamed, tmpl.targets.replace_types))
				{
					auto bytes = ReplaceAll(File::Open(file_path), content_substitutions);

					if (!File::Save(dst_path, bytes))
					{
						ThrowError("failed to write", dst_path);
					}
				}
				else if (!File::Copy(file_path, dst_path))
				{
					ThrowError("failed to copy", file_path);
				}
			}
		}
	}

	wrote_files = wrote_files_here;

	return dst;
}

char Normalize(char c)
{
	if (c == '-') c = '_';

	if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');

	return c;
}

REFLEX_END_INTERNAL

bool ReflexCLI::StringCompare::eq(CString::View a, CString::View b)
{
	if (a.size != b.size) return false;

	for (UInt i = 0; i < a.size; ++i)
	{
		if (Normalize(a[i]) != Normalize(b[i])) return false;
	}

	return true;
}

WString ReflexCLI::CreateProject(const TemplateDefinition & base_tmpl, ArrayView <Variable> string_inputs, ArrayView <Variable> path_inputs, ArrayView <CString> targets, const WString::View & output_root, bool overwrite, System::FileHandle & std_out)
{
	auto tmpl = OpenTemplateDefinitionEx(base_tmpl.folder);
		
	AddTargetExcludes(tmpl, targets);

	Array <Variable> expanded = Join(ExpandStringVariables(tmpl, string_inputs), ExpandPathVariables(tmpl, path_inputs));

	WString name = FindVariable(expanded, "PRODUCT-NAME");

	if (!name) Bootstrap::CLI::ThrowError("product undefined");

	name = Replace(name, L' ', L'_');

	name = StripValue(name, '-');

	auto repo_name = Lowercase(name);

	auto dest_root = File::CorrectTrailingStroke(output_root);

	File::MakePath(dest_root);

	if (!System::IsDirectory(dest_root)) Bootstrap::CLI::ThrowError("could not create dest_folder");

	bool wrote_files = false;

	return InstallFolder(tmpl, expanded, tmpl.folder, dest_root, Join(repo_name, File::kStroke), overwrite, std_out, wrote_files);
}
