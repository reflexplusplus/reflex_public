#include "app.h"
#include "view.h"




//
//entrypoint

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	Bootstrap::PublishAppView< ::NotesCppApp::App, ::NotesCppApp::View >(config);
	
	return Bootstrap::StartApp< ::NotesCppApp::App >
	(
		config,
		"Reflex++",
		"Notes (Cpp App)",
		MakeKey32("NotesCppApp"),
		__FILE__
	 );
}
