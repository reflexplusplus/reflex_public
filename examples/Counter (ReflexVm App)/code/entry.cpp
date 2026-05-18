#include "reflex_ext.h"




//
//

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	return Bootstrap::StartVmApp
	(
		config,
		"Reflex Multimedia",
		"Counter (ReflexVm App)",
		K32("CounterReflexVmApp"),
		__FILE__,
		L":res:CounterReflexVmApp/main.c",
		L":res:CounterReflexVmApp/view.c"
	 );
}
