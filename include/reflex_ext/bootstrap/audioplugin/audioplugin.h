#pragma once

#include "[require].h"
#include "paraminfo.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	class AudioPlugin;

}




//
//AudioPlugin

class Reflex::Bootstrap::AudioPlugin :
	public App,
	public System::AudioPlugin::Callbacks
{
public:

	REFLEX_OBJECT(AudioPlugin, Object);



	//types

	using Class = System::AudioPlugin::Configuration::Class;

	using NoteInfo = System::AudioPlugin::NoteInfo;

	using Event = System::AudioPlugin::Event;

	using EventBuffer = System::AudioPlugin::EventBuffer;

	using ParamDesc = ParamDesc;



	//lifetime

	~AudioPlugin();



	//parameters

	UInt32 GetNumParameter() const { return m_parameters.ids.GetSize(); }

	ConstTRef <ParamDesc> GetParameterInfo(UInt32 idx) const { return m_parameters.info[idx]; }


	ArrayView <Key32> GetParameterIDs() const { return m_parameters.ids; }

	ArrayView <Value32> GetParameterValues() const { return m_parameters.values; }


	void BeginAutomation(UInt32 idx);

	void Automate(UInt32 idx, Float32 value);

	void EndAutomation(UInt32 idx);



	//links
	
	const TRef <System::AudioPlugin> instance;

	
	
protected:
	
	//lifetime

	AudioPlugin(System::AudioPlugin & owner, UInt32 magic, UInt16 chunkversion);



	//callbacks

	virtual bool OnPrepareProcessing(UInt32 max_buffersize, Float32 samplerate, UInt num_input, UInt num_output) = 0;

	virtual void OnProcessRt(UInt num_samples, UInt32 parameter_change_flags, const EventBuffer & events_in, Array <Event> & events_out, const ArrayView <const Float*> & inputs, const ArrayView <Float*> & outputs) = 0;



	//update host

	void PublishNoteInfo(ArrayView <NoteInfo> infos);

	void ScheduleReportChanges(UInt8 change_flags);



private:
	
	Data::Archive OnGetPluginChunk() final;

	void OnSetPluginChunk(const Data::Archive::View & chunk) final;


	void OnGetParameterInfo(UInt idx, CString & name) const final;

	Float32 OnGetParameterValue(UInt idx) const final;

	void OnSetParameterValue(UInt idx, Float32 value) final;

	void OnGetNoteInfo(Array <NoteInfo> & infos) const final;


	FunctionPointer <void(Callbacks&,UInt)> OnPrepare(UInt32 max_buffersize, Float32 samplerate, ConstTRef <EventBuffer> events_in, TRef <EventBuffer> events_out, const ArrayView <const Float32*> & inputs, const ArrayView <Float32*> & outputs) final;


	struct Parameters : public Streamable
	{
		Parameters(AudioPlugin & instance);
		
		void OnReset(Key32 context) override;
	
		void OnRestore(Data::Archive::View & chunk, Key32 context) override;

		void OnStore(Data::Archive & stream) const override;


		AudioPlugin & instance;
		
		const TRef < ObjectOf < Array <Pair < Key32, ConstReference <ParamDesc> > > > > paramdefs;
		
		Array < ConstReference <ParamDesc> > info;
		
		Array <Key32> ids;

		Array <Value32> values;

		UInt32 all_change_flags;

		Reference <Object> session_listener;
	};
	
	Parameters m_parameters;


	const EventBuffer * m_events_in;

	EventBuffer * m_events_out;

	ArrayView <const Float32*> m_audio_in;

	ArrayView <Float32*> m_audio_out;

	Array <System::AudioPlugin::NoteInfo> m_note_info;

	Array <Event> m_events_out_buffer;


	Reference <Object> m_session_listener;

	Reference <Object> m_report_changes_scheduler;

	UInt8 m_report_changes_flags;

	AtomicUInt32 m_atomic_change_flags;

	UInt8 m_automating;
};




//
//Detail

namespace Reflex::Bootstrap::Detail
{

	bool SelectAudioDevice(System::AudioPlugin::Lock & audioplugin, const WString::View & device);

	void RestoreStandaloneAudioApp(Global & global, System::AudioPlugin & audioplugin);

	void StoreStandaloneAudioApp(Global & global, System::AudioPlugin & audioplugin);

}




//
//impl

inline void Reflex::Bootstrap::AudioPlugin::BeginAutomation(UInt32 idx)
{
	REFLEX_ASSERT_MAINTHREAD("Bootstrap::AudioPlugin::BeginAutomation");
	REFLEX_ASSERT(idx < m_parameters.values.GetSize());

	m_automating++;

	instance->BeginAutomation(idx);
}

inline void Reflex::Bootstrap::AudioPlugin::Automate(UInt32 idx, Float32 value)
{
	REFLEX_ASSERT_MAINTHREAD("Bootstrap::AudioPlugin::Automate");
	REFLEX_ASSERT(idx < m_parameters.values.GetSize());
	REFLEX_ASSERT(m_automating);

	OnSetParameterValue(idx, value);

	instance->Automate(idx, value);
}

inline void Reflex::Bootstrap::AudioPlugin::EndAutomation(UInt32 idx)
{
	REFLEX_ASSERT_MAINTHREAD("Bootstrap::AudioPlugin::EndAutomation");
	REFLEX_ASSERT(idx < m_parameters.values.GetSize());

	instance->EndAutomation(idx);

	m_automating--;
}
