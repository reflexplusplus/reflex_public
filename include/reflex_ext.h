#pragma once

#include "reflex_ext/data.h"
#include "reflex_ext/file.h"
#include "reflex_ext/async.h"
#include "reflex_ext/glx.h"

// Distributions without the VM (the public SDK) ship no reflex_ext/vm.h,
// matching how reflex/vm.h guards its own contents.
#if __has_include("reflex_ext/vm.h")
#include "reflex_ext/vm.h"
#endif

#include "reflex_ext/ide.h"
#include "reflex_ext/bootstrap.h"
