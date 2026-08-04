#pragma once

#include "instance.h"




//
//declarations

REFLEX_NS(Reflex::System::Common)

template <class TYPE> class Standalone;

typedef Standalone <System::App> NonAudioApp;

REFLEX_END




//
//standalone

template <class TYPE>
class Reflex::System::Common::Standalone : public TYPE
{
public:

	//singleton access

	static Standalone * Get();



	//lifetime

	Standalone();

	~Standalone();



	//assign

	template <bool OPEN_EDITOR = true> TRef <Object> Initialise(ArrayView <CString::View> cmdline);

	void Deinitialise();

	TRef <Object> GetGlobal() { return m_global; }

	TRef <Object> GetClient() override;

	//editor

	virtual void OpenEditor() override;

	virtual void CloseEditor() override;



	//config

	const typename TYPE::Configuration config;




private:

	virtual void OnStartAudio(Object & client) {}

	virtual void OnStopAudio() {}

	virtual void OnReleaseData() final {}


	Reference <Object> m_global;

	TRef <Object> m_client;

	Reference <Window> m_window;


	static inline Standalone * st_self = nullptr;
};




//
//implementation

template <class TYPE> inline Reflex::System::Common::Standalone<TYPE> * Reflex::System::Common::Standalone<TYPE>::Get()
{
	return st_self;
}

template <class TYPE> inline Reflex::System::Common::Standalone<TYPE>::Standalone()
{
	st_self = this;

	g_plugin_host = "Standalone";
}

template <class TYPE> inline Reflex::System::Common::Standalone<TYPE>::~Standalone()
{
	st_self = nullptr;
}

template <class TYPE> template <bool OPEN_EDITOR> inline Reflex::TRef <Reflex::Object> Reflex::System::Common::Standalone<TYPE>::Initialise(ArrayView <CString::View> cmdline)
{
	REFLEX_STATIC_ASSERT((IsType<TYPE,App,AudioPlugin>::value));

	REFLEX_ASSERT(!m_global && !m_client);

	App::Attach(App::list);

	Retain(*this);

	m_global = TYPE::OnStart(cmdline, RemoveConst(config));

	if constexpr (IsType<TYPE,AudioPlugin>::value)
	{
		if (auto & classes = config.classes)
		{
			m_client = config.instance_ctr(m_global, classes.GetFirst(), *this);

			goto RunApp;
		}
		else if (auto & ctr = Cast<App::Configuration>(config).instance_ctr)
		{
			//this shouldnt happen if correct libs are used

			m_client = ctr(m_global, *this);

			goto RunApp;
		}

		DEV_ERROR(this->object_t->tname, "Configuration instance_ctr not defined");
	}
	else if constexpr (IsType<TYPE,App>::value)
	{
		if (m_global)
		{
			if (auto & ctr = config.instance_ctr)
			{
				m_client = ctr(m_global, *this);

				goto RunApp;
			}

			DEV_WARN(this->object_t->tname, "Configuration instance_ctr not defined");	//not hard error here, could be console app
		}
	}

	m_global.Clear();

	return Object::null;


	//found entry point

	REFLEX_MARKER(RunApp);

	Retain(m_client);

	if constexpr (IsType<TYPE,AudioPlugin>::value) OnStartAudio(m_client);

	if constexpr (OPEN_EDITOR) OpenEditor();

	return m_global;
}

template <class TYPE> inline void Reflex::System::Common::Standalone<TYPE>::Deinitialise()
{
	CloseEditor();

	if constexpr (IsType<TYPE,AudioPlugin>::value) OnStopAudio();

	m_client->ReleaseData();	//use this to ensure audiopluginclient releases circulars

	Release(m_client);		//!important, to ensure client is released before derived System::Instance impl destructor, but we dont want reference left set, because Instance delegate will likely access GetClient in its destructor

	Release(*this);
}

template <class TYPE> inline void Reflex::System::Common::Standalone<TYPE>::OpenEditor()
{
	if (!m_window)
	{
		if (auto & ctr = config.view_ctr)
		{
			m_window = ctr(*this, nullptr);
		}
	}
}

template <class TYPE> inline void Reflex::System::Common::Standalone<TYPE>::CloseEditor()
{
	m_window.Clear();
}

template <class TYPE> Reflex::TRef <Reflex::Object> Reflex::System::Common::Standalone<TYPE>::GetClient()
{
	return m_client;
}
