#include "app.h"
#include "view.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView<::SVGDemo::App,::SVGDemo::View>(config);
		
	return Bootstrap::StartApp<::SVGDemo::App>
	(
		config,
		"Reflex++",
		"SVG Demo",
		MakeKey32("SVGDemo"),
		__FILE__
	);
}
