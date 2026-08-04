#include "reflex_ext/bootstrap/audioplugin.h"




//
//AudioPlugin

REFLEX_BEGIN_INTERNAL(Reflex::Bootstrap)

struct NullParamDesc : ParamDesc
{
	NullParamDesc()
	{
		type = kTypeReal;
		min.fvalue = 0.0f;
		max.fvalue = 0.0f;
		init_value.fvalue = 0.0f;
		origin.fvalue = 0.0f;
	}
};

Reflex::Detail::Module::Member <NullParamDesc> g_null_param_info(module);

REFLEX_END_INTERNAL

Reflex::Bootstrap::ParamDesc & Reflex::Bootstrap::ParamDesc::null = Reflex::Bootstrap::g_null_param_info;

Reflex::Bootstrap::AudioPlugin::Parameters::Parameters(AudioPlugin & instance)
	: Streamable(instance.session, MakeKey32("parameters"), 1)
	, instance(instance)
	, paramdefs(global->QueryProperty<ParamDefs>(MakeKey32("bootstrap.paramdefs")))
	, info(paramdefs->value.GetSize())
	, ids(info.GetSize())
	, values(info.GetSize())
{
	auto & defs = paramdefs->value;

	REFLEX_LOOP(idx, info.GetSize())
	{
		auto & paramdef = defs[idx];

		ids[idx] = paramdef.a;

		info[idx] = paramdef.b;
	}
}

void Reflex::Bootstrap::AudioPlugin::Parameters::OnReset(Key32 context)
{
	REFLEX_LOOP(idx, values.GetSize()) values[idx] = info[idx]->init_value;

	instance.ScheduleReportChanges(System::AudioPlugin::kChangeParameterValues);

	instance.Notify(false);
}

void Reflex::Bootstrap::AudioPlugin::Parameters::OnRestore(Data::Archive::View & chunk, Key32 context)
{
	auto pdata = chunk.data;
	
	auto nparam = values.GetSize();
	
	auto nstoredparam = chunk.size / 8;

	auto bytesize32 = nparam * 4;

	auto pvalues = values.GetData();

	if (nstoredparam == nparam)
	{
		if (MemCompare(pdata, ids.GetData(), bytesize32))
		{
			//ids match, fast recall

			MemCopy(pdata + bytesize32, pvalues, bytesize32);

			instance.ScheduleReportChanges(System::AudioPlugin::kChangeParameterValues);

			instance.Notify(false);

			return;
		}
	}

	//ids changed, so remap

	Map <Key32,Value32> values;

	auto pstored = Reinterpret<Value32>(info.GetData() + (nstoredparam * 4));

	REFLEX_LOOP_PTR(Reinterpret<Key32>(info.GetData()), pid, nstoredparam) values[*pid] = *pstored++;

	auto pinfos = info.GetData();

	REFLEX_LOOP_PTR(ids.GetData(), pid, ids.GetSize())
	{
		*pvalues++ = *values.Search(*pid, &(*pinfos++)->init_value);
	}

	instance.ScheduleReportChanges(System::AudioPlugin::kChangeParameterValues);

	instance.Notify(false);
}

void Reflex::Bootstrap::AudioPlugin::Parameters::OnStore(Data::Archive & stream) const
{
	auto bytesize32 = values.GetSize() * 4;

	auto bytes = Extend(stream, bytesize32 * 2).data;

	MemCopy(ids.GetData(), bytes, bytesize32);

	MemCopy(values.GetData(), bytes + bytesize32, bytesize32);
}

Reflex::Bootstrap::AudioPlugin::AudioPlugin(System::AudioPlugin & system, UInt32 magic, UInt16 chunkversion)
	: App(magic, chunkversion)
	, instance(system)
	, m_parameters(*this)
	, m_session_listener(session->CreateListener([this](File::PersistentPropertySet::Notification notification, Key32 ctx)
	{
		if (notification != File::PersistentPropertySet::kNotificationStore)
		{
			ScheduleReportChanges(System::AudioPlugin::kChangeState);
		}
	}))
	, m_report_changes_flags(0)
	, m_automating(0)
{
	System::AudioPlugin::Callbacks::Publish(*this);
	
	if (!IsPlugin()) Detail::RestoreStandaloneAudioApp(global, instance);
}

Reflex::Bootstrap::AudioPlugin::~AudioPlugin()
{
	if (!IsPlugin()) Detail::StoreStandaloneAudioApp(global, instance);

	m_parameters.info.Clear();
}

Reflex::Data::Archive Reflex::Bootstrap::AudioPlugin::OnGetPluginChunk()
{
	Data::Archive rtn;

	Data::Serialize(rtn, *session);

	return rtn;
}

void Reflex::Bootstrap::AudioPlugin::OnSetPluginChunk(const Data::Archive::View & chunk)
{
	auto stream = chunk;

	Data::Deserialize(stream, *session);
}

void Reflex::Bootstrap::AudioPlugin::OnGetParameterInfo(UInt idx, CString & name) const
{
	name = m_parameters.info[idx]->name;
}

Reflex::Float32 Reflex::Bootstrap::AudioPlugin::OnGetParameterValue(UInt idx) const
{
	return Normalise(m_parameters.info[idx], m_parameters.values[idx]);
}

void Reflex::Bootstrap::AudioPlugin::OnSetParameterValue(UInt idx, Float32 value)
{
	m_parameters.values[idx] = Expand(m_parameters.info[idx], value);

	Notify(true);
}

Reflex::FunctionPointer <void(Reflex::System::AudioPlugin::Callbacks&,Reflex::UInt)> Reflex::Bootstrap::AudioPlugin::OnPrepare(UInt32 max_buffersize, Float32 samplerate, ConstTRef <System::AudioPlugin::EventBuffer> events_in, TRef <System::AudioPlugin::EventBuffer> events_out, const ArrayView <const Float32*> & inputs, const ArrayView <Float32*> & outputs)
{
	m_events_in = events_in.Adr();

	m_events_out = events_out.Adr();

	m_audio_in = inputs;

	m_audio_out = outputs;

	Notify(false);

	if (OnPrepareProcessing(max_buffersize, samplerate, m_audio_in.size, m_audio_out.size))
	{
		return [](Callbacks & callbacks, UInt32 samples)
		{
			auto self = Cast<AudioPlugin>(callbacks);

			for (auto & i : self->m_events_in->events)
			{
				if (i.type == System::AudioPlugin::Event::kTypeAutomation)
				{
					self->m_parameters.values[i.idx] = Expand(self->m_parameters.info[i.idx], i.value.f32);

					self->Notify(true);
				}
			}

			auto & events_out_buffer = self->m_events_out_buffer;

			events_out_buffer.Clear();

			self->OnProcessRt(samples, *self->m_events_in, events_out_buffer, self->m_audio_in, self->m_audio_out);

			self->m_events_out->events = events_out_buffer;
		};
	}
	else
	{
		return [](Callbacks & callbacks, UInt32 samples)
		{
			auto self = Cast<AudioPlugin>(callbacks);

			auto bytes = samples * sizeof(Float32);

			for (auto & i : self->m_audio_out)
			{
				MemClear(i, bytes);
			}

			self->m_events_out->events = {};
		};
	}
}

void Reflex::Bootstrap::AudioPlugin::ScheduleReportChanges(UInt8 change_flags)
{
	m_report_changes_flags |= change_flags;

	if (!m_report_changes_scheduler)
	{
		m_report_changes_scheduler = Async::CreateClock(this, [](AudioPlugin & self)
		{
			self.instance->ReportStateChange(Poll(self.m_report_changes_flags));

			self.m_report_changes_scheduler.Clear();
		});
	}
}

bool Reflex::Bootstrap::Detail::SelectAudioDevice(System::AudioPlugin::Lock & lock, const WString::View & device)
{
	//get current config

	auto audioplugin = lock.audioplugin;

	auto previous_sr = audioplugin->GetCurrentSampleRate();

	auto previous_buffersize = audioplugin->GetCurrentBufferSize();

	Array < Pair <WString, bool> > previous_states[2];

	REFLEX_LOOP(idx, 2) previous_states[idx] = audioplugin->GetAudioChannels(idx);


	//select device and restore config where possible

	bool ok = lock.SelectAudioDevice(device);

	REFLEX_LOOP(output, 2)
	{
		auto & previous_state = previous_states[output];

		previous_state.SetSize(lock.audioplugin->GetAudioChannels(output).GetSize());

		REFLEX_LOOP(idx, previous_state.GetSize())
		{
			lock.EnableAudioChannel(True(output), idx, previous_state[idx].b);
		}
	}

	if (Search(audioplugin->GetAvailableSampleRates(), previous_sr))
	{
		lock.RequestSampleRate(previous_sr);
	}
	else
	{
		lock.RequestSampleRate(44100.0f);
	}

	auto buffer_sizes = audioplugin->GetAvailableBufferSizes();

	if (Search(buffer_sizes, previous_buffersize))
	{
		lock.RequestBufferSize(previous_buffersize);
	}
	else if (buffer_sizes)
	{
		UInt32 buffer_size = buffer_sizes.GetLast();

		for (auto & i : buffer_sizes)
		{
			if (i >= 512)
			{
				buffer_size = i;

				break;
			}
		}

		lock.RequestBufferSize(buffer_size);
	}

	return ok;
}

void Reflex::Bootstrap::Detail::RestoreStandaloneAudioApp(Global & global, System::AudioPlugin & audioplugin)
{
	REFLEX_ASSERT(!IsPlugin());

	System::AudioPlugin::Lock lock(audioplugin);

	if (auto stream = Data::GetBinary(global.prefs, MakeKey32("Bootstrap:AudioMidiSettings")))
	{
		constexpr auto RestoreChannels = [](Data::Archive::View & stream, System::AudioPlugin::Lock & lock, bool output, decltype (&System::AudioPlugin::GetAudioChannels) getfn, decltype (&System::AudioPlugin::Lock::EnableAudioChannel) setfn)
		{
			auto & audioplugin = *lock.audioplugin;

			auto channels = (audioplugin.*getfn)(output);

			WString channel;

			REFLEX_LOOP(idx, Data::Deserialize<UInt16>(stream))
			{
				Data::DeserializeUCS2(stream, channel);

				if (auto result = Search<KeyCompare>(channels, channel))
				{
					(lock.*setfn)(output, result.value, true);
				}
			}
		};

		auto device = Data::DeserializeUCS2(stream);

		auto [samplerate, buffer_size] = Data::Deserialize<Float32, UInt32>(stream);

		if (lock.SelectAudioDevice(device))
		{
			lock.RequestSampleRate(samplerate);

			lock.RequestBufferSize(buffer_size);

			REFLEX_LOOP(idx, 2)
			{
				bool output = True(idx);

				RestoreChannels(stream, lock, output, &System::AudioPlugin::GetAudioChannels, &System::AudioPlugin::Lock::EnableAudioChannel);

				RestoreChannels(stream, lock, output, &System::AudioPlugin::GetMidiPorts, &System::AudioPlugin::Lock::EnableMidiPort);
			}

			return;
		}
	}

	if (auto defaultdevice = System::AudioPlugin::GetDefaultDevice(false))
	{
		SelectAudioDevice(lock, defaultdevice);

		auto nchannel = Min<UInt>(audioplugin.GetAudioChannels(true).GetSize(), 2);

		REFLEX_LOOP(idx, nchannel) lock.EnableAudioChannel(true, idx, true);
	}
}

void Reflex::Bootstrap::Detail::StoreStandaloneAudioApp(Global & global, System::AudioPlugin & audioplugin)
{
	REFLEX_ASSERT(!IsPlugin());

	constexpr auto StoreChannels = [](Data::Archive & archive, const Array < Pair <WString, bool> > & channels)
	{
		Data::Marker <UInt16> nchannel(archive);

		UInt16 n = 0;

		for (auto & i : channels)
		{
			if (i.b)
			{
				Data::SerializeUCS2(archive, i.a);

				n++;
			}
		}

		nchannel.Set(n);
	};

	Data::Archive stream;

	Data::SerializeUCS2(stream, audioplugin.GetCurrentAudioDevice());

	Data::Serialize(stream, audioplugin.GetCurrentSampleRate(), audioplugin.GetCurrentBufferSize());

	REFLEX_LOOP(idx, 2)
	{
		bool output = True(idx);

		StoreChannels(stream, audioplugin.GetAudioChannels(output));

		StoreChannels(stream, audioplugin.GetMidiPorts(output));
	}

	Data::SetBinary(global.prefs, MakeKey32("Bootstrap:AudioMidiSettings"), stream);
}
