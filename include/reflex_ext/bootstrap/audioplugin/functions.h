#pragma once

#include "parameter.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	[[nodiscard]] Unretained <ParameterDefinition> DefineContinuousParameter(WString && name, Float32 min, Float32 max, Float32 step, Float32 initial, UInt8 group_flags, FunctionPointer<WString(Value32)> to_string = &ContinuousToWString);

	[[nodiscard]] Unretained <ParameterDefinition> DefineDiscreteParameter(WString && name, Int32 min, Int32 max, Int32 initial, UInt8 group_flags);

	[[nodiscard]] Unretained <ParameterDefinition> DefineEnumParameter(WString && name, ArrayView <WString> values, Int32 initial, UInt8 group_flags);

	[[nodiscard]] Unretained <ParameterDefinition> DefineBoolParameter(WString && name, bool initial, UInt8 group_flags);

}
