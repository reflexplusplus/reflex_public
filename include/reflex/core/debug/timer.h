#pragma once

#include "profiler.h"




//
//Secondary API

namespace Reflex
{

	class ScopeTimer;

	class ScopeProfiler;

}




//
//ScopeTimer

class Reflex::ScopeTimer
{
public:

	REFLEX_INLINE ScopeTimer(Output & scope, const CString::View & name)
		: scope(scope)
		, m_name(name)
		, m_time(Detail::GetElapsedTime())
	{
	}

	REFLEX_INLINE ~ScopeTimer()
	{
		scope.Log(m_name.Extract(), kSpace, ToCString(GetElapsedTime() * 1000.0, 3));
	}

	REFLEX_INLINE Float GetElapsedTime() const { return Float(Detail::GetElapsedTime() - m_time); }



private:

	Output & scope;

	Output::Buffer m_name;

	Float64 m_time;

};




//
//ScopeProfiler

class Reflex::ScopeProfiler
{
public:

	REFLEX_INLINE ScopeProfiler(Output::Profiler & profiler)
		: profiler(profiler)
		, m_time(Detail::GetElapsedTime())
	{
	}

	REFLEX_INLINE ~ScopeProfiler()
	{
		profiler.Write(GetElapsedTime());
	}

	REFLEX_INLINE Float GetElapsedTime() const { return Float(Detail::GetElapsedTime() - m_time); }



private:

	Output::Profiler & profiler;

	Float64 m_time;

};
