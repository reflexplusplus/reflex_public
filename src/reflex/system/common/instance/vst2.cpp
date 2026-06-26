#include "vst2.h"

#include "instance.cpp"
#include "plugin.cpp"





//
//vstinstance

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

Vst2Instance::Vst2Instance(PluginSession & pluginsession, VST2API::HostCallback host, const Configuration::Class & cls)
	: PluginInstance(pluginsession, cls)
	, m_audiomaster(host)
	, m_automating(0)
{
	REFLEX_USE(VST2API);

	m_aeffect.object = this;
	m_aeffect.version = 1;
	m_aeffect.dispatcher = &Vst2Instance::OnDispatch;
	m_aeffect.set_parameter = &Vst2Instance::OnSetParameter;
	m_aeffect.get_parameter = &Vst2Instance::OnGetParameter;
	m_aeffect.process_replacing = &Vst2Instance::OnProcessReplacing;

	if (pluginsession.hostname.Empty())
	{
		char vendor[kMaxProductStringLength + 1];

		char product[kMaxProductStringLength + 1];

		m_audiomaster(m_aeffect, kHostGetVendorString, 0, 0, vendor, 0);

		m_audiomaster(m_aeffect, kHostGetProductString, 0, 0, product, 0);

		pluginsession.SetHostName(Join(vendor, " ", product));
	}



	//properties

	m_aeffect.unique_id = cls.vst2.uid;

	m_aeffect.num_inputs = cls.channels_io.a;

	m_aeffect.num_outputs = cls.channels_io.b;

	m_aeffect.num_params = cls.num_params;

	if (cls.type == AudioPlugin::Configuration::Class::kTypeInstrument)
	{
		m_aeffect.flags |= Plugin::kFlagIsSynth;
	}
	else
	{
		m_aeffect.flags &= ~Plugin::kFlagIsSynth;
	}

	m_aeffect.flags |= Plugin::kFlagProgramChunks;	//programsAreChunks

	m_aeffect.flags |= Plugin::kFlagCanReplacing;		//canProcessReplacing();



	//init events out

	MidiEvent ** dest = Reinterpret<MidiEvent*>(m_vsteventsout.vstevents.events);

	REFLEX_LOOP(idx, GetArraySize(m_vsteventsout.midievents))
	{
		auto & vstmidievent = m_vsteventsout.midievents[idx];

		vstmidievent.type = 1;
		vstmidievent.bytesize = sizeof(MidiEvent);
		vstmidievent.delta_frames = 0;
		vstmidievent.flags = 0;
		vstmidievent.note_length = 0;
		vstmidievent.note_offset = 0;
		vstmidievent.detune = 0;
		vstmidievent.note_off_velocity = 0;
		vstmidievent.reserved1 = 0;
		vstmidievent.reserved2 = 0;

		dest[idx] = &vstmidievent;
	}



	//editor

	m_editor_rect.left = 0;
	m_editor_rect.top = 0;
	m_editor_rect.right = 256;
	m_editor_rect.bottom = 256;

	if (pluginsession.config.view_ctr)
	{
		m_aeffect.flags |= Plugin::kFlagHasEditor;
	}
	else
	{
		m_aeffect.flags &= ~Plugin::kFlagHasEditor;
	}


	Initialise();
}

Vst2Instance::~Vst2Instance()
{
	Deinitialise();
}

void Vst2Instance::OnSetViewSize(const iSize & size)
{
	m_editor_rect.right = Int16(size.w);

	m_editor_rect.bottom = Int16(size.h);

	m_audiomaster(m_aeffect, VST2API::kHostSizeWindow, size.w, size.h, 0, 0);
}

void Vst2Instance::ReportStateChange(UInt8 change_flags)
{
	REFLEX_ASSERT_MAINTHREAD("Vst2Instance::ReportStateChange");

	if (EditorOpen() && !m_automating)	//ableton automation freaks out if ui is open and this is called
	{
		m_audiomaster(m_aeffect, VST2API::kHostUpdateDisplay, 0, 0, 0, 0);
	}
}

void Vst2Instance::ReportProcessingDelay(UInt32 delay)
{
	REFLEX_ASSERT_MAINTHREAD("Vst2Instance::ReportProcessingDelay");

	m_aeffect.initial_delay = delay;

	m_audiomaster(m_aeffect, VST2API::kHostIOChanged, 0, 0, 0, 0);
}

void Vst2Instance::Automate(UInt32 idx, Float32 value)
{
	REFLEX_ASSERT(m_automating);

	if (m_automating)
	{
		m_audiomaster(m_aeffect, VST2API::kHostAutomate, idx, 0, 0, value);
	}
}

void Vst2Instance::BeginAutomation(UInt32 idx)
{
	m_automating++;

	m_audiomaster(m_aeffect, VST2API::kHostBeginEdit, idx, 0, 0, 0);
}

void Vst2Instance::EndAutomation(UInt32 idx)
{
	m_audiomaster(m_aeffect, VST2API::kHostEndEdit, idx, 0, 0, 0);

	m_automating--;
}

void Vst2Instance::GetPinProperties(bool output, Int32 index, VST2API::PinProperties & properties)
{
	properties.flags = 3;

	UInt total_channels = (&cls.channels_io.a)[output];

	bool stereo = (total_channels > 1) && ((total_channels & 1) == 0);

	properties.arrangement_type = stereo ? VST2API::kSpeakerArrangementStereo : 0;

	UInt pin_index = stereo ? (UInt(index) / 2) : UInt(index);

	MakeAudioPinName(output, pin_index, total_channels, properties.label, GetArraySize(properties.label));
	RawStringCopy(properties.label, properties.short_label, GetArraySize(properties.short_label));
}

void Vst2Instance::OnSetParameter(VST2API::Plugin & vst, Int32 idx, Float32 value)
{
	//FROM HOST

	auto & self = *Cast<Vst2Instance>(vst.object);

	if (GetThreadID() == kMainThreadID)
	{
		self.GetCallbacks().OnSetParameterValue(idx, value);
	}
	else
	{
		self.QueueEvent(Event::kTypeAutomation, 0, UInt16(idx), value);
	}
}

Reflex::Float32 Vst2Instance::OnGetParameter(VST2API::Plugin & vst, Int32 idx)
{
	auto & self = *Cast<Vst2Instance>(vst.object);

	if (idx < self.m_aeffect.num_params)
	{
		return self.GetCallbacks().OnGetParameterValue(idx);
	}

	return 0.0f;
}

REFLEX_INLINE bool Vst2Instance::GetParameterProperties(Int32 idx, VST2API::ParameterProperties & param)
{
	if (GetThreadID() == kMainThreadID && idx < m_aeffect.num_params)
	{
		CString label;

		GetCallbacks().OnGetParameterInfo(idx, label);

		param.flags = 0;

		Reflex::RawStringCopy(label.GetData(), param.label, GetArraySize(param.label));

		Reflex::RawStringCopy(label.GetData(), param.short_label, GetArraySize(param.short_label));

		param.display_index = Int16(idx);		///< index where this parameter should be displayed (starting with 0)

		return true;
	}
	else
	{
		return false;
	}
}

REFLEX_INLINE void Vst2Instance::GetParameterName(Int32 idx, char * text)
{
	if (GetThreadID() == kMainThreadID && idx < m_aeffect.num_params)
	{
		CString label;

		GetCallbacks().OnGetParameterInfo(idx, label);

		Reflex::RawStringCopy(label.GetData(), text, VST2API::kMaxParameterStringLength);
	}
}

REFLEX_INLINE void Vst2Instance::GetParameterDisplay(Int32 idx, char * text)
{
	if (idx < m_aeffect.num_params)
	{
		auto value = GetCallbacks().OnGetParameterValue(idx);

		char buffer[VST2API::kMaxParameterStringLength];

		auto view = Reflex::Detail::ToCString(value * 100.0, 1, true, ToRegion(buffer));

		MemCopy(view.data, text, view.size);
	}
}

REFLEX_INLINE void Vst2Instance::GetParameterLabel(Int32 idx, char * text) //unit label
{
	text[0] = '%';
	text[1] = 0;
}

void Vst2Instance::OnProcessReplacing(VST2API::Plugin & vst, Float32 ** hostinputs, Float32 ** hostoutputs, Int32 frames)
{
	REFLEX_USE(VST2API);

	auto & self = *Cast<Vst2Instance>(vst.object);

	Int32 filter = (TimeInfo::kFlagPpqPositionValid | TimeInfo::kFlagTempoValid);

	auto ptr = self.m_audiomaster(vst, kHostGetTime, 0, filter, 0, 0);

	auto & timeinfo = *reinterpret_cast<TimeInfo*>(ptr);


	auto inputs = self.GetAudioChannels(false);

	for (auto & input : inputs) input = *hostinputs++;

	auto outputs = self.GetAudioChannels(true);

	for (auto & output : outputs) output = *hostoutputs++;


	auto srcs = self.ProcessRt(frames, True(timeinfo.flags & TimeInfo::kFlagTransportPlaying), timeinfo.tempo * kRcpSixty, timeinfo.ppq_pos);

	if (srcs)
	{
		auto & vsteventsout = self.m_vsteventsout;

		UInt n = srcs.size;

		auto & vstevents = vsteventsout.vstevents;

		vstevents.num_events = n & 63;

		auto dests = vsteventsout.midievents;

		REFLEX_LOOP(idx, n)
		{
			auto & src = srcs[idx];

			REFLEX_ASSERT(src.type == Event::kTypeMIDI);

			auto & vstmidievent = dests[idx & 63];

			vstmidievent.midi_data = src.value.u32;

			vstmidievent.delta_frames = src.position;
		}

		self.m_audiomaster(vst, kHostProcessEvents, 0, 0, &vstevents, 0);
	}
}

UIntNative Vst2Instance::OnDispatch(VST2API::Plugin & e, Int32 opcode, Int32 index, UIntNative value, void * ptr, Float32 opt)
{
	REFLEX_USE(VST2API);

	REFLEX_INLINE_LOCAL(UInt32,GetChunk)(Vst2Instance & self, void ** data)
	{
		REFLEX_ASSERT_MAINTHREAD("Vst2Instance::OnDispatch::GetChunk");

		self.m_chunk = self.GetCallbacks().OnGetPluginChunk();

		(*data) = self.m_chunk.GetData();

		return self.m_chunk.GetSize();
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(UInt32,SetChunk)(Vst2Instance & self, void * data, Int32 size)
	{
		REFLEX_ASSERT_MAINTHREAD("Vst2Instance::OnDispatch::SetChunk");

		ArrayView <UInt8> pluginchunk = { Reinterpret<UInt8>(data), UInt32(size) };

		self.GetCallbacks().OnSetPluginChunk(pluginchunk);

		return size;
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(Int32,CanDo)(const char * ptr)
	{
		CString::View query(ptr);

		for (auto & i : kCanDos)
		{
			if (i == query) return 1;
		}

		return 0;
	}
	REFLEX_END

	REFLEX_INLINE_LOCAL(Int32,ProcessEvents)(Vst2Instance & self, Events & vstevents)
	{
		UInt32 n = vstevents.num_events;

		VST2API::Event ** vstevent = vstevents.events;

		REFLEX_LOOP(idx, n)
		{
			auto & evnt = **(vstevent++);

			if (evnt.type == 1)
			{
				self.QueueEvent(Event::kTypeMIDI, UInt16(evnt.delta_frames), 0, Reinterpret<MidiEvent>(evnt).midi_data);
			}
		}

		return 1;
	}
	REFLEX_END

	auto & self = *Cast<Vst2Instance>(e.object);

	switch (opcode)
	{
	case kPluginProcessEvents:
		return ProcessEvents::Call(self, *(Events*)ptr);


	//parameters

	case kPluginCanBeAutomated:
		return 1;	//canParameterBeAutomated (index) ? 1 : 0;

	//case effString2Parameter:
	//	v = string2parameter (index, (char*)ptr) ? 1 : 0;
	//	break;

	case kPluginGetParamLabel:
		self.GetParameterLabel(index, (char*)ptr);
		return 0;

	case kPluginGetParamDisplay:
		self.GetParameterDisplay(index, (char*)ptr);
		return 0;

	case kPluginGetParamName:
		self.GetParameterName(index, (char*)ptr);
		return 0;

	case kPluginGetParameterProperties:
		return self.GetParameterProperties(index, *(VST2API::ParameterProperties*)ptr);



	//processor

	case kPluginSetSampleRate:
		self.PrepareSampleRate(opt);
		return 0;

	case kPluginSetBlockSize:
		self.PrepareBufferSize(UInt32(value));
		return 0;



	//info

	case kPluginGetInputProperties:
		self.GetPinProperties(false, index, *(VST2API::PinProperties*)ptr);
		return 1;

	case kPluginGetOutputProperties:
		self.GetPinProperties(true, index, *(VST2API::PinProperties*)ptr);
		return 1;

	case kPluginCanDo:
		return CanDo::Call((char*)ptr);

	case kPluginGetPlugCategory:
		if (self.m_aeffect.flags & Plugin::kFlagIsSynth)
		{
			return kPluginCategorySynth;
		}
		else
		{
			return kPluginCategoryEffect;	//kPluginCategoryUnknown;
		}
		break;

	case kPluginGetEffectName:
	case kPluginGetProductString:
		Reflex::RawStringCopy(self.cls.product.GetData(), (char*)ptr, kMaxEffectNameLength);	//kVstMaxProductStrLen
		return 1;

	case kPluginGetVendorString:
		Reflex::RawStringCopy(self.cls.vendor.GetData(), (char*)ptr, kMaxProductStringLength);
		return 1;

	case kPluginGetVendorVersion:
		return 1000;

	case kPluginGetVstVersion:
		return 2400;


	//programs
	//case effGetProgram:
	//	return 0;

	//case effSetProgramName: 	setProgramName ((char*)ptr);						break;

	case kPluginGetProgramName:
		Reflex::RawStringCopy("Default", (char*)ptr, kMaxProgramNameLength);
		//self.getProgramName ((char*)ptr);
		return 0;


	//case effMainsChanged:
	//	if (!value) suspend (); else resume ();
	//	break;


	//Editor
	case kPluginEditGetRect:
		if (self.m_aeffect.flags & Plugin::kFlagHasEditor)
		{
			auto rect = (VST2API::EditorRect**)ptr;

			*rect = &self.m_editor_rect;

			return 1;
		}
		break;

	case kPluginEditOpen:
		if (self.m_aeffect.flags & Plugin::kFlagHasEditor)
		{
			self.ShowEditor(ptr);

			return 1;
		}
		break;

	case kPluginEditClose:
		if (self.m_aeffect.flags & Plugin::kFlagHasEditor)
		{
			self.DiscardEditor();
		}
		break;



	//chunk

	case kPluginGetChunk:
		return GetChunk::Call(self, (void**)ptr/*, index ? true : false*/);

	case kPluginSetChunk:
		return SetChunk::Call(self, ptr, (Int32)value/*, index ? true : false*/);


	case kPluginClose:
		delete &self;
		return 0;
	}

	return 0;
}

REFLEX_END_INTERNAL
