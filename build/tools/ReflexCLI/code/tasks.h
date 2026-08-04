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


	struct Variable
	{
		CString token;
		WString value;
	};

	WString CreateProject(const TemplateDefinition & tmpl, ArrayView <Variable> string_inputs, ArrayView <Variable> path_inputs, ArrayView <CString> targets, const WString::View & output_root, bool overwrite, Reflex::System::FileHandle & std_out);


	void BuildResources(const WString::View & filename, Float & progress);
	
	void BuildPlist(const Data::PropertySet & args, Reflex::System::FileHandle & std_out);


	extern Output output;


	inline const auto & kColourDim = Bootstrap::CLI::Detail::kColours[Bootstrap::CLI::kColourBrightBlack];

	inline const auto & kColourDefault = Bootstrap::CLI::Detail::kColours[Bootstrap::CLI::kColourDefault];

}
