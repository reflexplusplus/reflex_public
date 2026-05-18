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

	using ParamDefs = ObjectOf < Array < Pair < Key32, ConstReference <ParamDesc> > > >;



	//lifetime

	AudioPlugin(UInt32 magic, System::AudioPlugin & system);

	~AudioPlugin();



	//parameters

	UInt32 GetNumParameter() const { return m_parameters.ids.GetSize(); }

	ConstTRef <ParamDesc> GetParameterInfo(UInt32 idx) const { return m_parameters.info[idx]; }


	ArrayView <Key32> GetParameterIds() const { return m_parameters.ids; }

	ArrayView <Value32> GetParameterValues() const { return m_parameters.values; }



	//links
	
	const TRef <System::AudioPlugin> instance;

	
	
protected:
	
	virtual bool OnPrepareProcessing(UInt32 max_buffersize, Float32 samplerate, UInt num_input, UInt num_output) = 0;

	virtual void OnProcessRt(UInt num_samples, const System::AudioPlugin::EventBuffer & events_in, Array <System::AudioPlugin::Event> & events_out, const ArrayView <const Float*> & inputs, const ArrayView <Float*> & outputs) = 0;


	void ScheduleReportChanges(UInt8 change_flags);



private:
	
	Data::Archive OnGetPluginChunk() final;

	void OnSetPluginChunk(const Data::Archive::View & chunk) final;


	void OnGetParameterInfo(UInt idx, CString & name) const final;

	Float32 OnGetParameterValue(UInt idx) const final;

	void OnSetParameterValue(UInt idx, Float32 value) final;


	FunctionPointer <void(Callbacks&,UInt)> OnPrepare(UInt32 max_buffersize, Float32 samplerate, ConstTRef <System::AudioPlugin::EventBuffer> events_in, TRef <System::AudioPlugin::EventBuffer> events_out, const ArrayView <const Float32*> & inputs, const ArrayView <Float32*> & outputs) final;


	struct Parameters : public Streamable
	{
		Parameters(AudioPlugin & instance);
		
		void OnReset(Key32 context) override;
	
		void OnRestore(Data::Archive::View & chunk, Key32 context) override;

		void OnStore(Data::Archive & stream) const override;


		AudioPlugin & instance;
		
		const TRef <ParamDefs> paramdefs;
		
		Array < ConstReference <ParamDesc> > info;
		
		Array <Key32> ids;

		Array <Value32> values;

		Reference <Object> session_listener;
	};
	
	Parameters m_parameters;


	const System::AudioPlugin::EventBuffer * m_events_in;

	System::AudioPlugin::EventBuffer * m_events_out;

	ArrayView <const Float32*> m_audio_in;

	ArrayView <Float32*> m_audio_out;

	Array <System::AudioPlugin::Event> m_events_out_buffer;


	Reference <Object> m_session_listener;

	Reference <Object> m_report_changes_scheduler;

	UInt8 m_report_changes_flags;

};




//
//Detail

namespace Reflex::Bootstrap::Detail
{

	bool SelectAudioDevice(System::AudioPlugin::Lock & audioplugin, const WString::View & device);

	void RestoreStandaloneAudioApp(Global & global, System::AudioPlugin & audioplugin);

	void StoreStandaloneAudioApp(Global & global, System::AudioPlugin & audioplugin);

}
