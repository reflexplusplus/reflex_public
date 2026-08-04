#include "reflex_ext.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	return Bootstrap::StartVmApp
	(
		config,
		"_VENDOR-NAME_",
		"_PRODUCT-NAME_",
		K32("_PRODUCT-NAME-SYMBOL_"),
		__FILE__,
		L":res:_PRODUCT-NAME-SYMBOL_/main.c",
		L":res:_PRODUCT-NAME-SYMBOL_/view.c"
	);
}
