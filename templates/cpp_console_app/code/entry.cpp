#include "session.h"
#include "reflex_ext/bootstrap/common/detail.h"

#if REFLEX_DEBUG
#include "reflex_ext/../../src/reflex_ext/bootstrap/common/bootstrap_functions.cpp"
#endif




//
//entry (minimal bootstrap, filesystem with locator for current directory)

using namespace Reflex;

Output _PRODUCT-NAME-SYMBOL_::output("_PRODUCT-NAME_");

_PRODUCT-NAME-SYMBOL_::ConsoleSession::ConsoleSession(ArrayView <CString::View> cmdline, CString::View vendor, CString::View product)
	: m_filesystem(File::VirtualFileSystem::Create(File::kdisk)),
	m_standard_in(System::FileHandle::Create(System::FileHandle::kStandardStreamIn)),
	m_standard_out(System::FileHandle::Create(System::FileHandle::kStandardStreamOut)),
	m_lock(m_filesystem)
{
	if constexpr (REFLEX_DEBUG)
	{
		auto path = Join(Bootstrap::Detail::ExtractProjectDir(__FILE__), L"reflex_log.txt");

		auto file = System::FileHandle::Create(path, System::FileHandle::kModeOverwrite);

		Output::SetLogFile(file);
	}
	else
	{
		Output::Disable();
	}

	m_lock.Attach(New<File::FileLocator>());

	m_lock.Attach(File::SearchPath::Create(System::GetCurrentDirectory()));

	::_PRODUCT-NAME-SYMBOL_::Main(*this, m_lock);
}

CString::View _PRODUCT-NAME-SYMBOL_::ConsoleSession::GetInput(CString::View prompt)
{
	Print(prompt);

	auto n = m_standard_in->Read(m_buffer, 128);

	CString::View cbuffer = { m_buffer, n };

	return Data::Detail::ReadLine(cbuffer);
}

UInt8 System::OnStart(const ArrayView <CString::View> & cmdline)
{
#if REFLEX_DEBUG
	auto agent_args = Bootstrap::ParseCmdlineArgs(cmdline, true);

	if (Data::GetBool(agent_args, "terminate-on-assert"))
	{
		System::Detail::DebugBreak = [](const char * msg)
		{
			auto file = Output::GetLogFile();

			file->Flush(true);

			File::WriteLine(file, "*** REFLEX_ASSERT ***");
			File::WriteLine(file, msg);

			System::Detail::EnumerateStackTrace(file.Adr(), [](void * pfile, UInt frame, const void * address, const char * symbol)
			{
				auto file = Cast<System::FileHandle>(pfile);

				auto buffer = Reflex::Detail::DebugJoin(" ", frame, address, symbol);

				File::WriteLine(*file, buffer);
			});

			file->Flush(true);

			System::Detail::Terminate(1);
		};
	}
#endif

	_PRODUCT-NAME-SYMBOL_::ConsoleSession session(cmdline, "_VENDOR-NAME_", "_PRODUCT-NAME_");

	return 0;
}
