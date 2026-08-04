#pragma once

#include "common/streamable.h"
#include "common/global.h"
#include "common/app.h"
#include "common/functions.h"
#include "common/entry.h"
#include "console_app.h"

// Distributions without the VM (the public SDK) ship no bootstrap/common/vm.h,
// matching how reflex/vm.h guards its own contents.
#if __has_include("common/vm.h")
#include "common/vm.h"
#endif

#include "common/ui/detail.h"
#include "common/ui/functions.h"
#include "common/ui/script_object.h"
#include "common/ui/view.h"
