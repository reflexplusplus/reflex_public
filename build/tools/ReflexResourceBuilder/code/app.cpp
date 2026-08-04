#include "app.h"
#include "../../ReflexCLI/code/common.h"




//
//App impl

namespace ResourceBuilder { namespace {	//begin internal namespace

struct AppImpl : public App
{
	AppImpl()
		: App(MakeKey32("ResourceBuilder"), 0)
	{
	}

	TRef <System::Task> Compile(const WString & path, ObjectOf <Float> & progress) override
	{
		AutoRelease(progress);	//no longer supported as we are calling CLI app

		m_thread = ResourceBuilder::Compile(path);

		return m_thread;
	}

	void OnReset(Key32 context) override {}

	void OnRestore(Data::Archive::View & stream, Key32 context) override {}

	bool OnImport(UInt16 version, Data::Archive::View & stream, Key32 context) override { return false; }

	void OnStore(Data::Archive & stream) const override {}


	Reference <System::Task> m_thread;
};

} } //end internal namespace

Reflex::TRef <Reflex::System::Task> ResourceBuilder::Compile(const WString::View & path)
{
	auto exe_path = ReflexCLI::GetReflexExecutablePath(ReflexCLI::GetReflexPath());

	REFLEX_ASSERT(exe_path);
	REFLEX_ASSERT(File::Exists(exe_path));

	auto process = System::Process::Create(exe_path, { L"build-resources", L"--path", path }, { .allow_window = false });

	REFLEX_ASSERT(IsValid(process));

	return process;
}

Reflex::TRef <ResourceBuilder::App> ResourceBuilder::App::Create()
{
	return New<ResourceBuilder::AppImpl>();
}

Reflex::Output ResourceBuilder::output("ReflexResourceBuilder");
