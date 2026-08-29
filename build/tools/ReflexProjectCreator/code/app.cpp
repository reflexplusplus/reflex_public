#include "app.h"
#include "../../ReflexCLI/code/tasks.h"




namespace ReflexProjectCreator { namespace {	//begin internal namespace

struct AppImpl : public App
{
	AppImpl();

	WString::View GetReflexPath() const override;

	ArrayView <TemplateDefinition> GetTemplates() const override;

	void InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, ArrayView <CString> targets, const WString & dest, bool overwrite) override;

	void RunTask(CString::View command, Array <WString> && args, const Function <bool(const Data::Archive & output)> & done = {});


	void OnReset(Key32 context) override;

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

};

AppImpl::AppImpl()
	: App(K32("ProjectCreator"), 3)
	, m_reflex_path(ReflexCLI::GetReflexPath())
{
}

WString::View AppImpl::GetReflexPath() const
{
	return m_reflex_path;
}

ArrayView <TemplateDefinition> AppImpl::GetTemplates() const
{
	return m_templates;
}

void AppImpl::OnReset(Key32 context)
{
	RunTask("templates", {}, [this](const Data::Archive & output)
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

void AppImpl::InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, ArrayView <CString> targets, const WString & dest, bool overwrite)
{
	auto folder_name = ReflexCLI::GetProjectFolderName(tmpl, inputs);

	if (!folder_name)
	{
		ReflexProjectCreator::output.Error("product undefined");
		return;
	}

	auto path = Join(dest, folder_name);

	Array <WString> args;

	args.Push(L"--template");
	args.Push(File::SplitFilename(File::RemoveTrailingStroke(tmpl.folder)).b);

	for (auto & [key, value] : inputs)
	{
		args.Push(Join(L"--", ToWString(key)));
		args.Push(ToWString(value));
	}

	args.Push(L"--generate");
	args.Push(ToWString(Merge(targets, ',')));
	args.Push(L"--output");
	args.Push(path);

	if (overwrite)
	{
		args.Push(L"--overwrite");
		args.Push(L"true");
	}

	RunTask("create", std::move(args), [path](const Data::Archive & output)
	{
		System::Open(path);

		return true;
	});
}

void AppImpl::RunTask(CString::View command, Array <WString> && args, const Function <bool(const Data::Archive & output)> & done)
{
	auto temp_file = File::AcquireTempFile(ToWString(command));

	Key32 id = command;

	auto & job = m_jobs.Acquire(id);

	args.Push(L"--command");
	args.Push(ToWString(command));
	args.Push(L"--detail");
	args.Push(L"true");

	job.task = System::Process::Create(ReflexCLI::GetReflexExecutablePath(m_reflex_path), args, { .std_out = temp_file.b.Adr(), .allow_window = false });

	temp_file.b.Clear();	//System::Process now owns the temp file handle

	REFLEX_ASSERT(IsValid(job.task));

	job.clock = Async::CreatePeriodicClock(0.25f, [this, id, done, filename = temp_file.a, &job]()
	{
		if (job.task->Completed())
		{
			job.task.Clear();	//discard process, releases output handle, file gets closed

			bool update = done(File::Open(filename));

			[[maybe_unused]] bool ok = System::Delete(filename);

			REFLEX_ASSERT(ok);

			m_jobs.Unset(id);

			if (update) Notify(false);
		}
	});

	Notify(false);
}

void AppImpl::OnRestore(Data::Archive::View & stream, Key32 context)
{
	Data::Deserialize(stream, m_templates);

	OnReset(context);	//check for changes
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

void ReflexCLI::TokenDefinition::Serialize(Data::Archive & stream) const
{
	Data::Serialize(stream, id, token, name);
}

void ReflexCLI::TokenDefinition::Deserialize(Data::Archive::View & stream)
{
	Data::Deserialize(stream, id, token, name);
}

void ReflexCLI::TemplateDefinition::Serialize(Data::Archive & stream) const
{
	Data::SerializeUTF8(stream, folder);
	Data::Serialize(stream, name, description_utf8, platforms, paths, strings);
}

void ReflexCLI::TemplateDefinition::Deserialize(Data::Archive::View & stream)
{
	Data::DeserializeUTF8(stream, folder);
	Data::Deserialize(stream, name, description_utf8, platforms, paths, strings);
}
