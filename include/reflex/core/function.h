#pragma once

#include "function/function_pointer.h"

#if REFLEX_USE_STD_FUNCTION
#include "function/std_function.h"
#include "function/std_bind.h"
#else
#include "function/reflex_function.h"
#include "function/reflex_bind.h"
#endif
