#pragma once

#include "core.h"




//
//

REFLEX_NS(Reflex::VM)

inline const Symbol kMap = { kGlobal, K32("Map") };

template <class KEY, class VALUE, bool CIRCULAR>
struct AbstractMap :
	public Object,
	public Detail::CircularT<CIRCULAR>,
	public Sequence <KEY,VALUE>
{
protected:

	AbstractMap(VM::Context & context, VM::TypeRef map_t)
		: Detail::CircularT<CIRCULAR>(context, *this)
	{
		Detail::FinaliseObject(*this, map_t);
	}
};

REFLEX_END
