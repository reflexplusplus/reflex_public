#pragma once

#include "[require].h"




//
//preprocessor

#ifndef DOCGEN
#define DOCGEN REFLEX_DEBUG
#endif




//
//declarations

namespace Docgen
{

	using namespace Reflex;

	using namespace Docformat;


	class Module;

	class Writer;


	struct Item;

	struct TypeItem;
	struct TypedefItem;
	struct GlobalItem;
	struct FunctionItem;
	struct MethodItem;
	struct MemberItem;
}
