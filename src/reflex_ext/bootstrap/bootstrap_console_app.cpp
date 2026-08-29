#include "reflex_ext/bootstrap/console_app.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap::CLI)

constexpr Key32 kcommand = K32("command");

void LoadArgsFile(Data::PropertySet & args)
{
	if (auto args_path = GetFilename(args, "args", false))
	{
		Data::Assimilate(args, Data::DecodePropertySet(Data::kJsonFormat, File::Open(args_path)));
	}
}

struct NullProgressBarImpl : public ProgressBar
{
	void Render(Float32 progress = 0.0f) override {}
};

void AppendText(ArrayRegion <char> & region, const CString::View & text)
{
	MemCopy(text.data, region.data, text.size);
	region = Nudge(region, text.size);
}

void AppendFilled(ArrayRegion <char> & region, UInt size, char value)
{
	Fill({ region.data, size }, value);
	region = Nudge(region, size);
}

template <bool PROGRESS>
struct ProgressBarImpl : public ProgressBar
{
	static constexpr UInt kBarWidth = 32;
	static constexpr UInt kAnsiColourSize = 5;
	static constexpr UInt kNumProgressColours = 4;

	static constexpr const char kSpinner[] = "|/-\\";

	ProgressBarImpl(System::FileHandle & out, CString::View title)
		: out(out)
		, m_spinner(0)
	{
		UInt total = title.size + 3;

		if constexpr (PROGRESS)
		{
			total += kBarWidth + 3 + (kAnsiColourSize * kNumProgressColours);
		}

		m_buffer.Reserve(total);
		m_buffer.SetSize<kAllocateNone>(total);

		auto region = ToRegion(m_buffer);

		*region.data = '\r';
		region = Nudge(region, 1);

		AppendText(region, title);
		*region.data = ' ';

		m_prefix_size = total - region.size + 1;

		File::WriteBytes(out, Data::Pack(CString::View("\x1b[?25l")));
	}

	~ProgressBarImpl()
	{
		File::WriteBytes(out, Data::Pack(CString::View("\r\x1b[2K\x1b[?25h")));
	}

	void Render(Float32 progress = 0.0f) override
	{
		CString::Region region = { m_buffer.GetData() + m_prefix_size, m_buffer.GetSize() - m_prefix_size };

		if constexpr (PROGRESS)
		{
			progress = Clip(progress, 0.0f, 1.0f);

			UInt filled = Truncate(progress * Float32(kBarWidth));
			UInt remaining = kBarWidth - filled;

			AppendText(region, Detail::kColours[kColourDefault]);
			*region.data = '[';
			region = Nudge(region, 1);
			AppendText(region, Detail::kColours[kColourGreen]);
			AppendFilled(region, filled, '#');
			AppendText(region, Detail::kColours[kColourBrightBlack]);
			AppendFilled(region, remaining, '.');
			AppendText(region, Detail::kColours[kColourDefault]);
			*region.data = ']';
			region = Nudge(region, 1);
			*region.data = ' ';
			region = Nudge(region, 1);
		}

		*region.data = kSpinner[(m_spinner++ & 3)];

		File::WriteBytes(out, Data::Pack(m_buffer));
		
		out->Flush(false);
	}

	const TRef <System::FileHandle> out;

	CString m_buffer;

	UInt m_prefix_size;

	UInt m_spinner;
};

NullProgressBarImpl g_null_progressbar;

REFLEX_END_INTERNAL

Reflex::Bootstrap::CLI::ProgressBar & Reflex::Bootstrap::CLI::ProgressBar::null = Reflex::Bootstrap::CLI::g_null_progressbar;

Reflex::TRef <Reflex::Bootstrap::CLI::ProgressBar> Reflex::Bootstrap::CLI::ProgressBar::Create(System::FileHandle & out, CString::View title, bool show_progress)
{
	if (show_progress)
	{
		return REFLEX_CREATE(ProgressBarImpl<true>, out, title);
	}
	else
	{
		return REFLEX_CREATE(ProgressBarImpl<false>, out, title);
	}
}

Reflex::Array <Reflex::CString::View> Reflex::Bootstrap::CLI::GetStringArray(const Data::PropertySet & args, Key32 id)
{
	auto split = Split(Data::GetCString(args, id), ',');

	Remove(split, "");

	return split;
}

Reflex::WString Reflex::Bootstrap::CLI::Detail::ExpandPath(CString::View id, WString::View input, bool folder, bool check_exists)
{
	constexpr CString::View kHint[] = { "<filename>", "<folder>" };
	constexpr WChar kDoubleStroke[] = { File::kStroke, File::kStroke, 0 };

	auto corrected = File::CorrectStrokes(input);

	if (corrected)
	{
		corrected = Replace(corrected, ToView(kDoubleStroke), ToView(File::kStroke));

		if (corrected.GetFirst() == L'~')
		{
			if (corrected.GetSize() == 1)
			{
				corrected = System::GetPath(System::kPathUserHome);
			}
			else if (corrected.GetSize() > 1 && corrected[1] == File::kStroke)
			{
				corrected = File::ResolveIncludePath(System::GetPath(System::kPathUserHome), Mid(corrected, 2));
			}
			else
			{
				ThrowMissingArg(id, kHint[folder]);
			}
		}
		else
		{
			corrected = File::ResolveIncludePath(System::GetCurrentDirectory(), corrected);
		}
	}

	if (check_exists)
	{
		auto fn = folder ? &System::IsDirectory : &System::Exists;

		if (!fn(corrected)) ThrowMissingArg(id, kHint[folder]);
	}

	return corrected;
}

const Reflex::CString::View Reflex::Bootstrap::CLI::Detail::kColours[kNumColour] =
{
	"\x1b[39m",	//kColourDefault

	"\x1b[30m",	//kColourBlack
	"\x1b[31m",	//kColourRed
	"\x1b[32m",	//kColourGreen
	"\x1b[33m",	//kColourYellow
	"\x1b[34m",	//kColourBlue
	"\x1b[35m",	//kColourMagenta
	"\x1b[36m",	//kColourCyan
	"\x1b[37m",	//kColourWhite

	"\x1b[90m",	//kColourBrightBlack
	"\x1b[91m",	//kColourBrightRed
	"\x1b[92m",	//kColourBrightGreen
	"\x1b[93m",	//kColourBrightYellow
	"\x1b[94m",	//kColourBrightBlue
	"\x1b[95m",	//kColourBrightMagenta
	"\x1b[96m",	//kColourBrightCyan
	"\x1b[97m",	//kColourBrightWhite
};

void Reflex::Bootstrap::CLI::Await(System::FileHandle & out, CString::View title, bool show_progress, const Function <bool()> & should_abort, const Function <void(TaskContext & ctx)> & bg_fn)
{
	struct TaskContextImpl : public TaskContext
	{
		bool IsWriteable() const override { return true; }
		UInt64 GetSize() const override { return archive.GetSize(); }
		void SetPosition(UInt64 position) override {}
		UInt64 GetPosition() const override { return archive.GetSize(); }
		UInt32 Read(void * ptr, UInt32 max_size) override { return 0; }
		UInt32 Write(const void * ptr, UInt size) override { archive.Append({ Cast<UInt8>(ptr), size }); return size; }
		bool Truncate() override { return true; }
		bool Flush(bool commit) override { return true; }

		bool Cancelled() const override
		{
			return ctx->Cancelled();
		}

		void SetProgress(Float progress) override
		{
			ctx->SetProgress(progress);
		}

		Async::Worker::Context * ctx;
		Data::Archive archive;
	} 
	task_context;

	auto display = Make<ProgressBar>(out, title, show_progress);

	display->Render();

	auto task = Make<Async::Worker>([bg_fn, &task_context](Async::Worker::Context & ctx) -> Async::Worker::Result
	{
		try
		{
			task_context.ctx = &ctx;

			bg_fn(task_context);

			return { true };
		}
		catch (const CString & error)
		{
			return { false, New<Data::CStringProperty>(error) };
		}
	});

	bool abort = false;

	while (!task->Completed())
	{
		display->Render(task->GetProgress());

		System::SuspendThread(100);

		if (SetFiltered(abort, should_abort()))
		{
			task->Cancel();
		}
	}

	task->Wait();

	display.Clear();

	File::WriteBytes(out, task_context.archive);

	if (task->GetStatus() == Async::Worker::kStatusFailed)
	{
		if (auto error  = DynamicCast<Data::CStringProperty>(task->GetResult()))
		{
			ThrowError(error->value);
		}
		else
		{
			ThrowError(title);
		}
	}
}

Reflex::UInt8 Reflex::Bootstrap::CLI::Detail::Dispatch(ArrayView <CString::View> cmdline, ArrayView <TaskDef> tasks, UInt8 flags, System::FileHandle & out, void * client, FunctionPointer <bool(void * client, ArrayView <CString::View> cmdline, Key32 task, const Data::PropertySet & args, System::FileHandle & out)> fallback)
{
	auto time = System::GetElapsedTime();

	bool ok = true;

	try
	{
		auto args = ParseCmdlineArgs(cmdline, false, 0);

		LoadArgsFile(args);

		if ((flags & kFlagForceVerbose) || GetBool(args, "verbose"))
		{
			Output::SetLogFile(out);
		}
		else
		{
			Output::Disable();
		}

		auto task_arg = Data::GetCString(args, kcommand);

		if (auto task = SearchValue<FieldCompare<&TaskDef::id>>(tasks, task_arg ? Key32(task_arg) : tasks[0].id))
		{
			task->fn(args, out);

			if (flags & kFlagPrintDuration)
			{
				File::WriteLine(out, Join("duration: ", ToCString(SetDelta(time, System::GetElapsedTime()), 2), " sec"));
			}
		}
		else
		{
			ok = fallback(client, cmdline, task_arg, args, out);
		}
	}
	catch (const CString & error)
	{
		ok = false;

		if (flags & kFlagPrintError)
		{
			Print(out, kColourBrightRed, error);
		}
	}

	return ok ? 0 : 1;
}
