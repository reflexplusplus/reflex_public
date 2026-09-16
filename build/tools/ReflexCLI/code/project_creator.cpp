#include "project_gen.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI)

struct Targets
{
	Array <WString> exclude_folders;
	Array <WString> exclude_files;
	Array <WString> replace_types;
	Array <CString> replace_vars;
};

struct TemplateDefinitionEx : public TemplateDefinition
{
	Array<Variable> path_generators;
	Array<Variable> string_generators;
	Targets targets;
};

struct Substitution
{
	Data::Archive from;
	Data::Archive to;
};

void AppendCStringArray(const Data::PropertySet & config, Array <CString> & generators, Key32 id)
{
	generators.Append(GetCStrings(config, id));
}

void AppendCStringArray(const Data::PropertySet & config, Array <WString> & generators, Key32 id)
{
	for (auto & value : GetCStrings(config, id)) generators.Push(ToWString(value));
}

WString EvaluateTemplateExpression(const TemplateDefinitionEx & tmpl, WString::View value, ArrayView<Variable> variables)
{
	return EvaluateVariableExpressions(value, variables, kVariableSyntaxTemplate, {});
}

Array<Variable> ExpandGeneratedVariables(const TemplateDefinitionEx & tmpl, ArrayView<Variable> generators, ArrayView<Variable> inputs, bool normalize_paths)
{
	Array<Variable> scope = inputs;
	for (auto & generator : generators) scope.Push(generator);

	Array<Variable> result;
	for (auto & generator : generators)
	{
		auto value = EvaluateTemplateExpression(tmpl, generator.value, scope);
		if (normalize_paths)
		{
			value = File::CorrectStrokes(value);
			File::RemoveTrailingStroke(value);
		}
		result.Push({ generator.name, std::move(value) });
	}
	return result;
}

Array <Substitution> BuildSubstitutionList(ArrayView <CString> tokens, const Array <Variable> & variables)
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
		if (auto variable = FindVariable(variables, token))
		{
			substitutions.Push({ Data::Pack(VariableReference(token, kVariableSyntaxTemplate)), Data::EncodeUTF8(variable->value) });
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
	return True(Search<CaseInsensitive>(extensions, File::SplitExtension(path).b));
}

bool CanWrite(const WString::View & path, System::FileHandle & std_in, System::FileHandle & std_out, const Function <bool(const WString&)> & overwrite)
{
	constexpr WString::View kProtected[] = { L"cfg", L"h", L"cpp", L"c", L"glx"};

	if (System::Exists(path))
	{
		if (!Search<CaseInsensitive>(kProtected, File::SplitExtension(path).b)) return true;

		return overwrite(path);
	}

	return true;
}

WString InstallFolder(const TemplateDefinitionEx & tmpl, const Array <Variable> & variables, const WString::View & src_path, const WString::View & dest_path, const WString::View & dest_name, bool & wrote_files, System::FileHandle & std_in, System::FileHandle & std_out, const Function <bool(const WString&)> & overwrite)
{
	auto content_substitutions = BuildSubstitutionList(tmpl.targets.replace_vars, variables);
	auto copy_permissions = [](WString::View source_path, WString::View dest_path)
	{
		constexpr UInt32 kExecuteBits = 0111;
		UInt32 source_permissions, dest_permissions;
		return GetFilePermissions(source_path, source_permissions) &&
			GetFilePermissions(dest_path, dest_permissions) &&
			SetFilePermissions(dest_path, (dest_permissions & ~kExecuteBits) | (source_permissions & kExecuteBits));
	};

	auto dst = Join(dest_path, dest_name);

	bool wrote_files_here = false;

	auto [folders, files] = File::List(src_path, true);

	for (auto & folder : folders)
	{
		auto folder_path = Join(src_path, folder.key);
		auto relative_folder = File::MakeRelativePath(tmpl.folder, folder_path);
		if (!Search(tmpl.targets.exclude_folders, folder.key) && !Search(tmpl.targets.exclude_folders, relative_folder))
		{
			bool wrote_files_beneath = false;

			InstallFolder(tmpl, variables, folder_path, dst, folder.key, wrote_files_beneath, std_in, std_out, overwrite);

			wrote_files_here |= wrote_files_beneath;
		}
	}

	for (auto & file : files)
	{
		if (!Search(tmpl.targets.exclude_files, file.key))
		{
			auto file_path = Join(src_path, file.key);
			auto dst_path = Join(dst, file.key);

			if (!CanWrite(dst_path, std_in, std_out, overwrite))
			{
				Bootstrap::CLI::Print(std_out, Bootstrap::CLI::kColourBrightBlack, Join("skipping ", Data::Unpack<CString::View>(Data::EncodeUTF8(dst_path))));
			}
			else
			{
				if (SetFiltered(wrote_files_here, true))
				{
					File::MakePath(dst);

					Require(System::IsDirectory(dst), "could not create", dst);
				}

				if (HasListedExtension(file.key, tmpl.targets.replace_types))
				{
					auto bytes = ReplaceAll(File::Open(file_path), content_substitutions);

					Require(SaveGeneratedFile(dst_path, bytes), "failed to write", dst_path);

					Require(copy_permissions(file_path, dst_path), "failed to copy permissions", dst_path);
				}
				else if (!File::Copy(file_path, dst_path))
				{
					ThrowError("failed to copy", file_path);
				}
				else if (!copy_permissions(file_path, dst_path))
				{
					ThrowError("failed to copy permissions", dst_path);
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

Reflex::WString ReflexCLI::CreateProject(const TemplateDefinition & base_tmpl, ArrayView <Variable> string_inputs, ArrayView <Variable> path_inputs, ArrayView <CString::View> targets, const WString & destination, System::FileHandle & std_in, System::FileHandle & out, const Function <bool(const WString&)> & overwrite)
{
	TemplateDefinitionEx tmpl;
	auto config = OpenTemplateCfg(base_tmpl.folder);
	Cast<TemplateDefinition>(tmpl) = DecodeTemplate(config);

	auto generate = Data::GetPropertySet(config, "generate");
	auto keymap = Data::GetKeyMap(config);
	tmpl.path_generators = DecodeVariables(generate, "paths", keymap);
	tmpl.string_generators = DecodeVariables(generate, "strings", keymap);

	auto exclude = Data::GetPropertySet(config, "exclude");
	AppendCStringArray(exclude, tmpl.targets.exclude_folders, "folders");
	AppendCStringArray(exclude, tmpl.targets.exclude_files, "files");

	auto replace = Data::GetPropertySet(config, "replace");
	AppendCStringArray(replace, tmpl.targets.replace_types, "types");
	AppendCStringArray(replace, tmpl.targets.replace_vars, "vars");

	if (!targets) Bootstrap::CLI::ThrowError("invalid --target value");
	Array<Variable> expanded = string_inputs;
	auto template_path = File::RemoveTrailingStroke(base_tmpl.folder);
	auto templates_path = File::SplitFilename(template_path).a;
	auto library_path = File::RemoveTrailingStroke(File::SplitFilename(File::RemoveTrailingStroke(templates_path)).a);
	expanded.Push({ "TEMPLATE_LIBRARY_PATH", std::move(library_path) });
	for (auto & path : path_inputs)
	{
		auto value = File::CorrectStrokes(path.value);
		File::RemoveTrailingStroke(value);
		expanded.Push({ path.name, std::move(value) });
	}
	for (auto & variable : ExpandGeneratedVariables(tmpl, tmpl.string_generators, expanded, false)) expanded.Push(std::move(variable));
	for (auto & variable : ExpandGeneratedVariables(tmpl, tmpl.path_generators, expanded, true)) expanded.Push(std::move(variable));
	File::MakePath(destination);

	if (!System::IsDirectory(destination)) Bootstrap::CLI::ThrowError("could not create dest_folder");

	bool wrote_files = false;
	auto folder = InstallFolder(tmpl, expanded, tmpl.folder, destination, {}, wrote_files, std_in, out, overwrite);

	ProjectGen::Generate(Join(folder, L"project.cfg"), targets, out);

	return folder;
}
