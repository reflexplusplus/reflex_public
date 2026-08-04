#include "reflex_ext/bootstrap/console_app.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap::CLI)

void LoadArgsFile(Data::PropertySet & args)
{
	if (auto args_path = GetFilenameArg(args, "args", false))
	{
		Data::Assimilate(args, Data::DecodePropertySet(Data::kJsonFormat, File::Open(args_path)));
	}
}

const TaskDef * SelectTask(const ArrayView <TaskDef> & tasks, const Data::PropertySet & args)
{
	auto task_arg = Data::GetCString(args, "cmd");

	Key32 id = task_arg ? MakeKey32(task_arg) : tasks[0].id;

	for (auto & task : tasks)
	{
		if (task.id == id)
		{
			return &task;
		}
	}

	return nullptr;
}

bool WaitForTask(System::FileHandle & std_out, bool verbose, Async::Task & task)
{
	constexpr const char kSpinner[] = "|/-\\";

	UInt idx = 0;

	if (verbose)
	{
		task.Wait();
	}
	else
	{
		while (!task.Completed())
		{
			auto line = Join("\rWaiting for task... ", kSpinner[idx++ % 4]);

			File::WriteBytes(std_out, Data::Pack(line));

			System::SuspendThread(100);
		}

		File::WriteLine(std_out);
	}

	return task.GetStatus() == Async::Task::kStatusCompleted;
}

REFLEX_END_INTERNAL

Reflex::Data::PropertySet Reflex::Bootstrap::CLI::Detail::PackArgs(ArrayView <CString::View> cmdline)
{
	Data::PropertySet args;

	UInt count = cmdline.size & ~1u;

	if (count != cmdline.size)
	{
		Data::SetCString(args, "cmd", cmdline.GetFirst());

		cmdline = Nudge(cmdline, 1);
	}

	UInt idx = 0;

	while (idx < count)
	{
		auto key = Mid<true>(cmdline[idx++], 2);

		auto value = cmdline[idx++];

		Data::SetCString(args, key, value);
	}

	return args;
}

Reflex::WString Reflex::Bootstrap::CLI::GetFilenameArg(const Data::PropertySet & args, CString::View id, bool check_exists)
{
	auto corrected = File::CorrectStrokes(ToWString(Data::GetCString(args, id)));

	const WChar double_stroke[] = { File::kStroke, File::kStroke, 0 };

	corrected = Replace(corrected, ToView(double_stroke), ToView(File::kStroke));

	auto filename = File::ResolveRelativePath(corrected);

	if (check_exists && !System::Exists(filename))
	{
		ThrowMissingArg(id, "<filename>");
	}

	return filename;
}

Reflex::WString Reflex::Bootstrap::CLI::GetFolderArg(const Data::PropertySet & args, CString::View id, bool check_exists)
{
	auto folder = File::CorrectTrailingStroke(GetFilenameArg(args, id, false));

	if (check_exists && !System::IsDirectory(folder))
	{
		ThrowMissingArg(id, "<folder>");
	}

	return folder;
}

Reflex::UInt8 Reflex::Bootstrap::CLI::Dispatch(const ArrayView <CString::View> & cmdline, const ArrayView <TaskDef> & tasks, UInt8 flags)
{
	static constexpr auto RunCatchingError = [](TaskFn fn, const Data::PropertySet & args, System::FileHandle & std_out, Async::Worker::Context & ctx)
	{
		try
		{
			fn(ctx, args, std_out);

			ctx.SetResult(true);
		}
		catch (const CString & error)
		{
			ctx.SetResult(false, New<Data::CStringProperty>(error));
		}
	};

	static constexpr auto DisplayError = [](UInt8 flags, System::FileHandle & std_out, Object & result)
	{
		if (flags & kFlagPrintError)
		{
			if (auto perror = DynamicCast<Data::CStringProperty>(result))
			{
				File::WriteLine(std_out, perror->value);
			}
		}
	};

	auto time = System::GetElapsedTime();

	auto args = Detail::PackArgs(cmdline);

	LoadArgsFile(args);

	auto std_out = Make<System::FileHandle>(System::FileHandle::kStandardStreamOut);

	bool verbose = (flags & kFlagForceVerbose) || GetBoolArg(args, "verbose");
	
	if (verbose)
	{
		Output::SetOutputFile(std_out);
	}
	else
	{
		Output::Disable();
	}

	if (auto taskdef = SelectTask(tasks, args))
	{
		bool ok = false;

		if (taskdef->async)
		{
			auto task = Make<Async::Worker>([&args, &std_out, fn = taskdef->fn](Async::Worker::Context & ctx)
			{
				RunCatchingError(fn, args, std_out, ctx);
			});

			ok = WaitForTask(std_out, verbose, task);

			DisplayError(flags, std_out, task->GetResult());
		}
		else
		{
			struct TaskContext : Async::Worker::Context
			{
				using Context::m_result_ok;
				using Context::m_result_payload;
			}
			ctx;

			RunCatchingError(taskdef->fn, args, std_out, ctx);

			DisplayError(flags, std_out, ctx.m_result_payload);

			ok = ctx.m_result_ok;
		}

		if (flags & kFlagPrintDuration)
		{
			File::WriteLine(std_out, Join("duration: ", ToCString(SetDelta(time, System::GetElapsedTime()), 2), " sec"));
		}

		return ok ? 0 : 1;
	}
	else
	{
		File::WriteLine(std_out, "unknown task");

		return 1;
	}
}
