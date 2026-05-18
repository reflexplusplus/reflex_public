#include "app.h"
#include "view.h"




//
//entrypoint

TRef <Object> System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView< ::NotesCppApp::App, ::NotesCppApp::View >(config);
	
	return Bootstrap::StartApp< ::NotesCppApp::App >
	(
		config,
		"Reflex Multimedia",
		"Notes (Cpp App)",
		K32("NotesCppApp"),
		__FILE__
	 );
}
