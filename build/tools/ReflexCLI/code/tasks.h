#pragma once

#include "common.h"




//
//declarations

namespace ReflexCLI
{

	struct Variable
	{
		CString token;
		WString value;
	};

	WString CreateProject(const TemplateDefinition & tmpl, ArrayView <Variable> string_inputs, ArrayView <Variable> path_inputs, CString::View generate, const WString::View & output_root, bool overwrite, Reflex::System::FileHandle & std_out);

	void BuildResources(const WString::View & filename, Float & progress);


	extern Output output;

}
