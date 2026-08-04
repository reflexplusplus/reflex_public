#include "reflex_ext/bootstrap/app.h"




//
//impl

Reflex::Bootstrap::App::App(UInt32 magic, UInt16 chunkversion)
	: Streamable(New<File::PersistentPropertySet>(Data::kPropertySetFormat), MakeKey32("app"), chunkversion)
	, magic(magic)
	, session(propertyset)
{
	SetProperty("session", session);	//this is "published" because PublishAppView needs to access it (due to legacy dependencies, PublishAppView can not assume app is a Bootstrap::App)

	AttachSessionListener();
}

void Reflex::Bootstrap::App::OnReleaseData()
{
	//slight mis-use of this callback (the main purpose is for draining circular references in VM)
	//System::Instance guaranetees to call it prior to destruction, so we use it here to avoid c++ problem of virtual functions not working in destructor

	Data::Archive stream;

	session->Serialize(stream);

	Data::SetBinary(global->prefs, magic, stream);

	UnsetProperty<File::PersistentPropertySet>("session");
}

bool Reflex::Bootstrap::App::Open(const WString & path)
{
	bool ok = false;

	auto data = File::Open(path);

	if (data.GetSize() > 4)
	{
		m_session_listener.Clear();	//avoid feedback

		const decltype (&App::OnImport) fns[2] = { &App::Open, &App::OnImport };

		for (auto fn : fns)
		{
			if ((this->*fn)(path, data))
			{
				m_filename = path;

				m_edited = false;

				Notify(false);	//need to manually notify because detached listener

				ok = true;

				break;
			}
		}

		AttachSessionListener();
	}

	return ok;
}

bool Reflex::Bootstrap::App::Save(const WString & path)
{
	Data::Archive stream;

	Data::Serialize(stream, magic);

	m_session_listener.Clear();

	session->Serialize(stream);

	AttachSessionListener();

	if (File::Save(path, stream))
	{
		m_filename = path;

		m_edited = false;

		Notify(false);

		return true;
	}

	return false;
}

void Reflex::Bootstrap::App::Notify(bool edited)
{
	if (edited) m_edited = true;

	StateMt::Notify();
}

void Reflex::Bootstrap::App::AttachSessionListener()
{
	if (!m_session_listener)
	{
		m_session_listener = session->CreateListener([this](File::PersistentPropertySet::Notification n, Key32 context)
		{
			constexpr Key32 kPresetInfoKey = "app.presetinfo";

			switch (n)
			{
			case File::PersistentPropertySet::kNotificationReset:
				m_filename.Clear();
				m_edited = false;
				Notify(false);
				break;

			case File::PersistentPropertySet::kNotificationRestore:
				if (auto stream = Data::GetBinary(session, kPresetInfoKey))
				{
					Data::Deserialize(stream, m_edited);

					Data::DeserializeUCS2(stream, m_filename);

					Notify(false);
				}
				break;

			case File::PersistentPropertySet::kNotificationStore:
				{
					Data::Archive stream;

					Data::Serialize(stream, m_edited);

					Data::SerializeUCS2(stream, m_filename);

					Data::SetBinary(session, kPresetInfoKey, stream);
				}
				break;

			default:	//passify android studio
				break;
			}
		});
	}
}

bool Reflex::Bootstrap::App::Open(const WString::View & path, const Data::Archive::View & data)
{
	if (Data::Unpack<UInt32>(data) == magic)
	{
		auto stream = Splice(data, 4).b;

		return session->Deserialize(stream, File::PersistentPropertySet::kContextPreset) == Data::SerializableFormat::kDeserializeErrorNone;
	}
	else
	{
		return false;
	}
}
