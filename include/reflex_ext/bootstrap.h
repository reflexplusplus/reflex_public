#pragma once

#if defined(REFLEX_BOOTSTRAP_TYPE_APP)
#include "bootstrap/app.h"
#elif defined(REFLEX_BOOTSTRAP_TYPE_AUDIOAPP) || defined(REFLEX_BOOTSTRAP_TYPE_AUDIOPLUGIN)
#include "bootstrap/audioplugin.h"
#elif defined(REFLEX_BOOTSTRAP_TYPE_VM_APP)
#include "bootstrap/vm_app.h"
#elif defined(REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP)
#include "bootstrap/console_app.h"
#elif defined (REFLEX_BOOTSTRAP_TYPE_LIBRARY)
#include "bootstrap/common.h"
#else
REFLEX_STATIC_ASSERT("Invalid REFLEX_BOOTSTRAP_TYPE")
#endif
