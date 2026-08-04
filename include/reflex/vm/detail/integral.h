#pragma once

#include "../[require].h"




REFLEX_NS(Reflex::VM::Detail)

using Value8 = UInt8;

using Value16 = UInt16;

using Value32 = UInt32;

using Value64 = UInt64;


template <UInt32 SIZE> struct ValueOfSize { UInt8 data[SIZE]; };

using Value128 = ValueOfSize <16>;

using Value192 = ValueOfSize <24>;

using Value256 = ValueOfSize <32>;

REFLEX_END

