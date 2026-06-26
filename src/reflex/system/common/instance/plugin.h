#pragma once

#include "../module.h"
#include "instance.h"
#include "eventqueue.h"




//
//declarations

REFLEX_NS(Reflex::System::Common)

enum PluginFormat : UInt32
{
	kPluginFormatVST2 = CC32("VST2"),
	kPluginFormatVST3 = CC32("VST3"),
	kPluginFormatCLAP = CC32("CLAP"),
	kPluginFormatAAX  = CC32("AAX_"),
	kPluginFormatAUv2 = CC32("AUv2"),
	kPluginFormatAUv3 = CC32("AUv3"),
};

struct ReflexInstantiator
{
	ReflexInstantiator(void * hinstance)
	{
		SetInstanceHandle(hinstance);

		root_module.Init();
	}

	~ReflexInstantiator()
	{
		root_module.Deinit();
	}
};

struct PluginSession : public Object
{
	PluginSession(void * hinstance, PluginFormat format)
		: reflex(hinstance)
		, format(format)
		, client(AudioPlugin::OnStart({}, config))
	{
	}

	void SetHostName(CString && value)
	{
		hostname = std::move(value);

		g_plugin_host = hostname;
	}


	const ReflexInstantiator reflex;

	const PluginFormat format;

	AudioPlugin::Configuration config;

	const Reference <Object> client;

	CString hostname;
};

class PluginInstance : public AudioPlugin
{
public:

	static constexpr CString::View kInOut[2] = { "In", "Out" };

	static constexpr Float64 kRcpSixty = 1.0 / 60.0;


	static PluginInstance* IsOpeningPluginWindow(void * hostwindow);	//hacky, called by window


	void ShowEditor(void * hostwindow)
	{
		m_hostwindow = hostwindow;

		if (!m_editor)
		{
			if (auto & view_ctr = session->config.view_ctr)
			{
				st_pluginwindowinfo = { this, hostwindow };

				view_ctr(*this, hostwindow);

				REFLEX_ASSERT(m_editor);	//ensure PluginEditor set m_editor

				st_pluginwindowinfo = {};
			}
		}
	}

	void DiscardEditor()
	{
		CloseEditor();

		m_hostwindow = nullptr;
	}

	TRef <Window> GetEditor() { return m_editor; }

	void SetEditorSize(const iSize & size)
	{
		//this guard is because both host and plugin can call SetRect, and a feedback loop can occur

		m_host_resizing++;

		m_editor->SetRect({ {}, size });

		m_host_resizing--;
	}

	iSize GetEditorContentSize() const { return m_editor->GetClient()->OnGetContentSize(); }

	bool IsEditorResizable() const { return m_resizable_window; }

	bool IsResizingWindow() const { return m_host_resizing; }



	//callbacks

	virtual void OnSetViewSize(const iSize & size) = 0;



	//links

	const Reference <PluginSession> session;

	const Configuration::Class cls;



protected:

	PluginInstance(PluginSession & session, const Configuration::Class & cls);

	
	void Initialise();

	void Deinitialise();


	//interface for derived impl

	bool PrepareProcessing(UInt32 buffersize, Float32 sr)
	{
		if (Or(SetFiltered(m_buffersize, buffersize), SetFiltered(m_samplerate, sr)))
		{
			m_process_rt = m_callbacks->OnPrepare(m_buffersize, m_samplerate, m_eventsin, m_eventsout, Reinterpret<Array<const Float*>>(m_audiochannels[0]), m_audiochannels[1]/*cls.nchannels.a, cls.nchannels.b*/);

			return true;
		}

		return false;
	}

	bool PrepareBufferSize(UInt buffersize)
	{
		if (SetFiltered(m_buffersize, buffersize))
		{
			m_process_rt = m_callbacks->OnPrepare(m_buffersize, m_samplerate, m_eventsin, m_eventsout, Reinterpret<Array<const Float*>>(m_audiochannels[0]), m_audiochannels[1]/*cls.nchannels.a, cls.nchannels.b*/);

			return true;
		}

		return false;
	}

	bool PrepareSampleRate(Float32 sr)
	{
		if (SetFiltered(m_samplerate, sr))
		{
			m_process_rt = m_callbacks->OnPrepare(m_buffersize, m_samplerate, m_eventsin, m_eventsout, Reinterpret<Array<const Float*>>(m_audiochannels[0]), m_audiochannels[1]/*cls.nchannels.a, cls.nchannels.b*/);

			return true;
		}

		return false;
	}

	REFLEX_INLINE ArrayRegion <Float*> GetAudioChannels(bool output) { return ToRegion(m_audiochannels[output]); }

	template <class TYPE> REFLEX_INLINE void QueueEvent(AudioPlugin::Event::Type type, UInt16 position, UInt16 idx, TYPE value) { m_eventsinqueue.Append(type, position, idx, value); }

	ArrayView <Event> ProcessRt(UInt samples, bool clock, Float64 bps, Float64 beatpos);

	bool EditorOpen() const { return True(m_editor); }


	static void MakeAudioPinName(bool output, bool stereo, UInt pin_index, char * out_buffer, UInt out_buffer_capacity);


	//overrides

	void OpenEditor() override;

	void CloseEditor() override;


	UInt32 GetCurrentBufferSize() const override final { return m_buffersize; }

	Float32 GetCurrentSampleRate() const override final { return m_samplerate; }


	TRef <Object> GetClient() override final { return m_client; }

	REFLEX_INLINE AudioPlugin::Callbacks & GetCallbacks() { return m_callbacks; }



private:

	template <class BASE> friend struct PluginWindow;


	Array < Pair <WString, bool> > GetMidiPorts(bool output) const override final { return {}; }


	Array <WString> GetAvailableAudioDevices() const override final { return {}; }

	WString GetCurrentAudioDevice() const override final { return {}; }


	Array <UInt32> GetAvailableBufferSizes() const override final { return {}; }

	Array <Float32> GetAvailableSampleRates() const override final { return {}; }


	Array < Pair <WString, bool> > GetAudioChannels(bool output) const override final;



	Reference <Object> m_client;

	TRef <AudioPlugin::Callbacks> m_callbacks;


	EventInQueue m_eventsinqueue;

	AudioPlugin::EventBuffer m_eventsin, m_eventsout;

	Array <Float*> m_audiochannels[2];

	UInt m_buffersize;

	Float32 m_samplerate;

	FunctionPointer <void(Callbacks&,UInt)> m_process_rt;


	void * m_hostwindow;

	Reference <Window> m_editor;

	UInt16 m_host_resizing;

	bool m_resizable_window;


	static inline Pair <PluginInstance*,void*> st_pluginwindowinfo = {};

};

template <class BASE> 
struct PluginWindow : public BASE
{
	template <class ...VARGS> PluginWindow(PluginInstance & plugin, bool resizable, VARGS &&... v)
		: BASE(std::forward<VARGS>(v)...)
		, plugin(plugin)
	{
		plugin.m_resizable_window = resizable;
	}

	virtual void RequestResize(const iSize & size, Int32 wdelta, Int32 hdelta) = 0;

	void SetClient(TRef <Window::Client> client) final
	{
		BASE::SetClient(client);

		plugin.m_editor = this;
	}

	PluginInstance & plugin;
};

REFLEX_END

REFLEX_INLINE Reflex::ArrayView <Reflex::System::AudioPlugin::Event> Reflex::System::Common::PluginInstance::ProcessRt(UInt samples, bool clock, Float64 bps, Float64 beatpos)
{
	m_eventsin.clock = clock;

	m_eventsin.bps = bps;

	m_eventsin.beatpos = beatpos;

	m_eventsin = m_eventsinqueue.Flush(clock, bps, beatpos);

	m_eventsout.events = {};

	m_process_rt(m_callbacks, samples);

	return m_eventsout.events;
}

REFLEX_INLINE Reflex::System::Common::PluginInstance * Reflex::System::Common::PluginInstance::IsOpeningPluginWindow(void * hostwindow)
{
	if (hostwindow == st_pluginwindowinfo.b)
	{
		auto plugin = st_pluginwindowinfo.a;

		st_pluginwindowinfo = {};

		return plugin;
	}
	
	return nullptr;
}
