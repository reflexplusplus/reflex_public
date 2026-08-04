#include "[require].h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex)

AtomicUInt32 g_null_count;

REFLEX_END_INTERNAL

void Reflex::Output::EnumerateProfilers(const Function <void(Profiler&)> & callback)
{
	constexpr UIntNative kOffset = REFLEX_OFFSETOF(Profiler, m_item);

	for (auto & i : m_profilers)
	{
		auto profiler = ToPointer<Profiler>(ToUIntNative(&i) - kOffset);

		callback(*profiler);
	}
}

Reflex::Output::Profiler::Profiler()
	: m_plist_state(&g_null_count)
	, m_precision(2)
{
	Reset();
}

Reflex::Output::Profiler::Profiler(Output & output, const CString::View & name, UInt precision)
	: m_plist_state(RemoveConst(output.GetAdr()))
	, m_name(name)
	, m_precision(precision)
{
	REFLEX_ASSERT_MAINTHREAD("Output::Profiler::Profiler");

	m_item.Attach(Reinterpret<Item::List>(output.m_profilers));

	Reset();
}

Reflex::Output::Profiler::~Profiler()
{
	REFLEX_ASSERT_MAINTHREAD("Output::Profiler::~Profiler");

	m_plist_state->fetch_add(1, std::memory_order_release);
}

void Reflex::Output::Profiler::Detach()
{
	m_item.Detach();
}

void Reflex::Output::Item::OnDetach(List & list)
{
	constexpr UIntNative kOffset = REFLEX_OFFSETOF(Profiler, m_item);

	auto self = ToPointer<Profiler>(ToUIntNative(this) - kOffset);

	self->m_plist_state->fetch_add(1, std::memory_order_release);

	self->m_plist_state = &g_null_count;
}

void Reflex::Output::Profiler::Reset()
{
	m_count = 0;

	m_min = Float64(kMaxUInt32);

	m_max = -m_min;

	m_status = kLogNormal;

	Wipe(ToRegion(m_values));
	
	m_sum = 0.0;

	m_plist_state->fetch_add(1, std::memory_order_release);

	Notify();
}

void Reflex::Output::Profiler::Write(Float64 value)
{
	m_min = Min(m_min, value);

	m_max = Max(m_max, value);

	//m_status = Max(m_status, status);

	auto & item = m_values[m_count++ & 127];
	
	m_sum -= item;

	item = value;

	m_sum += value;

	m_plist_state->fetch_add(1, std::memory_order_release);

	Notify();
}

void Reflex::Output::Profiler::SetStatus(LogType status)
{
	m_status = Max(m_status, status);

	m_plist_state->fetch_add(1, std::memory_order_release);

	Notify();
}
