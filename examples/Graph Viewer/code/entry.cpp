#include "app.h"
#include "view.h"




//
//entrypoint

TRef <Object> System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView<::GraphViewer::App,::GraphViewer::View>(config);

	return Bootstrap::StartApp<::GraphViewer::App>
	(
		config,
		"Reflex++",
		"Graph Viewer",
		MakeKey32("GraphViewer"),
		__FILE__
	 );
}
