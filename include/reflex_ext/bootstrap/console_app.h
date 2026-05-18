#pragma once

#include "common/global.h"




//
//Experimental API

namespace Reflex::Bootstrap::CLI
{
	using TaskFn = FunctionPointer <void(Async::Worker::Context & ctx, const Data::PropertySet & args, System::FileHandle & std_out)>;

	struct TaskDef
	{
		Key32 id;
		bool async = false;
		TaskFn fn;
	};

	enum Flags : UInt8
	{
		kFlagPrintDuration = 1 << 0,
		kFlagForceVerbose = 1 << 1,
	};


	//for main

	UInt8 Dispatch(const ArrayView <CString::View> & cmdline, const ArrayView <TaskDef> & tasks, UInt8 flags = 0);



	//for tasks

	inline WString GetPathArg(const Data::PropertySet & args, Key32 id)
	{
		return File::ResolveRelativePath(File::CorrectStrokes(ToWString(Data::GetCString(args, id))));
	}

	inline WString GetFolderArg(const Data::PropertySet & args, Key32 id)
	{
		return File::CorrectTrailingStroke(GetPathArg(args, id));
	}

	inline bool GetBoolArg(const Data::PropertySet & args, Key32 id)
	{
		return Data::GetCString(args, id) == Reflex::Detail::kFalseTrue[1];
	}

	inline void SetCompleted(Async::Worker::Context & ctx)
	{
		ctx.SetResult(true);
	}

	inline void SetError(Async::Worker::Context & ctx, CString::View error)
	{
		ctx.SetResult(false, New<Data::CStringProperty>(error));
	}

}




//
//impl

REFLEX_NS(Reflex::Bootstrap::CLI::Detail)

inline Data::PropertySet PackArgs(ArrayView <CString::View> cmdline)
{
	Data::PropertySet args;

	UInt count = cmdline.size & ~1u;

	if (count != cmdline.size)
	{
		Data::SetCString(args, "task", cmdline.GetFirst());

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

inline void LoadArgsFile(Data::PropertySet & args)
{
	if (auto args_path = GetPathArg(args, "args"))
	{
		Data::Assimilate(args, Data::DecodePropertySet(Data::kJsonFormat, File::Open(args_path)));
	}
}

inline const TaskDef * SelectTask(const ArrayView <TaskDef> & tasks, const Data::PropertySet & args)
{
	auto task_arg = Data::GetCString(args, "task");

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

inline bool WaitForTask(System::FileHandle & std_out, bool verbose, Async::Task & task)
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

REFLEX_END

inline Reflex::UInt8 Reflex::Bootstrap::CLI::Dispatch(const ArrayView <CString::View> & cmdline, const ArrayView <TaskDef> & tasks, UInt8 flags)
{
	auto time = System::GetElapsedTime();

	auto args = Detail::PackArgs(cmdline);

	Detail::LoadArgsFile(args);

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

	bool result = false;

	if (auto taskdef = Detail::SelectTask(tasks, args))
	{
		CString error_msg;

		if (taskdef->async)
		{
			auto task = Make<Async::Worker>([&args, &std_out, fn = taskdef->fn](Async::Worker::Context & ctx)
			{
				fn(ctx, args, std_out);
			});

			result = Detail::WaitForTask(std_out, verbose, task);

			if (auto perror = DynamicCast<Data::CStringProperty>(task->GetResult()))
			{
				error_msg = perror->value;
			}
		}
		else
		{
			struct TaskContext : Async::Worker::Context
			{
				using Context::m_result_ok;
				using Context::m_result_payload;
			}
			ctx;

			taskdef->fn(ctx, args, std_out);

			result = ctx.m_result_ok;

			if (auto perror = DynamicCast<Data::CStringProperty>(ctx.m_result_payload))
			{
				error_msg = perror->value;
			}
		}

		if (result)
		{
			File::WriteLine(std_out, "ok");
		}
		else
		{
			File::WriteLine(std_out, error_msg);
		}
	}
	else
	{
		File::WriteLine(std_out, "unknown task");
	}

	if (flags & kFlagPrintDuration)
	{
		File::WriteLine(std_out, Join("duration: ", ToCString(SetDelta(time, System::GetElapsedTime()), 2), " sec"));
	}

	return result ? 0 : 1;
}
