#include "app.h"
#include "view.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView< ::_PRODUCT-NAME-SYMBOL_::App, ::_PRODUCT-NAME-SYMBOL_::View >(config);

	return Bootstrap::StartApp< ::_PRODUCT-NAME-SYMBOL_::App >
	(
		config,
		"_VENDOR-NAME_",
		"_PRODUCT-NAME_",
		K32("_PRODUCT-NAME-SYMBOL_"),
		__FILE__
	);
}
