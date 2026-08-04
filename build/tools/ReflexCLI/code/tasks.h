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

	struct Variable
	{
		CString token;
		WString value;
	};

	WString CreateProject(const TemplateDefinition & tmpl, ArrayView <Variable> string_inputs, ArrayView <Variable> path_inputs, ArrayView <CString> targets, const WString::View & output_root, bool overwrite, Reflex::System::FileHandle & std_out);

	void BuildResources(const WString::View & filename, Float & progress);


	extern Output output;

}
