#include "app.h"
#include "view.h"




//
//

namespace ResourceBuilder { namespace {	//begin internal namespace

void CompileFromCmdLine(const ArrayView <CString::View> & cmds)
{
	auto global = AutoRelease(Bootstrap::Global::Acquire("Reflex++", "Resource Builder", Bootstrap::Detail::ExtractProjectDir(__FILE__), K32("ResourceBuilder")));

	CString cmdline = Merge(cmds, ' ');

	CString::View unquoted = Trim<char>(cmdline, [](char c)	//remove quotes
	{
		return c == '"';
	});

	WString path = ToWString(unquoted);

	path = File::CorrectStrokes(path);	//windows to reflex

	path = File::RemoveDuplicateStrokes(path);	//remove "//" as these will mess up ResolveRelativePath

	path = File::ResolveRelativePath(path);

	auto std_out = Make<System::FileHandle>(System::FileHandle::kStandardStreamOut);

	File::WriteLine(std_out, Join("warning: ReflexResourceBuilder is deprecated. Use: reflex build-resources --path \"", ToCString(path), '"'));

	auto task = AutoRelease(Compile(path));

	task->Wait();
}

} }

Reflex::TRef <Reflex::Object> Reflex::System::App::OnStart(const ArrayView <CString::View> & cmdline, Configuration & config)
{
	if (cmdline)
	{
		Output::SetOutputFile(New<System::FileHandle>(System::FileHandle::kStandardStreamOut));

		ResourceBuilder::CompileFromCmdLine(cmdline);

		return Object::null;	//for desktop app builds, returning null object here will behave like a console (terminate immediately, instead of starting message loop)
	}
	else
	{
		Bootstrap::PublishAppView<ResourceBuilder::App,ResourceBuilder::View>(config);

		return Bootstrap::StartApp<ResourceBuilder::App>
		(
			config,
			"Reflex++",
			"Resource Builder",
			MakeKey32("ResourceBuilder"),
			__FILE__
		);
	}
}
