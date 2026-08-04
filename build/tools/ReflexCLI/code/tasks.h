#pragma once

#include "common.h"




//
//declarations

namespace ReflexCLI
{

	struct StringCompare
	{
		static bool eq(CString::View a, CString::View b);
	};


	void Install(CString::View version, const Array <CString::View> & platforms, const WString & path, bool test, System::FileHandle & std_out);

	void ListVersions(Reflex::System::FileHandle & std_out);

	void GetVersion(System::FileHandle & std_out);

	void SetPath(const WString & path, System::FileHandle & std_out);

	void Doc(const Data::PropertySet & args, System::FileHandle & std_out);

	void DocHelp(System::FileHandle & std_out);


	enum Target : UInt8
	{
		kTargetWindows,
		kTargetMacOS,
		kTargetIOS,
		kTargetAndroid,
		kTargetCMake
	};

	constexpr CString::View kTargets[] = { "windows", "macos", "ios", "android", "cmake" };

	struct Variable
	{
		CString token;
		WString value;
	};

	WString CreateProject(const TemplateDefinition & tmpl, ArrayView <Variable> string_inputs, ArrayView <Variable> path_inputs, ArrayView <CString> targets, const WString & destination, bool overwrite, Reflex::System::FileHandle & std_out);


	void BuildResources(const WString::View & filename, Float & progress);
	
	void BuildPlist(const Data::PropertySet & args, Reflex::System::FileHandle & std_out);


	extern Output output;


	inline const auto & kColourDim = Bootstrap::CLI::Detail::kColours[Bootstrap::CLI::kColourBrightBlack];

	inline const auto & kColourDefault = Bootstrap::CLI::Detail::kColours[Bootstrap::CLI::kColourDefault];


	inline void PrintCommandWithDescription(System::FileHandle & std_out, CString::View name, CString::View description)
	{
		File::WriteLine(std_out, Join(kColourDefault, name, ' ', kColourDim, description, kColourDefault));
	}

}
