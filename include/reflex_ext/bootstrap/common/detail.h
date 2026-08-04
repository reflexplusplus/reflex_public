#pragma once

//remove some windows macros that mess up reflex

#undef RGB
#undef near
#undef far

#include "[require].h"




//
//Detail

namespace Reflex::Bootstrap::Detail
{

	WString MakeProductPath(System::Path system_path, const CString::View & vendor, const CString::View & name, const WString::View & filename = {});

	constexpr WString::View kPrefsFilename = L"config";


	//entry helpers

	inline WString ExtractProjectDir(const char * filepath)
	{
		#if REFLEX_DEBUG
		//allow reloading of edited script files from local machine in debug mode
		auto store = File::CorrectStrokes(ToWString(ToView(filepath)));
		return File::ResolveRelativePath(Join(File::SplitFilename(store).a, L"..", System::kPathDelimiter));
		#else
		return {};
		#endif
	}


	struct Monitor;		//wrapper for StateMt/State::Monitor

}





//
//Detail::Monitor

struct Reflex::Bootstrap::Detail::Monitor
{
	Monitor()
	{
		Disconnect();
	}

	void Connect(const State & state)
	{
		m_impl.st.Init(state);

		m_poll = [](Impl & impl)
		{
			return impl.st->Poll();
		};
	}

	void Connect(const StateMt & state)
	{
		m_impl.mt.Init(state);

		m_poll = [](Impl & impl) { return impl.mt->Poll(); };
	}

	void Disconnect() { m_poll = [](Impl &) { return false; }; }

	bool Poll() { return m_poll(m_impl); }



private:

	union Impl
	{
		Reflex::Detail::Initialiser <State::Monitor> st;
		Reflex::Detail::Initialiser <StateMt::Monitor> mt;
	}
	m_impl;

	FunctionPointer <bool(Impl &)> m_poll;
};