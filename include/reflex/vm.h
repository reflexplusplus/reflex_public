#pragma once

#include "vm/[require].h"

#include "vm/library.h"
#include "vm/detail.h"

#include "vm/string.h"

#include "vm/module.h"
#include "vm/program.h"
#include "vm/context.h"

#include "vm/error.h"
#include "vm/compiler.h"
#include "vm/functions.h"

#include "vm/stack.h"

#include "vm/detail/mt.h"

#include "vm/bind.h"

#include "vm/bindings/node.h"
#include "vm/bindings/core.h"
#include "vm/bindings/data.h"

#if REFLEX_INCLUDE_UI
#include "vm/bindings/glx.h"
#endif
