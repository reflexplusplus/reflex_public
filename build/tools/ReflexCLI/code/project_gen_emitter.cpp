#include "project_gen_emitter.h"

REFLEX_BEGIN_INTERNAL(ReflexCLI::ProjectGen)

constexpr CString::View kGnuWarningNone = "-w";
constexpr CString::View kGnuWarningAll = "-Wall";
constexpr CString::View kGnuWarningExtra = "-Wextra";
constexpr CString::View kGnuWarningUnusedParameter = "-Wno-unused-parameter";
constexpr CString::View kGnuWarningPedantic = "-Wpedantic";

constexpr CString::View kGnuStandards[] = { "-std=c++17", "-std=c++20" };
constexpr CString::View kGnuOptimizations[] = { "-O0", "-Os", "-O2", "-O3" };

constexpr CString::View kClangFloatingPointStrict = "-ffp-model=strict";

[[maybe_unused]] bool HasTemplateVariable(WString::View value)
{
	for (UInt index = 0; index + 2 < value.size; ++index)
	{
		if (value[index] != L'[' || !((value[index + 1] >= L'A' && value[index + 1] <= L'Z') || value[index + 1] == L'_')) continue;

		UInt end = index + 2;
		while (end < value.size && ((value[end] >= L'A' && value[end] <= L'Z') || (value[end] >= L'0' && value[end] <= L'9') || value[end] == L'_')) ++end;
		if (end < value.size && value[end] == L']') return true;
	}
	return false;
}

REFLEX_END_INTERNAL

Reflex::WString ReflexCLI::ProjectGen::GetProjectFolder(const Project & project, BuildPlatform platform)
{
	return Join(project.GetRoot(), project.GetGeneratedDirectory(), ToWString(kBuildPlatforms[platform]), File::kStroke);
}

Reflex::WString ReflexCLI::ProjectGen::TranslateVariables(WString::View value, ArrayView<Variable> mappings, WString::View open, WString::View close)
{
	WString result = value;
	for (auto & mapping : mappings) result = Replace(result, Join(open, ToWString(mapping.name), close), mapping.value);
	return result;
}

Reflex::Data::Archive ReflexCLI::ProjectGen::Template(Data::Archive::View value, ArrayView<Variable> variables)
{
	auto result = TranslateVariables(Data::DecodeUTF8(value), variables, L"[", L"]");
	REFLEX_ASSERT(!HasTemplateVariable(result));
	return Data::EncodeUTF8(result);
}

Reflex::WString ReflexCLI::ProjectGen::EscapeQuotedString(WString::View value, WChar additional)
{
	REFLEX_ASSERT(!Search(value, L'\r') && !Search(value, L'\n'));

	WString result;
	for (auto c : value)
	{
		if (c == L'\\' || c == L'"' || c == additional) result.Push(L'\\');
		result.Push(c);
	}
	return result;
}

Reflex::WString ReflexCLI::ProjectGen::ShellQuote(WString::View value)
{
	WString result = L"'";
	for (auto c : value)
	{
		if (c == '\'') result.Append(L"'\\''");
		else result.Push(c);
	}
	result.Push('\'');
	return result;
}

void ReflexCLI::ProjectGen::WriteLine(Data::Archive & output, UInt indent, CString::View line)
{
	auto extend = Extend(output, indent);
	Fill(extend, UInt8('\t'));
	Data::WriteLine(output, line);
}

void ReflexCLI::ProjectGen::WriteLine(Data::Archive & output, UInt indent, WString::View line)
{
	auto extend = Extend(output, indent);
	Fill(extend, UInt8('\t'));
	Data::WriteLine(output, line);
}

void ReflexCLI::ProjectGen::WriteCMakeInvocation(Data::Archive & output, UInt indentation, CString::View command, WString::View head, ArrayView<WString> values)
{
	auto opening = Join(ToWString(command), L"(", head);
	switch (values.size)
	{
	case 0:
		WriteLine(output, indentation, Join(opening, L")"));
		return;
	case 1:
		WriteLine(output, indentation, Join(opening, head ? L" " : L"", values.GetFirst(), L")"));
		return;
	default:
		WriteLine(output, indentation, opening);
		for (auto & value : values) WriteLine(output, indentation + 1, value);
		WriteLine(output, indentation, ")");
	}
}

void ReflexCLI::ProjectGen::WriteCMakeTargetValues(Data::Archive & output, UInt indentation, CString::View command, CString::View target, CString::View scope, ArrayView<WString> values)
{
	if (values) WriteCMakeInvocation(output, indentation, command, ToWString(Join(target, " ", scope)), values);
}

void ReflexCLI::ProjectGen::SaveFile(const WString & path, Data::Archive::View data, BuildPlatform platform)
{
	auto existing = File::Open(path);

	if (existing != data)
	{
		File::MakePath(File::SplitFilename(path).a);

		if (!File::Save(path, data)) Bootstrap::CLI::ThrowError(Join(kBuildPlatforms[platform], ": failed to write '", EncodeUTF8(path), "'"));
	}
}

Reflex::Array<Reflex::CString::View> ReflexCLI::ProjectGen::GnuWarningOptions(const TargetConfiguration & config)
{
	REFLEX_STATIC_ASSERT(kWarningLevelCount == 4);
	auto warning_level = config.GetEnum<WarningLevel>(kWarningLevel, kWarningLevelNames, kWarningLevel_standard);
	Array<CString::View> result;

	switch (warning_level)
	{
	case kWarningLevel_none:
		result = { kGnuWarningNone };
		break;

	case kWarningLevel_relaxed:
		result = { kGnuWarningAll, kGnuWarningUnusedParameter };
		break;

	case kWarningLevel_standard:
		result = { kGnuWarningAll, kGnuWarningExtra, kGnuWarningUnusedParameter };
		break;

	case kWarningLevel_pedantic:
		result = { kGnuWarningAll, kGnuWarningExtra, kGnuWarningPedantic };
		break;

	default:
		REFLEX_ASSERT(false);
		break;
	}

	if (warning_level != kWarningLevel_pedantic)
	{
		result.Push("-Wno-sign-compare");
		result.Push("-Wno-missing-field-initializers");
		result.Push("-Wno-char-subscripts");
		result.Push("-Wno-missing-braces");
	}
	return result;
}

Reflex::Array<Reflex::CString::View> ReflexCLI::ProjectGen::ClangFloatingPointOptions(const TargetConfiguration & config)
{
	REFLEX_STATIC_ASSERT(kFloatingPointCount == 4);

	switch (config.GetEnum<FloatingPoint>(kFloatingPoint, kFloatingPointNames, kFloatingPoint_default))
	{
	case kFloatingPoint_default:
	case kFloatingPoint_precise:
		return {};

	case kFloatingPoint_fast:
		return { kGnuFloatingPointFast };

	case kFloatingPoint_strict:
		return { kClangFloatingPointStrict };

	default:
		REFLEX_ASSERT(false);
		return {};
	}
}

Reflex::Array <Reflex::CString::View> ReflexCLI::ProjectGen::GnuCompileOptions(const TargetConfiguration & config, bool include_standard)
{
	REFLEX_STATIC_ASSERT(GetArraySize(kGnuStandards) == kCppStandardCount);
	REFLEX_STATIC_ASSERT(GetArraySize(kGnuOptimizations) == kOptimizationCount);

	Array<CString::View> result;
	auto cpp_standard = config.GetEnum<CppStandard>(kCppStandard, kCppStandardNames, kCppStandard_cxx20);
	auto optimization = config.GetEnum<Optimization>(kOptimization, kOptimizationNames, kOptimization_none);
	auto optimized = optimization != kOptimization_none;
	if (include_standard) result.Push(kGnuStandards[cpp_standard]);
	if (!config.GetBool(kRtti, true)) result.Push("-fno-rtti");
	for (auto option : GnuWarningOptions(config)) result.Push(option);
	result.Push(kGnuOptimizations[optimization]);
	if (config.GetBool(kDebugInformation, !optimized)) result.Push("-g");
	if (config.GetBool(kDeadStrip, optimized))
	{
		result.Push("-ffunction-sections");
		result.Push("-fdata-sections");
	}
	return result;
}
