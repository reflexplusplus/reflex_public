#include "session.h"




//
//entry (minimal bootstrap, filesystem with locator for current directory)

using namespace Reflex;

Output _PRODUCT-NAME-SYMBOL_::output("_PRODUCT-NAME_");

_PRODUCT-NAME-SYMBOL_::ConsoleSession::ConsoleSession(const Reflex::ArrayView <Reflex::CString::View> & cmdline, const CString::View & vendor, const CString::View & product)
	: m_filesystem(Reflex::File::VirtualFileSystem::Create(Reflex::File::kdisk)),
	m_standard_in(System::FileHandle::Create(System::FileHandle::kStandardStreamIn)),
	m_standard_out(System::FileHandle::Create(System::FileHandle::kStandardStreamOut)),
	m_lock(m_filesystem)
{
	for (auto & i : cmdline) m_cmdline.Push(i);

	if (REFLEX_DEBUG)
	{
		auto path = Join(System::GetPath(System::kPathDesktop), ToWString(vendor), System::kPathDelimiter, ToWString(product));

		File::MakePath(path);

		path.Append(Join(System::kPathDelimiter, ToWString(System::GetTime()), L".txt"));

		auto file = System::FileHandle::Create(path, System::FileHandle::kModeOverwrite);

		Output::SetOutputFile(file);
	}
	else
	{
		Output::Disable();
	}

	m_lock.Attach(New<Reflex::File::FileLocator>());

	m_lock.Attach(Reflex::File::SearchPath::Create(System::GetCurrentDirectory()));

	::_PRODUCT-NAME-SYMBOL_::Main(*this, m_lock);
}

CString::View _PRODUCT-NAME-SYMBOL_::ConsoleSession::GetInput(const CString::View & prompt)
{
	Print(prompt);

	auto n = m_standard_in->Read(m_buffer, 128);

	CString::View cbuffer = { m_buffer, n };

	return Data::Detail::ReadLine(cbuffer);
}

UInt8 System::OnStart(const ArrayView <CString::View> & cmdline)
{
	_PRODUCT-NAME-SYMBOL_::ConsoleSession session(cmdline, "_VENDOR-NAME_", "_PRODUCT-NAME_");

	return 0;
}
