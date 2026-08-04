#pragma once

#include "../../[require].h"




//
//declarations

REFLEX_NS(Reflex::Data::Detail)

template <class TYPE> using DynamicIteratorOfType = typename Sequence< Address, Reference <TYPE> >::Range;

REFLEX_END
