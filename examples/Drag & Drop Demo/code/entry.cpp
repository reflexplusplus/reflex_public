#include "app.h"
#include "view.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView<::DragDropDemo::App,::DragDropDemo::View>(config);

	return Bootstrap::StartApp<::DragDropDemo::App>
	(
		config,
		"Reflex Multimedia",
		"Drag & Drop Demo",
		K32("DragDropDemo"),
		__FILE__
	);
}
