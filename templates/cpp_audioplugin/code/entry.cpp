#include "instance.h"
#include "view.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::AudioPlugin::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView< ::_PRODUCT-NAME-SYMBOL_::Instance, ::_PRODUCT-NAME-SYMBOL_::View >(config);

	return Bootstrap::StartAudioPlugin< ::_PRODUCT-NAME-SYMBOL_::Instance >
	(
		config,
		"_VENDOR-NAME_",
		"_PRODUCT-NAME_",
		K32("_PRODUCT-NAME-SYMBOL_"),
		__FILE__
	);
}
