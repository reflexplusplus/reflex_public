#include "reflex/reflex.h"

#include "reflex_ext/reflex_ext_core.cpp"
#include "reflex_ext/reflex_ext_async.cpp"
#include "reflex_ext/reflex_ext_file.cpp"
#if (!defined(REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP))
#include "reflex_ext/reflex_ext_glx.cpp"
#include "reflex_ext/reflex_ext_ide.cpp"
#if defined(REFLEX_BOOTSTRAP_TYPE_VM_APP) || defined(REFLEX_ENABLE_VM)
#include "reflex_ext/reflex_ext_vm.cpp"
#endif
#endif
#include "reflex_ext/reflex_ext_bootstrap.cpp"
