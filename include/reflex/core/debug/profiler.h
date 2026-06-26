#pragma once

#include "output.h"




//
//Secondary API

class Reflex::Output::Profiler :
	public Object,
	public StateMt
{
public:

	static Profiler & null;

	
	
	//lifetime

	Profiler(Output & scope, const CString::View & name, UInt precision = 2);

	~Profiler();



	//info

	void SetName(const CString::View & name) { m_name = name; }

	CString::View GetName() const { return m_name.Extract(); }

	UInt GetPrecision() const { return m_precision; }



	//access

	void Reset();

	void Write(Float64 value);

	void SetStatus(LogType status);


	UInt GetCount() const { return m_count; }

	Float64 GetAverage() const { return m_sum / Clip<UInt>(m_count, 1, 128); }

	Float64 GetMin() const { return m_min; }

	Float64 GetMax() const { return m_max; }

	LogType GetStatus() const { return m_status; }



	//special

	void Detach();	//remove from IDE view



protected:

	Profiler();



private:

	friend Output;

	Item m_item;

	AtomicUInt32 * m_plist_state;

	Buffer m_name;

	Float64 m_values[128];

	Float64 m_min, m_max, m_sum;

	LogType m_status;

	UInt32 m_count;

	UInt32 m_precision;

};
