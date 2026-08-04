#include "module.h"




//
//module

Reflex::Detail::Module Reflex::System::Common::g_module = { "Reflex::System", Reflex::root_module };

const Reflex::Detail::Module & Reflex::System::module = Reflex::System::Common::g_module;

