#pragma once

#include "core/preprocessor.h"

#include "core/module.h"

#include "core/warnings.h"
#include "core/types.h"
#include "core/defines.h"

#include "core/assert.h"

#include "core/meta.h"
#include "core/compare_policy.h"

#include "core/functions/cast.h"
#include "core/functions/bit.h"
#include "core/functions/logic.h"
#include "core/functions/math.h"
#include "core/functions/iterate.h"
#include "core/functions/memory.h"

#include "core/idx.h"
#include "core/tuple.h"
#include "core/optional.h"
#include "core/geometry.h"

#include "core/object.h"
#include "core/allocator.h"

#include "core/allocation.h"
#include "core/array.h"
#include "core/sequence.h"

#include "core/string.h"
#include "core/key.h"

#include "core/function.h"

#include "core/state.h"
#include "core/queue.h"
#include "core/list.h"
#include "core/node.h"
#include "core/signal.h"

#include "core/detail.h"

#include "core/debug.h"

#include "core/legacy/flags.h"




//
//null

namespace Reflex::Detail { template <> struct ExternalNull <DynamicTypeInfo> {}; }

REFLEX_EXTERN_NULL(Reflex::Allocation <UInt8>);
REFLEX_EXTERN_NULL(Reflex::Allocation <UInt32>);
