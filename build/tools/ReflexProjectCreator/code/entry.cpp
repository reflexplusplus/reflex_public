#include "app.h"
#include "view.h"





Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView<ReflexProjectCreator::App, ReflexProjectCreator::View>(config);

	return Bootstrap::StartApp<ReflexProjectCreator::App>
	(
		config,
		"Reflex++",
		"Project Creator",
		K32("ReflexProjectCreator"),
		__FILE__
	);
}
