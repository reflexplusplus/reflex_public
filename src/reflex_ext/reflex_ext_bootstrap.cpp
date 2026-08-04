
#include "bootstrap/common/bootstrap_streamable.cpp"
#include "bootstrap/common/bootstrap_global.cpp"

#ifndef REFLEX_BOOTSTRAP_TYPE_CONSOLE_APP
#include "bootstrap/common/bootstrap_ui_clipboard.cpp"
#include "bootstrap/common/bootstrap_ui_main_window.cpp"
#include "bootstrap/common/bootstrap_ui_settings_dialog.cpp"
#include "bootstrap/common/bootstrap_ui_view.cpp"
#include "bootstrap/common/bootstrap_ui_functions.cpp"

#if (REFLEX_BOOTSTRAP_VM)
#include "bootstrap/common/bootstrap_ui_vm_view.cpp"
#endif

#if defined (REFLEX_BOOTSTRAP_TYPE_AUDIOAPP) || defined (REFLEX_BOOTSTRAP_TYPE_AUDIOPLUGIN)
#include "bootstrap/app/bootstrap_app.cpp"
#include "bootstrap/audioplugin/bootstrap_audioplugin.cpp"
#include "bootstrap/audioplugin/bootstrap_param_desc.cpp"
#include "bootstrap/audioplugin/bootstrap_ui_param_control.cpp"
#elif defined(REFLEX_BOOTSTRAP_TYPE_APP)
#include "bootstrap/app/bootstrap_app.cpp"
#elif defined(REFLEX_BOOTSTRAP_TYPE_VM_APP)
#include "bootstrap/common/bootstrap_ui_vm_view.cpp"
#include "bootstrap/app/bootstrap_app.cpp"
#include "bootstrap/vm_app/bootstrap_vm_app.cpp"
#include "bootstrap/vm_app/bootstrap_vm_view.cpp"
#else
REFLEX_STATIC_ASSERT("Invalid REFLEX_BOOTSTRAP_TYPE")
#endif
#endif
