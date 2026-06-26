#include "[require].h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex)

constexpr CString::View kLogTypes[] = { " ", " WARN ", " ERROR " };

constexpr UInt8 kDefaultOutputFlags = kOutputQueue | kOutputConsole;

struct DebugGlobals : public StateMt
{
	struct OutputImpl : public Output
	{
		using Output::m_output_mask;
		using Output::m_output;
	};


	DebugGlobals();

	~DebugGlobals();


	template <bool QUEUE, bool CONSOLE, bool FILE> static void OutputFn(const Output & self, LogType status, const CString::View & stringref);

	REFLEX_TBINDER_3P(OutputFn);


	static void BindOutput();


	Reference <System::FileHandle> m_file;

	static UInt8 st_output_flags;
};

struct NullProfiler : public Output::Profiler
{
	using Output::Profiler::Profiler;
}

g_null_profiler;

UInt8 DebugGlobals::st_output_flags = kDefaultOutputFlags;

Reflex::Detail::Module g_debug_module( "Reflex.debug", System::module );

Reflex::Detail::Module::Member <DebugGlobals> g_debug_globals(g_debug_module);

DebugGlobals::DebugGlobals()
{
	st_output_flags = kDefaultOutputFlags;

	BindOutput();
}

DebugGlobals::~DebugGlobals()
{
	m_file.Clear();

	st_output_flags = 0;

	BindOutput();
}

void DebugGlobals::BindOutput()
{
	for (auto & i : Output::range)
	{
		auto & impl = Cast<OutputImpl>(i);

		impl.m_output = OutputFnBinder::Bind(st_output_flags & impl.m_output_mask);
	}
}

UInt8 g_eol = 10;

template <bool QUEUE, bool CONSOLE, bool FILE> void DebugGlobals::OutputFn(const Output & self, LogType status, const CString::View & value)
{
	if constexpr (QUEUE | CONSOLE | FILE)
	{
		CString::View ns = self.name;

		if constexpr (QUEUE)
		{
			Output::queue.Push({ self.id, status, value });
		}

		if constexpr (CONSOLE)
		{
			auto buffer = Detail::DebugJoin({}, '[', ns, ']', kLogTypes[status], value);

			if constexpr (REFLEX_DEBUG)
			{
				System::DebugLog(false, buffer.data);
			}
			else
			{
				System::Log(buffer.data);
			}
		}

		if constexpr (FILE)
		{
			REFLEX_ASSERT(System::module.IsInitalised());

			auto & file = *g_debug_globals->m_file;

			file.Write(ns.data, ns.size);

			file.Write(kSpace.data, 1);

			file.Write(value.data, value.size);

			file.Write(&g_eol, 1);
		}
	}
}

REFLEX_END_INTERNAL

const Reflex::Detail::Module & Reflex::debug_module = g_debug_module;

Reflex::Output::Profiler & Reflex::Output::Profiler::null = Reflex::g_null_profiler;

Reflex::Output::Queue Reflex::Output::queue;

const Reflex::StateMt & Reflex::Output::state = *Reflex::g_debug_globals;

const Reflex::CString::View Reflex::Detail::kFalseTrue[] = { "false", "true" };

decltype (&Reflex::System::GetElapsedTime) Reflex::Detail::GetElapsedTime = &Reflex::System::GetElapsedTime;

void Reflex::Output::SetOutputFile(TRef <System::FileHandle> logfile)
{
	REFLEX_ASSERT(System::module.IsInitalised());

	if (logfile)
	{
		DebugGlobals::st_output_flags |= kOutputFile;
	}
	else
	{
		DebugGlobals::st_output_flags &= ~kOutputFile;
	}

	g_debug_globals->m_file = logfile;

	DebugGlobals::BindOutput();

	Detail::GetElapsedTime = &Reflex::System::GetElapsedTime;
}

void Reflex::Output::Disable()
{
	REFLEX_ASSERT(System::module.IsInitalised());
	
	DebugGlobals::st_output_flags = REFLEX_DEBUG ? kOutputConsole : 0;

	DebugGlobals::BindOutput();

	Detail::GetElapsedTime = [](){ return 0.0; };
}

Reflex::Output::Output(const char * name, UInt8 mask)
	: name(name)
	, id(Output::name)
	, m_output_mask(mask)
	, m_output(&DebugGlobals::OutputFn<true,true,false>)
{
}

void Reflex::Output::SetFlags(UInt8 output_flags)
{
	m_output_mask = output_flags;

	m_output = DebugGlobals::OutputFnBinder::Bind(DebugGlobals::st_output_flags & m_output_mask);
}
