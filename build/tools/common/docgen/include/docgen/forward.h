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

	class Module;

	class Writer;


	struct Symbol;	//64 bit key for indexing


	struct Item;

	struct TypeItem;


	template <class TYPE_ID> struct AbstractField;

}
