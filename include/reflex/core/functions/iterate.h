#pragma once

#include "../detail/iterate.h"




//
//Secondary API

REFLEX_NS(Reflex)

template <class TYPE> auto Reverse(TYPE && iterable) { return Detail::RangeHolder(iterable.rbegin(), iterable.rend()); }

REFLEX_END
