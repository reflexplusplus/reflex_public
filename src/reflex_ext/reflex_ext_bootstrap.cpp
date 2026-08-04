
#include "bootstrap/common/bootstrap_global.cpp"

#if defined(REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP)
#include "bootstrap/bootstrap_console_app.cpp"
#else
#include "bootstrap/common/bootstrap_streamable.cpp"
#include "bootstrap/common/bootstrap_ui_clipboard.cpp"
#include "bootstrap/common/bootstrap_ui_main_window.cpp"
#include "bootstrap/common/bootstrap_ui_settings_dialog.cpp"
#include "bootstrap/common/bootstrap_ui_view.cpp"
#include "bootstrap/common/bootstrap_ui_functions.cpp"

#if defined (REFLEX_BOOTSTRAP_VM) || defined(REFLEX_BOOTSTRAP_TYPE_VM_APP)
#include "bootstrap/common/bootstrap_ui_vm_view.cpp"
#endif

#if defined(REFLEX_BOOTSTRAP_TYPE_APP)
#include "bootstrap/bootstrap_app.cpp"
#elif defined (REFLEX_BOOTSTRAP_TYPE_AUDIOAPP) || defined (REFLEX_BOOTSTRAP_TYPE_AUDIOPLUGIN)
#include "bootstrap/bootstrap_audioplugin.cpp"
#elif defined(REFLEX_BOOTSTRAP_TYPE_VM_APP)
#include "bootstrap/bootstrap_vm_app.cpp"
#else
REFLEX_STATIC_ASSERT("Invalid REFLEX_BOOTSTRAP_TYPE")
#endif
#endif
