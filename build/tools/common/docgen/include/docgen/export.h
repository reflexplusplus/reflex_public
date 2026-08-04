#pragma once

#include "data.h"
#include "writer.h"




//
//declarations

namespace Docgen
{

	//export to structured data

	void EnumerateModules(Key32 codebase, Output & logger, Writer & w);

	void ExportSymbols(const Writer & w, Output & logger, Data::PropertySet & root);

	void ExportSymbols(Key32 language, Key32 codebase, Output & logger, const WString::View & path);



	//unpack kData

	Field RestoreField(const Data::PropertySet & node);

	decltype(Docgen::FunctionItem::overloads) UnpackFunctionSignatures(const Data::PropertySet & in);

}
