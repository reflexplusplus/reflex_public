#pragma once

#include "writer.h"




//
//declarations

namespace Docgen
{

	//export to structured data

	void EnumerateModules(Key32 codebase, Output & logger, Writer & w);

	void ExportSymbols(const Writer & w, Output & logger, Data::PropertySet & root);

	void ExportSymbols(Key32 language, Key32 codebase, Output & logger, const WString::View & path);
}
