#include "[require].h"
#include "file_handle.h"




//
//component

REFLEX_BEGIN_INTERNAL(Reflex::System::Win)

struct Process : public Reflex::System::Process
{
	Process(const WString & filename, const ArrayView <WString> & arguments, const Options & options);

	~Process();

	bool Status() const override;

	bool Completed() const override;

	void Wait() override;

	void Terminate() override;


	static void Backslash(WString & out, WChar ch, UInt & backslash_count);

	static WString QuoteWindowsArg(WString::View arg);

	static WString CombineArgs(const WString & path, const ArrayView <WString> & args)
	{
		WString out = QuoteWindowsArg(path);

		if (args)
		{
			for (auto & i : args)
			{
				out.Push(L' ');

				out.Append(QuoteWindowsArg(i));
			}
		}

		return out;
	}


	bool m_status;

	Reference <System::FileHandle> m_std_out;

	//HANDLE m_readpipe, m_writepipe;

	PROCESS_INFORMATION m_processinfo;


	static constexpr Int32 kPriorities[] = { BELOW_NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS, ABOVE_NORMAL_PRIORITY_CLASS };
};

Process::Process(const WString & path, const ArrayView <WString> & arguments, const Options & options)
{
	Init(m_processinfo);

	auto startupinfo = Init<STARTUPINFOW>();

	startupinfo.cb = sizeof(STARTUPINFOW);

	HANDLE process_std_out = nullptr;
	bool inherit_handles = false;

	if (options.std_out)
	{
		if (auto handle = QueryWriteableFileHandle(*options.std_out))
		{
			if (DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &process_std_out, 0, TRUE, DUPLICATE_SAME_ACCESS))
			{
				startupinfo.dwFlags |= STARTF_USESTDHANDLES;
				startupinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
				startupinfo.hStdOutput = process_std_out;
				startupinfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

				m_std_out = options.std_out;
				inherit_handles = true;
			}
		}
	}

	WString cmdline = CombineArgs(path, arguments);

	m_status = True(CreateProcessW(0, (LPWSTR)cmdline.GetData(), 0, 0, inherit_handles, CREATE_DEFAULT_ERROR_MODE | (options.allow_window ? 0 : CREATE_NO_WINDOW) | kPriorities[options.priority], 0, 0, &startupinfo, &m_processinfo));

	if (process_std_out)
	{
		CloseHandle(process_std_out);
	}
}

Process::~Process()
{
	if (Completed())
	{
		TerminateProcess(m_processinfo.hProcess, 0);
	}
	else
	{
		WaitForSingleObject(m_processinfo.hProcess, INFINITE);
	}

	CloseHandle(m_processinfo.hThread);

	CloseHandle(m_processinfo.hProcess);
}

bool Process::Status() const
{
	return m_status;
}

bool Process::Completed() const
{
	DWORD code;

	if (GetExitCodeProcess(m_processinfo.hProcess, &code))
	{
		return code != STILL_ACTIVE;
	}

	return false;
}

void Process::Terminate()
{
	TerminateProcess(m_processinfo.hProcess, 0);
}

void Process::Wait()
{
	WaitForSingleObject(m_processinfo.hProcess, INFINITE);
}

REFLEX_NOINLINE void Process::Backslash(WString & out, WChar ch, UInt & backslash_count)
{
	auto ptr = Extend(out, backslash_count + 1).data;

	REFLEX_LOOP(idx, backslash_count)
	{
		*ptr++ = L'\\';
	}

	backslash_count = 0;

	*ptr++ = ch;
}

WString Process::QuoteWindowsArg(WString::View arg)
{
	if (arg)
	{
		if (arg.GetFirst() == L'"' && arg.GetLast() == L'"') return arg;

		for (auto ch : arg)
		{
			switch (ch)
			{
			case L' ':
			case L'\t':
			case L'"':
				goto Quote;
			}
		}

		return arg;

		REFLEX_MARKER(Quote);

		WString out = L"\"";
		UInt backslash_count = 0;

		for (auto ch : arg)
		{
			if (ch == L'\\')
			{
				++backslash_count;
			}
			else if (ch == L'"')
			{
				// for (UInt i = 0; i < backslash_count * 2 + 1; ++i)
				// {
					// out.Push(L'\\');
				// }

				// backslash_count = 0;

				// out.Push(L'"');

				backslash_count = backslash_count * 2 + 1;

				Backslash(out, L'"', backslash_count);
			}
			else
			{
				// for (UInt i = 0; i < backslash_count; ++i)
				// {
					// out.Push(L'\\');
				// }

				// backslash_count = 0;

				// out.Push(ch);

				Backslash(out, ch, backslash_count);
			}
		}

		// for (UInt i = 0; i < backslash_count * 2; ++i)
		// {
			// out.Push(L'\\');
		// }

		// out.Push(L'"');

		backslash_count *= 2;

		Backslash(out, L'"', backslash_count);

		return out;
	}
	else
	{
		return L"\"\"";
	}
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::System::Process> Reflex::System::Process::Create(const WString & path, const ArrayView <WString> & arguments, const Options & options)
{
	return REFLEX_CREATE(Win::Process, path, arguments, options);
}
