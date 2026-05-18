#pragma once

#define REFLEX_INCLUDE_UI false
#include "reflex_ext.h"




//
//ConsoleSession

namespace _PRODUCT-NAME-SYMBOL_
{
	
	class ConsoleSession
	{
	public:

		//lifetime

		ConsoleSession(const Reflex::ArrayView <Reflex::CString::View> & cmdline, const Reflex::CString::View & vendor, const Reflex::CString::View & product);


		//access

		Reflex::ArrayView <Reflex::CString> GetCommandLine() const;

		template <class ... ARGS> void Print(ARGS && ... args);

		template <class ... ARGS> void PrintEx(ARGS && ... args);

		Reflex::CString::View GetInput(const Reflex::CString::View & prompt);



	private:

		Reflex::Reference <Reflex::Object> m_debug_module;

		Reflex::Reference <Reflex::File::VirtualFileSystem> m_filesystem;

		Reflex::Reference <Reflex::System::FileHandle> m_standard_in;

		Reflex::Reference <Reflex::System::FileHandle> m_standard_out;

		Reflex::File::VirtualFileSystem::Lock m_lock;

		Reflex::Array <Reflex::CString> m_cmdline;

		char m_buffer[128];
	};

	extern void Main(ConsoleSession & session, Reflex::File::VirtualFileSystem::Lock & lock);

	extern Reflex::Output output;

}




//
//impl

inline Reflex::ArrayView <Reflex::CString> _PRODUCT-NAME-SYMBOL_::ConsoleSession::GetCommandLine() const
{
	return m_cmdline;
}

template <class ... ARGS> inline void _PRODUCT-NAME-SYMBOL_::ConsoleSession::Print(ARGS && ... args)
{
	auto text = Reflex::Detail::DebugJoin(Reflex::kSpace, std::forward<ARGS>(args)...);

	output.LogEx(Reflex::kLogNormal, text);

	Reflex::File::WriteLine(m_standard_out, text);
}

template <class ... ARGS> inline void _PRODUCT-NAME-SYMBOL_::ConsoleSession::PrintEx(ARGS && ... args)
{
	auto text = Reflex::Detail::DebugJoin({}, std::forward<ARGS>(args)...);

	output.LogEx(Reflex::kLogNormal, text);

	Reflex::File::WriteLine(m_standard_out, text);
}
