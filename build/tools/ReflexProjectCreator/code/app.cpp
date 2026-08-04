#include "app.h"
#include "../../ReflexCLI/code/tasks.h"




namespace ReflexProjectCreator { namespace {	//begin internal namespace

struct AppImpl : public App
{
	AppImpl();

	WString::View GetReflexPath() const override;

	ArrayView <TemplateDefinition> GetTemplates() const override;

	ArrayView <Pair<CString,bool>> GetTargets() const override;

	void InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, ArrayView <CString> targets, const WString & dest, bool overwrite) override;

	void RunTask(CString::View cmd, Array <WString> && args, const Function <bool(const Data::Archive & output)> & done = {});


	void OnReset(Key32 context) override;

	void OnRestore(Data::Archive::View & stream, Key32 context) override;

	void OnStore(Data::Archive & stream) const override;


	WString m_reflex_path;

	Array <TemplateDefinition> m_templates;
	
	Array <Pair<CString,bool>> m_targets;


	struct Job
	{ 
		Reference <Object> clock;
		Reference <System::Process> task;
	};

	Map <Key32,Job> m_jobs;

};

AppImpl::AppImpl()
	: App(MakeKey32("ProjectCreator"), 2)
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

ArrayView <Pair<CString,bool>> AppImpl::GetTargets() const
{
	return m_targets;
}

void AppImpl::OnReset(Key32 context)
{
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

	RunTask("list-targets", {}, [this](const Data::Archive & output)
	{
		Array <Pair<CString,bool>> targets;

		auto itr = ToView(output);
	
		CString line;

		while (Data::ReadLine(itr, line))
		{
			auto & target = targets.Push({ Trim<char>(line), false });

			switch (MakeKey32(target.a))
			{
			case MakeKey32("android_studio"):
				target.b = true;
				break;

			case MakeKey32("visual_studio"):
				target.b = System::kPlatform == System::kPlatformWindows;
				break;

			case MakeKey32("xcode"):
				target.b = System::kPlatform == System::kPlatformMacOS;
				break;
			}
		}

		return SetFiltered(m_targets, targets);
	});
}

void AppImpl::InstantiateTemplate(const TemplateDefinition & tmpl, ArrayView <Pair<CString>> inputs, ArrayView <CString> targets, const WString & dest, bool overwrite)
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
	args.Push(ToWString(Merge(targets, ',')));
	args.Push(L"--output");
	args.Push(dest);

	if (overwrite)
	{
		args.Push(L"--overwrite");
		args.Push(L"true");
	}

	RunTask("create", std::move(args), [dest](const Data::Archive & output)
	{
		constexpr CString::View kProjectCreatedAt = "project created at ";

		WString created_path = dest;
		
		auto itr = ToView(output);
		
		CString line;

		while (Data::ReadLine(itr, line))
		{
			auto trimmed = Trim<char>(line);

			if (Left<true>(trimmed, kProjectCreatedAt.size) == kProjectCreatedAt)
			{
				created_path = ToWString(Mid<true>(trimmed, kProjectCreatedAt.size));
			}
			else if (trimmed)
			{
				ReflexProjectCreator::output.Error(trimmed);
			}
		}

		System::Open(created_path);

		return true;
	});
}

void AppImpl::RunTask(CString::View cmd, Array <WString> && args, const Function <bool(const Data::Archive & output)> & done)
{
	auto temp_file = File::AcquireTempFile(ToWString(cmd));

	Key32 id = cmd;

	auto & job = m_jobs.Acquire(id);

	args.Push(L"--cmd");
	args.Push(ToWString(cmd));
	args.Push(L"--detail");
	args.Push(L"true");

	job.task = System::Process::Create(ReflexCLI::GetReflexExecutablePath(m_reflex_path), args, { .std_out = temp_file.b.Adr(), .allow_window = false });

	temp_file.b.Clear();	//System::Process now owns the temp file handle

	REFLEX_ASSERT(job.task->Status());

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
	Data::Deserialize(stream, m_templates, m_targets);

	OnReset(context);	//check for changes
}

void AppImpl::OnStore(Data::Archive & stream) const
{
	Data::Serialize(stream, m_templates, m_targets);
}

} } //end internal namespace

Reflex::Output ReflexProjectCreator::output("ProjectCreator");

Reflex::TRef <ReflexProjectCreator::App> ReflexProjectCreator::App::Create()
{
	return New<ReflexProjectCreator::AppImpl>();
}
