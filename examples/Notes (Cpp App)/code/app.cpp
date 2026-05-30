#include "app.h"




//
//NotesCppApp::App implementation

namespace NotesCppApp { namespace {	//begin internal namespace

struct AppImpl : public App
{
	static constexpr UInt16 kChunkVersion = 1;

	AppImpl()
		: App(MakeKey32("NotesCppApp"), kChunkVersion)
	{
		output.Log("Notes (Cpp App) constructed");
	}



	//interface implementation

	void AddNote() override
	{
		m_notes.Push();

		Notify(true);
	}

	void SetNote(UInt idx, const Data::Archive & utf8) override
	{
		if (idx < m_notes.GetSize())
		{
			if (SetFiltered(m_notes[idx], utf8))
			{
				Notify(true);
			}
		}
	}

	void DeleteNote(UInt idx) override
	{
		if (idx < m_notes.GetSize())
		{
			m_notes.Remove(idx);

			if (!m_notes) m_notes.Push();

			Notify(true);
		}
	}

	UInt GetNumNotes() const override
	{
		return m_notes.GetSize();
	}

	Data::Archive::View GetNote(UInt idx) const override
	{
		if (idx < m_notes.GetSize())
		{
			return m_notes[idx];
		}
		else
		{
			return {};
		}
	}



	//Data::iStreamable callbacks

	void OnReset(Key32 context) override
	{
		m_notes.Clear();

		m_notes.Push();
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		Data::Deserialize(stream, m_notes);
	}

	bool OnImport(UInt16 version, Data::Archive::View & stream, Key32 context) override
	{
		return false;
	}

	void OnStore(Data::Archive & stream) const override
	{
		//if you change/add stored data here, you will need to increment kChunkVersion, as previous chunks will be invalid
		//after changing the kChunkVersion, previous version chunks will be restored by the OnImport callback

		Data::Serialize(stream, m_notes);
	}



	//data

	Array <Data::Archive> m_notes;

};

} }	//end internal namesapce

Reflex::TRef <NotesCppApp::App> NotesCppApp::App::Create()
{
	return New<NotesCppApp::AppImpl>();
}

Reflex::Output NotesCppApp::output("Notes (Cpp App)");
