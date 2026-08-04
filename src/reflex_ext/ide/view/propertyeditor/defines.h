#pragma once

#include "../[include].h"




//
//impl standard maps used by propertyeditor to map names of keys and types, to make it human readable

REFLEX_NS(Reflex::File)

extern const Key32 kID;

enum Type { kTypeUInt8, kTypeUInt16, kTypeUInt32, kTypeUInt64, kTypeFloat32, kTypeBinary, kTypeKey32, kTypeCString, kNumType };

REFLEX_END
