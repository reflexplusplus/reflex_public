#include "app.h"
#include "view.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView<::CustomDrawing::App,::CustomDrawing::View>(config);

	return Bootstrap::StartApp<::CustomDrawing::App>
	(
		config,
		"Reflex Multimedia",
		"Custom Drawing",
		K32("CustomDrawing"),
		__FILE__
	);
}
