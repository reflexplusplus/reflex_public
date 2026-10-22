#pragma once

#include "project_gen.h"

namespace ReflexCLI::ProjectGen
{
	constexpr CString::View kGnuFloatingPointFast = "-ffast-math";

	inline WString MakeBuildScriptName(CString::View configuration)
	{
		return ToWString(Join("build_", configuration));
	}

	inline WString MakeBuildScriptName(const TargetConfiguration & config)
	{
		return MakeBuildScriptName(config.GetName());
	}

	WString GetProjectFolder(const Project & project, BuildPlatform platform);
	Pair<WString> GetLinuxDesktopPaths(const TargetConfiguration & config);	//script, generated launcher
	Array<WString> GetLinuxDesktopStageCommand(const TargetConfiguration & config);
	bool GenerateLinuxDesktopFiles(const TargetConfiguration & config);
	void CheckAssetPath(const Project & project, BuildPlatform emitter, CString::View property, const WString & path);
	WString TranslateVariables(WString::View value, ArrayView <Variable> mappings, WString::View open = L"$(", WString::View close = L")");
	Data::Archive Template(Data::Archive::View value, ArrayView <Variable> mappings);
	Data::Archive Template(const File::EmbeddedResource & resource, ArrayView <Variable> mappings);
	WString EscapeQuotedString(WString::View value, WChar additional = 0);
	WString ShellQuote(WString::View value);
	CString ShellQuote(CString::View value);
	void WriteLine(Data::Archive & output, UInt indent, CString::View line = {});
	void WriteLine(Data::Archive & output, UInt indent, WString::View line);
	void WriteCMakeInvocation(Data::Archive & output, UInt indentation, CString::View command, WString::View head = {}, ArrayView<WString> values = {});
	void WriteCMakeTargetValues(Data::Archive & output, UInt indentation, CString::View command, CString::View target, CString::View scope, ArrayView<WString> values);
	void SaveFile(const WString & path, Data::Archive::View data, BuildPlatform platform);
	void SaveMetaFile(const WString & path, Data::Archive::View data, MetaSystem system);
	void SetExecutable(const WString & path);
	void SaveText(const WString & path, CString::View text, BuildPlatform platform);
	void SaveText(const WString & path, WString::View text, BuildPlatform platform);
	Array <CString::View> GnuWarningOptions(const TargetConfiguration & config, bool gcc, bool cpp);
	Array <CString::View> ClangFloatingPointOptions(const TargetConfiguration & config);
	Array <CString::View> GnuCompileOptions(const TargetConfiguration & config, bool gcc, bool cpp, bool include_standard);
}




//
//impl

inline Reflex::CString ReflexCLI::ProjectGen::ShellQuote(CString::View text)
{
	return EncodeUTF8(ShellQuote(ToWString(text)));
}

inline Reflex::Data::Archive ReflexCLI::ProjectGen::Template(const File::EmbeddedResource & resource, ArrayView<Variable> variables)
{
	return Template(File::Extract(resource), variables);
}

inline void ReflexCLI::ProjectGen::SaveText(const WString & path, CString::View text, BuildPlatform platform)
{
	SaveFile(path, Data::Pack(text), platform);
}

inline void ReflexCLI::ProjectGen::SaveText(const WString & path, WString::View text, BuildPlatform platform)
{
	SaveFile(path, Data::EncodeUTF8(text), platform);
}
