#include "app.h"
#include "view.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView<::DragAndDrop::App,::DragAndDrop::View>(config);

	return Bootstrap::StartApp<::DragAndDrop::App>
	(
		config,
		"Reflex++",
		"Drag And Drop",
		MakeKey32("DragAndDrop"),
		__FILE__
	);
}
