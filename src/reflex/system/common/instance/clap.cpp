#include "plugin.h"
#include "instance.cpp"
#include "plugin.cpp"

#include "reflex/system/ext/clap/include/clap/entry.h"
#include "reflex/system/ext/clap/include/clap/factory/plugin-factory.h"
#include "reflex/system/ext/clap/include/clap/ext/audio-ports.h"
#include "reflex/system/ext/clap/include/clap/ext/note-ports.h"
#include "reflex/system/ext/clap/include/clap/ext/params.h"
#include "reflex/system/ext/clap/include/clap/ext/state.h"
#include "reflex/system/ext/clap/include/clap/ext/gui.h"
#include "reflex/system/ext/clap/include/clap/ext/latency.h"

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

struct ClapSession : public PluginSession
{
	using Class = AudioPlugin::Configuration::Class;

	ClapSession(void * hinstance);

	struct ClassRecord
	{
		const AudioPlugin::Configuration::Class * reflex_class;
		clap_plugin_descriptor descriptor = {};
		Array <const char*> features;	// null-terminated feature list, owned here
	};

	Array <ClassRecord> classes;
};

typedef Reflex::The <ClapSession> TheClapPluginSession;

struct ClapInstance : 
	public PluginInstance,
	public clap_plugin
{
	REFLEX_OBJECT(ClapInstance, PluginInstance);

	struct UiAutomationEvent
	{
		enum Type : UInt8 { kBeginAutomation, kEndAutomation, kAutomate };

		Type type;
		UInt16 idx;
		Float32 value;
	};

	using ProcessClapRt = FunctionPointer <clap_process_status(Object&, const clap_process * clap_process_data)>;


	ClapInstance(ClapSession & pluginsession, const Configuration::Class & cls, const clap_host * host);

	~ClapInstance();


	void OnSetViewSize(const iSize & size) override;

	void ReportProcessingDelay(UInt32 delay) override;

	void BeginAutomation(UInt32 idx) override;

	void Automate(UInt32 idx, Float32 value) override;

	void EndAutomation(UInt32 idx) override;

	void ReportStateChange(UInt8 flags) override;


	bool StoreState(const clap_ostream * stream);

	bool RestoreState(const clap_istream * stream);


	static ClapInstance * GetInstance(const clap_plugin * plugin) { return Cast<ClapInstance>(plugin->plugin_data); }

	void QueueUiAutomationEvent(UiAutomationEvent::Type type, UInt16 idx, Float32 value);


	clap_process_status ProcessRt(const clap_process * process);

	void ReceiveMidiAndAutomationEventsRt(const clap_input_events & events);

	void SendAutomationEventsRt(const clap_output_events & out);

	void SendMidiAndAutomationEventsRt(ArrayView <Event> events, const clap_output_events & out);


	const clap_host * m_clap_host;

	decltype(clap_host_params::rescan) m_clap_host_params_rescan;
	decltype(clap_host_params::request_flush) m_clap_host_params_request_flush;
	decltype(clap_host_latency::changed) m_clap_host_latency_changed;
	decltype(clap_host_gui::request_resize) m_clap_host_gui_request_resize;
	decltype(clap_host_gui::resize_hints_changed) m_clap_host_gui_resize_hints_changed;
	decltype(clap_host_state::mark_dirty) m_clap_host_state_mark_dirty;


	ProcessClapRt m_process_clap_rt;

	
	Array <Quad<UInt8>> m_channel_maps[2];

	void * m_host_window;
	
	UInt32 m_reported_latency;

	Queue <UiAutomationEvent,64> m_param_events;


	static const clap_plugin_factory st_clap_factory;
	static const clap_plugin_audio_ports st_clap_audio_ports;
	static const clap_plugin_note_ports st_clap_note_ports;
	static const clap_plugin_params st_clap_params;
	static const clap_plugin_latency st_clap_latency;
	static const clap_plugin_state st_clap_state;
	static const clap_plugin_gui st_clap_gui;

	inline static decltype(&GetMaxPixelDensity) st_get_platform_pixel_factor = []() { return 1; };

	static void NullHostParamsRescan(const clap_host * host, clap_param_rescan_flags flags) {}
	static void NullHostParamsRequestFlush(const clap_host * host) {}
	static void NullHostLatencyChanged(const clap_host * host) {}
	static bool NullHostGuiRequestResize(const clap_host * host, uint32_t width, uint32_t height) { return false; }
	static void NullHostGuiResizeHintsChanged(const clap_host * host) {}
	static void NullHostStateMarkDirty(const clap_host * host) {}
};

ClapSession::ClapSession(void * hinstance)
	: PluginSession(hinstance, kPluginFormatCLAP)
{
	//build type & category map

	const char * types[4];
	types[Class::kTypeAudioProcessor] = CLAP_PLUGIN_FEATURE_AUDIO_EFFECT;
	types[Class::kTypeAudioGenerator] = CLAP_PLUGIN_FEATURE_INSTRUMENT;
	types[Class::kTypeEventProcessor] = CLAP_PLUGIN_FEATURE_NOTE_EFFECT;
	types[Class::kTypeEventGenerator] = CLAP_PLUGIN_FEATURE_NOTE_EFFECT;
	//!note CLAP_PLUGIN_FEATURE_NOTE_DETECTOR not supported by our scheme

	const char * categories[15];
	categories[GetFirstBit(Class::kEQ).value] = CLAP_PLUGIN_FEATURE_EQUALIZER;
	categories[GetFirstBit(Class::kDynamics).value] = CLAP_PLUGIN_FEATURE_COMPRESSOR;     // could also be limiter/gate/etc
	categories[GetFirstBit(Class::kReverb).value] = CLAP_PLUGIN_FEATURE_REVERB;
	categories[GetFirstBit(Class::kDelay).value] = CLAP_PLUGIN_FEATURE_DELAY;
	categories[GetFirstBit(Class::kModulation).value] = CLAP_PLUGIN_FEATURE_CHORUS;         // closest generic modulation
	categories[GetFirstBit(Class::kPitchShift).value] = CLAP_PLUGIN_FEATURE_PITCH_SHIFTER;
	categories[GetFirstBit(Class::kDistortion).value] = CLAP_PLUGIN_FEATURE_DISTORTION;
	categories[GetFirstBit(Class::kFilter).value] = CLAP_PLUGIN_FEATURE_FILTER;
	categories[GetFirstBit(Class::kRestoration).value] = CLAP_PLUGIN_FEATURE_RESTORATION;
	categories[GetFirstBit(Class::kMastering).value] = CLAP_PLUGIN_FEATURE_MASTERING;
	categories[GetFirstBit(Class::kSurround).value] = CLAP_PLUGIN_FEATURE_MIXING;         // no direct surround tag in list
	categories[GetFirstBit(Class::kAnalyzer).value] = CLAP_PLUGIN_FEATURE_ANALYZER;
	categories[GetFirstBit(Class::kSynth).value] = CLAP_PLUGIN_FEATURE_SYNTHESIZER;
	categories[GetFirstBit(Class::kSampler).value] = CLAP_PLUGIN_FEATURE_SAMPLER;
	categories[GetFirstBit(Class::kDrum).value] = CLAP_PLUGIN_FEATURE_DRUM_MACHINE;  // or CLAP_PLUGIN_FEATURE_DRUM

	REFLEX_ASSERT(GetFirstBit(Class::kDrum).value < GetArraySize(categories));

	auto n = config.classes.GetSize();

	classes.Allocate(n);

	REFLEX_LOOP(idx, n)
	{
		auto & [cls, descriptor, features] = classes.Push<kAllocateNone>({ .reflex_class = &config.classes[idx], .descriptor = {} });

		descriptor.clap_version = CLAP_VERSION;
		descriptor.id = cls->clap.uid.GetData();
		descriptor.name = cls->product.GetData();
		descriptor.vendor = cls->vendor.GetData();
		descriptor.version = cls->version.GetData();

		auto category = cls->category;

		if (category & Class::kAnalyzer)
		{
			features.Push(CLAP_PLUGIN_FEATURE_ANALYZER);
		}
		else
		{
			features.Push(types[cls->type]);
		}

		REFLEX_LOOP(idx, GetArraySize(categories))
		{
			if (BitCheck(category, idx))
			{
				features.Push(categories[idx]);
			}
		}

		features.Push(nullptr);

		descriptor.features = features.GetData();
	}
}

ClapInstance::ClapInstance(ClapSession & pluginsession, const Configuration::Class & cls, const clap_host * host)
	: PluginInstance(pluginsession, cls)
	, m_clap_host(host)
	, m_clap_host_params_rescan(&NullHostParamsRescan)
	, m_clap_host_params_request_flush(&NullHostParamsRequestFlush)
	, m_clap_host_latency_changed(&NullHostLatencyChanged)
	, m_clap_host_gui_request_resize(&NullHostGuiRequestResize)
	, m_clap_host_gui_resize_hints_changed(&NullHostGuiResizeHintsChanged)
	, m_clap_host_state_mark_dirty(&NullHostStateMarkDirty)
	, m_process_clap_rt(nullptr)
	, m_host_window(nullptr)
	, m_reported_latency(0)
{
	REFLEX_DISABLE_WARNINGS	//suppress erroneous MSVC constexpr warning
	if (kPlatform == kPlatformWindows) st_get_platform_pixel_factor = &GetMaxPixelDensity;
	REFLEX_ENABLE_WARNINGS

	clap_plugin::plugin_data = this;
	clap_plugin::init = [](const clap_plugin * plugin) { return true; };
	clap_plugin::destroy = [](const clap_plugin * plugin)
	{
		Release(GetInstance(plugin));
	};
	
	clap_plugin::activate = [](const clap_plugin * plugin, Float64 sample_rate, uint32_t min_frames_count, uint32_t max_frames_count)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin::activate");
		auto instance = GetInstance(plugin);
		instance->PrepareProcessing(max_frames_count, Float32(sample_rate));
		return true;
	};

	clap_plugin::deactivate = [](const clap_plugin * plugin)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin::deactivate");
	};

	clap_plugin::start_processing = [](const clap_plugin * plugin)
	{
		return true;
	};

	clap_plugin::stop_processing = [](const clap_plugin * plugin)
	{
	};

	clap_plugin::reset = [](const clap_plugin * plugin)
	{
		GetInstance(plugin)->m_param_events.Flush();
	};

	clap_plugin::get_extension = [](const clap_plugin *, const char * id) -> const void *
	{
		switch (MakeKey32(CString::View(id)))
		{
			case K32(CLAP_EXT_AUDIO_PORTS): return &st_clap_audio_ports;
			case K32(CLAP_EXT_NOTE_PORTS): return &st_clap_note_ports;
			case K32(CLAP_EXT_PARAMS): return &st_clap_params;
			case K32(CLAP_EXT_STATE): return &st_clap_state;
			case K32(CLAP_EXT_GUI): return &st_clap_gui;
			case K32(CLAP_EXT_LATENCY): return &st_clap_latency;
			default: break;
		}

		return nullptr;
	};

	clap_plugin::on_main_thread = [](const clap_plugin *)
	{
	};

	clap_plugin::process = [](const clap_plugin * plugin, const clap_process * process)
	{
		return GetInstance(plugin)->ProcessRt(process);
	};

	if (auto name = m_clap_host->name)
	{
		session->SetHostName(name);
	}

	if (auto host_params = Cast<clap_host_params>(host->get_extension(host, CLAP_EXT_PARAMS)))
	{
		if (host_params->rescan) m_clap_host_params_rescan = host_params->rescan;
		if (host_params->request_flush) m_clap_host_params_request_flush = host_params->request_flush;
	}

	if (auto host_latency = Cast<clap_host_latency>(host->get_extension(host, CLAP_EXT_LATENCY)))
	{
		if (host_latency->changed) m_clap_host_latency_changed = host_latency->changed;
	}

	if (auto host_gui = Cast<clap_host_gui>(host->get_extension(host, CLAP_EXT_GUI)))
	{
		if (host_gui->request_resize) m_clap_host_gui_request_resize = host_gui->request_resize;
		if (host_gui->resize_hints_changed) m_clap_host_gui_resize_hints_changed = host_gui->resize_hints_changed;
	}

	if (auto host_state = Cast<clap_host_state>(host->get_extension(host, CLAP_EXT_STATE)))
	{
		if (host_state->mark_dirty) m_clap_host_state_mark_dirty = host_state->mark_dirty;
	}

	REFLEX_LOOP(output, 2)
	{
		auto nchannels = (&cls.channels_io.a)[output];

		auto & map = m_channel_maps[output];
		
		map.Allocate(nchannels);

		if (nchannels & 1)
		{
			REFLEX_LOOP(idx, nchannels)
			{
				map.Push<kAllocateNone>({ UInt8(idx), UInt8(idx), UInt8(0) });
			}
		}
		else
		{
			REFLEX_LOOP(idx, nchannels)
			{
				map.Push<kAllocateNone>({ UInt8(idx), UInt8(idx >> 1), UInt8(idx & 1) });
			}
		}
	}

	Initialise();	//m_client is created here

	if (auto process_clap_rt = GetProperty<ObjectOf<ProcessClapRt>>(GetClient(), kNullKey)->value)
	{
		m_process_clap_rt = process_clap_rt;

		clap_plugin::process = [](const clap_plugin * plugin, const clap_process * process)
		{
			auto clap_instance = GetInstance(plugin);

			return clap_instance->m_process_clap_rt(clap_instance->GetClient(), process);
		};
	}
	else
	{
		clap_plugin::process = [](const clap_plugin * plugin, const clap_process * process)
		{
			return GetInstance(plugin)->ProcessRt(process);
		};
	}
}

ClapInstance::~ClapInstance()
{
	Deinitialise();
}

void ClapInstance::OnSetViewSize(const iSize & size)
{
	REFLEX_ASSERT_MAINTHREAD("ClapInstance::OnSetViewSize");

	if (!IsResizingWindow())
	{
		m_clap_host_gui_request_resize(m_clap_host, size.w, size.h);

		// By the time the view has a size, the editor exists and its
		// resizability (m_resizable_window) is known. Nudge the host to
		// re-query the resize hints, in case it cached can_resize == false from
		// a query made before the editor was created (a host may legally call
		// gui.get_size / gui.can_resize between gui.create and gui.set_parent).
		m_clap_host_gui_resize_hints_changed(m_clap_host);
	}
}

void ClapInstance::ReportProcessingDelay(UInt32 delay)
{
	REFLEX_ASSERT_MAINTHREAD("ClapInstance::ReportProcessingDelay");

	m_reported_latency = delay;

	m_clap_host_latency_changed(m_clap_host);
}

void ClapInstance::ReportStateChange(UInt8 change_flags)
{
	REFLEX_ASSERT_MAINTHREAD("ClapInstance::ReportStateChange");

	clap_param_rescan_flags clap_param_flags = 0;

	if (change_flags & kChangeParameterValues) clap_param_flags |= CLAP_PARAM_RESCAN_VALUES;

	if (change_flags & kChangeParameterInfo) clap_param_flags |= (CLAP_PARAM_RESCAN_TEXT | CLAP_PARAM_RESCAN_INFO);

	if (clap_param_flags) m_clap_host_params_rescan(m_clap_host, clap_param_flags);

	if (change_flags & kChangeState)
	{
		m_clap_host_state_mark_dirty(m_clap_host);
	}
}

void ClapInstance::BeginAutomation(UInt32 idx)
{
	QueueUiAutomationEvent(UiAutomationEvent::kBeginAutomation, UInt16(idx), 0.0);
}

void ClapInstance::Automate(UInt32 idx, Float32 value)
{
	QueueUiAutomationEvent(UiAutomationEvent::kAutomate, UInt16(idx), value);
}

void ClapInstance::EndAutomation(UInt32 idx)
{
	QueueUiAutomationEvent(UiAutomationEvent::kEndAutomation, UInt16(idx), 0.0);
}

REFLEX_NOINLINE void ClapInstance::QueueUiAutomationEvent(UiAutomationEvent::Type type, UInt16 idx, Float32 value)
{
	m_param_events.Push({ type, idx, value });

	m_clap_host_params_request_flush(m_clap_host);
}

bool ClapInstance::StoreState(const clap_ostream * stream)
{
	constexpr auto Write = [](const clap_ostream * stream, ArrayView <UInt8> chunk)
	{
		while (chunk)
		{
			auto transferred = Int32(stream->write(stream, chunk.data, chunk.size));

			if (transferred <= 0) return false;

			chunk = Nudge(chunk, transferred);
		}

		return true;
	};

	auto chunk = GetCallbacks().OnGetPluginChunk();
	
	UInt32 size = chunk.GetSize();
	
	if (!Write(stream, { Reinterpret<UInt8>(&size), sizeof(UInt32) })) return false;

	if (!Write(stream, chunk)) return false;

	return true;
}

bool ClapInstance::RestoreState(const clap_istream * stream)
{
	constexpr auto Read = [](const clap_istream * stream, ArrayRegion <UInt8> region)
	{
		while (region)
		{
			auto transferred = Int32(stream->read(stream, region.data, region.size));

			if (transferred <= 0) return false;

			region = Nudge(region, transferred);
		}

		return true;
	};

	UInt32 size = 0;
	
	if (!Read(stream, { Reinterpret<UInt8>(&size), sizeof(UInt32) })) return false;

	Array <UInt8> chunk(size);
	
	if (!Read(stream, chunk)) return false;

	GetCallbacks().OnSetPluginChunk(chunk);
	
	return true;
}

void ClapInstance::ReceiveMidiAndAutomationEventsRt(const clap_input_events & events)
{
	auto get = events.get;

	REFLEX_LOOP(i, events.size(&events))
	{
		auto header = get(&events, i);
	
		if (header->space_id == CLAP_CORE_EVENT_SPACE_ID)
		{
			switch (header->type)
			{
				case CLAP_EVENT_MIDI:
				{
					auto & m = *Reinterpret<clap_event_midi>(header);
					UInt32 value = UInt32(m.data[0]) | (UInt32(m.data[1]) << 8) | (UInt32(m.data[2]) << 16);
					QueueEvent(AudioPlugin::Event::kTypeMIDI, UInt16(Min<UInt32>(header->time, 0x3FFF)), 0, value);
				}
				break;

				case CLAP_EVENT_PARAM_VALUE:
				{
					auto & e = *Reinterpret<clap_event_param_value>(header);
					QueueEvent(AudioPlugin::Event::kTypeAutomation, UInt16(Min<UInt32>(header->time, 0x3FFF)), UInt16(e.param_id), Float32(e.value));
				}
				break;
			}
		}
	}
}

void ClapInstance::SendMidiAndAutomationEventsRt(ArrayView <Event> events, const clap_output_events & out)
{
	auto push = out.try_push;

	for (auto & i : events)
	{
		if (i.type == AudioPlugin::Event::kTypeMIDI)
		{
			clap_event_midi e = {};
			e.header.size = sizeof(clap_event_midi);
			e.header.time = i.position;
			e.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			e.header.type = CLAP_EVENT_MIDI;
			e.port_index = 0;
			e.data[0] = UInt8(i.value.u32 & 0xFF);
			e.data[1] = UInt8((i.value.u32 >> 8) & 0xFF);
			e.data[2] = UInt8((i.value.u32 >> 16) & 0xFF);
			push(&out, &e.header);
		}
		else
		{
			clap_event_param_value e = {};
			e.header.size = sizeof(clap_event_param_value);
			e.header.time = i.position;
			e.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
			e.header.type = CLAP_EVENT_PARAM_VALUE;
			e.param_id = i.idx;
			e.value = i.value.f32;
			push(&out, &e.header);
		}
	}
}

REFLEX_INLINE clap_process_status ClapInstance::ProcessRt(const clap_process * process)
{
	constexpr Float64 kBeatPosMult = 1.0 / Float64(CLAP_BEATTIME_FACTOR);

	auto inputs = GetAudioChannels(false);

	for (auto & indices : m_channel_maps[0])
	{
		auto & input = process->audio_inputs[indices.b];

		inputs[indices.a] = input.data32[indices.c];
	}

	auto outputs = GetAudioChannels(true);

	for (auto & indices : m_channel_maps[1])
	{
		auto & output = process->audio_outputs[indices.b];

		outputs[indices.a] = output.data32[indices.c];
	}

	if (auto in_events = process->in_events) ReceiveMidiAndAutomationEventsRt(*in_events);

	bool clock = false;
	Float64 bps = 1.0;
	Float64 beatpos = 0.0;

	if (process->transport)
	{
		auto & tr = *process->transport;
		clock = (tr.flags & CLAP_TRANSPORT_IS_PLAYING) != 0;
		bps = Max(tr.tempo, 1.0) / 60.0;
		beatpos = tr.song_pos_beats * kBeatPosMult;
	}

	auto out_events = PluginInstance::ProcessRt(process->frames_count, clock, bps, beatpos);

	if (auto host = process->out_events)
	{
		SendAutomationEventsRt(*host);

		SendMidiAndAutomationEventsRt(out_events, *host);
	}

	return CLAP_PROCESS_CONTINUE;
}

void ClapInstance::SendAutomationEventsRt(const clap_output_events & out)
{
	union Wrapper
	{
		clap_event_param_gesture gesture;
		clap_event_param_value value;
	}
	wrapper;

	UiAutomationEvent i;

	auto push = out.try_push;

	while (m_param_events.Pop(i))
	{
		wrapper.value = {};
		wrapper.value.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
		wrapper.value.param_id = i.idx;

		switch (i.type)
		{
		case UiAutomationEvent::kAutomate:
			wrapper.value.header.size = sizeof(clap_event_param_value);
			wrapper.value.header.type = CLAP_EVENT_PARAM_VALUE;
			wrapper.value.value = i.value;
			break;

		case UiAutomationEvent::kBeginAutomation:
			wrapper.gesture.header.size = sizeof(clap_event_param_gesture);
			wrapper.gesture.header.type = CLAP_EVENT_PARAM_GESTURE_BEGIN;
			break;

		case UiAutomationEvent::kEndAutomation:
			wrapper.gesture.header.size = sizeof(clap_event_param_gesture);
			wrapper.gesture.header.type = CLAP_EVENT_PARAM_GESTURE_END;
			break;
		}

		push(&out, &wrapper.gesture.header);
	}
}

const clap_plugin_factory ClapInstance::st_clap_factory =
{
	.get_plugin_count = [](const clap_plugin_factory *) -> uint32_t
	{
		return TheClapPluginSession::Get()->classes.GetSize();
	},
	.get_plugin_descriptor = [](const clap_plugin_factory *, uint32_t index) -> const clap_plugin_descriptor *
	{
		return &TheClapPluginSession::Get()->classes[index].descriptor;
	},
	.create_plugin = [](const clap_plugin_factory *, const clap_host * host, const char * plugin_id) -> const clap_plugin *
	{
		auto session = TheClapPluginSession::Get();

		CString::View uid = CString::View(plugin_id);

		for (auto & i : session->classes)
		{
			auto & cls = *i.reflex_class;

			if (cls.clap.uid == uid)
			{
				auto instance = New<ClapInstance>(session, cls, host).Adr();

				instance->desc = &i.descriptor;

				Retain(instance);

				return Cast<clap_plugin>(instance);
			}
		}

		return nullptr;
	}
};

const clap_plugin_audio_ports ClapInstance::st_clap_audio_ports =
{
	.count = [](const clap_plugin * plugin, bool is_input) -> uint32_t
	{
		auto self = GetInstance(plugin);
		uint32_t channels = is_input ? self->cls.channels_io.a : self->cls.channels_io.b;
		if (!channels) return 0;
		return (channels & 1) ? channels : (channels / 2);
	},
	.get = [](const clap_plugin * plugin, uint32_t index, bool is_input, clap_audio_port_info * info)
	{
		auto self = GetInstance(plugin);

		bool output = !is_input;

		auto bus_channels = (&self->cls.channels_io.a)[output];

		auto stereo = (bus_channels & 1) == 0;

		info->id = index;
		self->MakeAudioPinName(output, stereo, index, info->name, sizeof(info->name));
		info->channel_count = stereo ? 2 : 1;
		info->flags = CLAP_AUDIO_PORT_IS_MAIN;
		info->port_type = stereo ? CLAP_PORT_STEREO : CLAP_PORT_MONO;
		info->in_place_pair = CLAP_INVALID_ID;
		return true;
	},
};

const clap_plugin_note_ports ClapInstance::st_clap_note_ports =
{
	.count = [](const clap_plugin * plugin, bool is_input) -> uint32_t
	{
		auto self = GetInstance(plugin);
		return is_input ? self->cls.midi_io.a : self->cls.midi_io.b;
	},
	.get = [](const clap_plugin *, uint32_t index, bool is_input, clap_note_port_info * info)
	{
		constexpr const char * kLabels[2] = {"MIDI Out", "MIDI In"};
		
		info->id = index;
		RawStringCopy(kLabels[is_input], info->name, sizeof(info->name));
		// Advertise MIDI_MPE alongside plain MIDI so MPE-aware hosts (Bitwig,
		// Reaper) deliver per-channel pitchbend / CC74 / channel-pressure to
		// the plugin instead of consolidating them upstream. Plain MIDI stays
		// supported as a fallback for hosts without MPE input. Plugins that
		// need to differentiate handling can read clap_event_midi data byte 0
		// (channel low nibble) - the MPE convention places member channels
		// 2..16 around master channel 1, but MIDI events are byte-identical
		// to MIDI dialect, so existing parsing works unchanged.
		info->supported_dialects = CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_MIDI_MPE;
		info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI_MPE;
		return true;
	},
};

const clap_plugin_latency ClapInstance::st_clap_latency =
{
	.get = [](const clap_plugin * plugin) -> uint32_t
	{
		return GetInstance(plugin)->m_reported_latency;
	}
};

const clap_plugin_params ClapInstance::st_clap_params =
{
	.count = [](const clap_plugin * plugin) -> uint32_t
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin_params::count");

		return GetInstance(plugin)->cls.num_params;
	},
	.get_info = [](const clap_plugin * plugin, uint32_t param_index, clap_param_info * param_info)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin_params::get_info");

		auto self = GetInstance(plugin);
		if (param_index < self->cls.num_params)
		{
			param_info->id = param_index;
			param_info->flags = CLAP_PARAM_IS_AUTOMATABLE;
			param_info->cookie = nullptr;
			param_info->min_value = 0.0;
			param_info->max_value = 1.0;
			param_info->default_value = self->GetCallbacks().OnGetParameterValue(param_index);
			param_info->module[0] = 0;

			CString name;
			self->GetCallbacks().OnGetParameterInfo(param_index, name);
			RawStringCopy(name.GetData(), param_info->name, sizeof(param_info->name));

			return true;
		}

		return false;
	},
	.get_value = [](const clap_plugin * plugin, clap_id param_id, Float64 * value)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin_params::get_value");
		auto self = GetInstance(plugin);
		if (param_id < self->cls.num_params)
		{
			*value = self->GetCallbacks().OnGetParameterValue(param_id);

			return true;
		}

		return false;
	},
	.value_to_text = [](const clap_plugin *, clap_id, Float64 value, char * out_buffer, uint32_t out_buffer_capacity)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin_params::value_to_text");
		auto temp = ToCString(Float32(value * 100.0f), 1, false);
		RawStringCopy(temp.GetData(), out_buffer, out_buffer_capacity);
		return true;
	},
	.text_to_value = [](const clap_plugin *, clap_id, const char * display, Float64 * value)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin_params::text_to_value");
		*value = Clip(ToFloat32(display), 0.0f, 1.0f);
		return true;
	},
	.flush = [](const clap_plugin * plugin, const clap_input_events * in, const clap_output_events * out)
	{
		auto self = GetInstance(plugin);
	
		if (in)
		{
			if (System::GetThreadID() == kMainThreadID)
			{
				auto get = in->get;
				
				auto & callbacks = self->GetCallbacks();

				REFLEX_LOOP(i, in->size(in))
				{
					auto header = get(in, i);

					if (header->space_id == CLAP_CORE_EVENT_SPACE_ID && header->type == CLAP_EVENT_PARAM_VALUE)
					{
						auto & e = *Reinterpret<clap_event_param_value>(header);

						callbacks.OnSetParameterValue(e.param_id, Float32(e.value));
					}
				}
			}
			else
			{
				self->ReceiveMidiAndAutomationEventsRt(*in);
			}
		}

		if (out)
		{
			self->SendAutomationEventsRt(*out);
		}
	}
};

const clap_plugin_state ClapInstance::st_clap_state =
{
	.save = [](const clap_plugin_t *plugin, const clap_ostream_t *stream)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin_state::save");
		return GetInstance(plugin)->StoreState(stream);
	},
	.load = [](const clap_plugin_t *plugin, const clap_istream_t *stream)
	{
		REFLEX_ASSERT_MAINTHREAD("clap_plugin_state::load");
		return GetInstance(plugin)->RestoreState(stream);
	}
};

const clap_plugin_gui ClapInstance::st_clap_gui =
{
	.is_api_supported = [](const clap_plugin * plugin, const char * api, bool is_floating)
	{
		if (is_floating)
		{
			return false;
		}
		else if (TheClapPluginSession::Get()->config.view_ctr)
		{
			switch (MakeKey32(api))
			{
			case K32(CLAP_WINDOW_API_WIN32):
				return kPlatform == kPlatformWindows;

			case K32(CLAP_WINDOW_API_COCOA):
				return kPlatform == kPlatformMacOS;

			default:
				return false;
			}
		}
		else
		{
			return false;
		}
	},
	.get_preferred_api = [](const clap_plugin * plugin, const char ** api, bool * is_floating)
	{
		*is_floating = false;
	
		switch (kPlatform)
		{
		case kPlatformWindows:
			*api = CLAP_WINDOW_API_WIN32;
			break;

		case kPlatformMacOS:
			*api = CLAP_WINDOW_API_COCOA;
			break;

		default:
			return false;
		}

		return true;
	},
	.create = [](const clap_plugin * plugin, const char * api, bool is_floating)
	{
		return !is_floating && TheClapPluginSession::Get()->config.view_ctr;
	},
	.destroy = [](const clap_plugin * plugin)
	{
		auto instance = GetInstance(plugin);
		instance->DiscardEditor();
		instance->m_host_window = nullptr;
	},
	.set_scale = [](const clap_plugin *, Float64)
	{
		return true;
	},
	.get_size = [](const clap_plugin * plugin, uint32_t * width, uint32_t * height)
	{
		auto instance = GetInstance(plugin);

		// The editor is created lazily (set_parent/show -> ShowEditor). If the
		// host queries the size before then, there is no editor to measure;
		// report failure rather than dereferencing a null editor. The real size
		// is pushed to the host via request_resize once the editor lays out
		// (OnSetViewSize).
		if (!instance->GetEditor())
		{
			return false;
		}

		auto [w, h] = instance->GetEditorContentSize();

		Int pixel_factor = st_get_platform_pixel_factor();

		*width = UInt32(w * pixel_factor);
		*height = UInt32(h * pixel_factor);
		return true;
	},
	.can_resize = [](const clap_plugin * plugin)
	{
		return GetInstance(plugin)->IsEditorResizable();
	},
	.get_resize_hints = [](const clap_plugin * plugin, clap_gui_resize_hints_t * hints)
	{
		auto resizable = GetInstance(plugin)->IsEditorResizable();
	
		hints->can_resize_horizontally = resizable;
		hints->can_resize_vertically = resizable;
		hints->preserve_aspect_ratio = false;
		hints->aspect_ratio_width = 0;
		hints->aspect_ratio_height = 0;
		return true;
	},
	.adjust_size = [](const clap_plugin *, uint32_t *, uint32_t *)
	{
		return true;
	},
	.set_size = [](const clap_plugin * plugin, uint32_t width, uint32_t height)
	{
		auto instance = GetInstance(plugin);

		Int pixel_factor = st_get_platform_pixel_factor();

		instance->SetEditorSize({ Int32(width) / pixel_factor, Int32(height) / pixel_factor });

		return true;
	},
	.set_parent = [](const clap_plugin * plugin, const clap_window * window)
	{
		auto instance = GetInstance(plugin);
	
		instance->m_host_window = window->ptr;

		instance->ShowEditor(window->ptr);

		return true;
	},
	.set_transient = [](const clap_plugin * plugin, const clap_window * window)
	{
		return true;
	},
	.suggest_title = [](const clap_plugin *, const char *) {},
	.show = [](const clap_plugin * plugin)
	{
		auto instance = GetInstance(plugin);
		
		instance->ShowEditor(instance->m_host_window);
			
		return true;
	},
	.hide = [](const clap_plugin * plugin)
	{
		GetInstance(plugin)->DiscardEditor();
		
		return true;
	},
};

REFLEX_END_INTERNAL

extern "C" CLAP_EXPORT const clap_plugin_entry_t clap_entry =
{
	.clap_version = CLAP_VERSION,
	.init = [](const char *)
	{
		Reflex::Retain(Reflex::System::Common::TheClapPluginSession::Acquire(nullptr));

		return true;
	},
	.deinit = []()
	{
		Reflex::Release(Reflex::System::Common::TheClapPluginSession::Get());
	},
	.get_factory = [](const char * factory_id) -> const void *
	{
		if (Reflex::CString::View(factory_id) == CLAP_PLUGIN_FACTORY_ID)
		{
			return &Reflex::System::Common::ClapInstance::st_clap_factory;
		}
		return nullptr;
	}
};
