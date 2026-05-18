#include "app.h"




//
//SVGDemo::App implementation

using namespace Reflex;

REFLEX_BEGIN_INTERNAL(SVGDemo)

struct AppImpl :
	public App,
	public Bootstrap::Streamable
{
	static constexpr UInt16 kChunkVersion = 1;

	AppImpl()
		: App(K32("SVGDemo"))
		, Bootstrap::Streamable(session, K32("app"), kChunkVersion)
	{
		output.Log("SVG Demo constructed");
	}



	//put your interface implementation here, eg

	void SetPath(const WString & path) override
	{
		m_svg_path = path;
	
		Notify(true);	//publish state change
	}

	const WString & GetPath() const override
	{
		return m_svg_path;
	}



	//Data::iStreamable callbacks

	void OnReset(Key32 context) override
	{
		m_svg_path.Clear();
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
		Data::DeserializeUTF8(stream, m_svg_path);
	}

	bool OnImport(UInt16 version, Data::Archive::View & stream, Key32 context) override
	{
		return false;
	}

	void OnStore(Data::Archive & stream) const override
	{
		//if you change/add stored data here, you will need to increment kChunkVersion, as previous chunks will be invalid
		//after changing the kChunkVersion, previous version chunks will be restored by the OnImport callback

		Data::SerializeUTF8(stream, m_svg_path);
	}


	
	//state

	WString m_svg_path;

};

REFLEX_END_INTERNAL

TRef <SVGDemo::App> SVGDemo::App::Create()
{
	return New<SVGDemo::AppImpl>();
}

Reflex::Output SVGDemo::output("SVG Demo");
