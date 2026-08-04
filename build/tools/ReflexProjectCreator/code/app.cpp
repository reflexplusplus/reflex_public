#include "app.h"




namespace ReflexProjectCreator { namespace {	//begin internal namespace

struct AppImpl : public App
{
	AppImpl();

	WString::View GetReflexPath() const override;

	//void SetReflexPath(const WString & path) override;

	ArrayView <TemplateDefinition> GetTemplates() const override;

	void RefreshTemplates() override;

	void InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, const WString & dest) override;

	void RunTask(CString::View cmd, Array <WString> && args, const Function <bool(const Data::Archive & output)> & done = {});


	void OnReset(Key32 context) override {}

	void OnRestore(Data::Archive::View & stream, Key32 context) override;

	void OnStore(Data::Archive & stream) const override;


	WString m_reflex_path;

	Array <TemplateDefinition> m_templates;


	struct Job
	{ 
		Reference <Object> clock;
		Reference <System::Process> task;
	};

	Map <Key32,Job> m_jobs;

	Reference <Object> m_session_listener;
};

AppImpl::AppImpl()
	: App(MakeKey32("ProjectCreator"), 1)
	, m_reflex_path(ReflexCLI::GetReflexPath())
{
	RefreshTemplates();
}

WString::View AppImpl::GetReflexPath() const
{
	return m_reflex_path;
}

//void AppImpl::SetReflexPath(const WString & path)
//{
//	m_reflex_path = path;
//
//	Data::SetWString(Bootstrap::global->prefs, kReflexPathPref, m_reflex_path);
//
//	RefreshTemplates();
//}

ArrayView <TemplateDefinition> AppImpl::GetTemplates() const
{
	return m_templates;
}

void AppImpl::RefreshTemplates()
{
	m_templates.Clear();

	RunTask("list-templates", {}, [this](const Data::Archive & output)
	{
		Array <TemplateDefinition> templates;

		auto root = Data::DecodePropertySet(Data::kPropertySheetFormat, output);

		for (auto & i : root.Iterate<Data::PropertySet>())
		{
			templates.Push(ReflexCLI::DecodeTemplate(i.value));
		}

		return SetFiltered(m_templates, templates);
	});
}

void AppImpl::InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, const WString & dest)
{
	Array <WString> args;

	args.Push(L"--template");
	args.Push(File::SplitFilename(File::RemoveTrailingStroke(tmpl.folder)).b);

	for (auto & [key, value] : inputs)
	{
		args.Push(Join(L"--", ToWString(key)));
		args.Push(ToWString(value));
	}

	args.Push(L"--targets");
	args.Push(L"all");
	args.Push(L"--output");
	args.Push(dest);

	RunTask("create", std::move(args), [dest](const Data::Archive & output)
	{
		System::Open(dest);

		if (auto error = Data::Unpack<CString::View>(output))
		{
			ReflexProjectCreator::output.Error(error);
		}

		return true;
	});
}

void AppImpl::RunTask(CString::View cmd, Array <WString> && args, const Function <bool(const Data::Archive & output)> & done)
{
	auto global = Bootstrap::global;

	WString capture_path = Bootstrap::Detail::MakeProductPath(System::kPathUserData, global->vendor, global->product, Join(ToWString(System::GetTime()), L".propertysheet"));

	auto output = New<System::FileHandle>(capture_path, System::FileHandle::kModeOverwrite);
	
	Key32 id = cmd;

	auto & job = m_jobs.Acquire(id);

	args.Push(L"--cmd");
	args.Push(ToWString(cmd));
	args.Push(L"--detail");
	args.Push(L"true");

	job.task = System::Process::Create(ReflexCLI::GetReflexExecutablePath(m_reflex_path), args, { .std_out = output.Adr(), .allow_window = false });

	REFLEX_ASSERT(job.task->Status());

	job.clock = Async::CreatePeriodicClock(0.25f, [this, id, done, capture_path, &job, output]()
	{
		if (job.task->Completed())
		{
			job.task.Clear();	//discard process, releases output handle, file gets closed

			bool update = done(File::Open(capture_path));

			System::Delete(capture_path);

			m_jobs.Unset(id);

			if (update) Notify(false);
		}
	});

	Notify(false);
}

void AppImpl::OnRestore(Data::Archive::View & stream, Key32 context)
{
	Data::Deserialize(stream, m_templates);
}

void AppImpl::OnStore(Data::Archive & stream) const
{
	Data::Serialize(stream, m_templates);
}

} } //end internal namespace

Reflex::Output ReflexProjectCreator::output("ProjectCreator");

Reflex::TRef <ReflexProjectCreator::App> ReflexProjectCreator::App::Create()
{
	return New<ReflexProjectCreator::AppImpl>();
}
