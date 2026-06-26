#pragma once

#include "reflex/system/ext/com.h"

#define MS_TRY(fn, args) /*Common::output(REFLEX_STRINGIFY(Direct3D::##fn));*/ if (!SUCCEEDED(fn##args)) { throw (false); }

