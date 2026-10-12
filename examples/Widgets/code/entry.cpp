#include "app.h"
#include "view.h"

Reflex::Unretained <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> &, Configuration & config)
{
	Bootstrap::PublishAppView<::Widgets::App,::Widgets::View>(config);

	return Bootstrap::StartApp<::Widgets::App>
	(
		config, "Reflex++", "Widgets", MakeKey32("Widgets"), __FILE__
	);
}
