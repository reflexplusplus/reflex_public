#pragma once

#include "project_gen.h"

namespace ReflexCLI::ProjectGen
{
	constexpr CString::View kGnuFloatingPointFast = "-ffast-math";

	WString GetProjectFolder(const Project & project, BuildPlatform platform);
	WString TranslateVariables(WString::View value, ArrayView <Variable> mappings, WString::View open = L"$(", WString::View close = L")");
	Data::Archive Template(Data::Archive::View value, ArrayView <Variable> mappings);
	Data::Archive Template(const File::EmbeddedResource & resource, ArrayView <Variable> mappings);
	WString EscapeQuotedString(WString::View value, WChar additional = 0);
	WString ShellQuote(WString::View value);
	CString ShellQuote(CString::View value);
	void WriteLine(Data::Archive & output, UInt indent, CString::View line = {});
	void WriteLine(Data::Archive & output, UInt indent, WString::View line);
	void WriteCMakeInvocation(Data::Archive & output, UInt indentation, CString::View command, WString::View head = {}, ArrayView<WString> values = {});
	void WriteCMakeTargetValues(Data::Archive & output, UInt indentation, CString::View command, CString::View target, ArrayView<WString> values);
	void SaveFile(const WString & path, Data::Archive::View data, BuildPlatform platform);
	void SaveText(const WString & path, CString::View text, BuildPlatform platform);
	void SaveText(const WString & path, WString::View text, BuildPlatform platform);
	void SaveCommandScript(const WString & filename, Data::Archive::View text, BuildPlatform platform);
	void SaveCommandScript(const WString & filename, WString::View text, BuildPlatform platform);
	Array <CString::View> GnuWarningOptions(const TargetConfiguration & config);
	Array <CString::View> ClangFloatingPointOptions(const TargetConfiguration & config);
	Array <CString::View> GnuCompileOptions(const TargetConfiguration & config, bool include_standard);
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

inline void ReflexCLI::ProjectGen::SaveCommandScript(const WString & filename, WString::View text, BuildPlatform platform)
{
	constexpr UInt32 kExecuteBits = 0111;
	SaveText(filename, text, platform);
	UInt32 permissions;
	Require(GetFilePermissions(filename, permissions), "generated project", "could not read file permissions");
	Require(SetFilePermissions(filename, permissions | kExecuteBits), "generated project", "could not set file permissions");
}

inline void ReflexCLI::ProjectGen::SaveCommandScript(const WString & filename, Data::Archive::View text, BuildPlatform platform)
{
	constexpr UInt32 kExecuteBits = 0111;
	SaveFile(filename, text, platform);
	UInt32 permissions;
	Require(GetFilePermissions(filename, permissions), "generated project", "could not read file permissions");
	Require(SetFilePermissions(filename, permissions | kExecuteBits), "generated project", "could not set file permissions");
}
