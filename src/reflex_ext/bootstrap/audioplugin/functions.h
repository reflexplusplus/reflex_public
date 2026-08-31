#pragma once

#include "../../../../include/reflex_ext/bootstrap/audioplugin/functions.h"




//
//internal

namespace Reflex::Bootstrap
{

	Float32 Normalise(const ParameterDefinition & definition, Value32 value);

	Value32 Expand(const ParameterDefinition & definition, Float32 value);

}
