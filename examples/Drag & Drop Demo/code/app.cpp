#include "app.h"




//
//DragDropDemo::App implementation

namespace DragDropDemo { namespace {	//begin internal namespace

using namespace Reflex;

struct AppImpl :
	public App,
	public Bootstrap::Streamable
{
	static constexpr UInt16 kChunkVersion = 0;

	AppImpl()
		: App(K32("DragDropDemo"))
		, Bootstrap::Streamable(session, K32("app"), kChunkVersion)
	{
		output.Log("Drag & Drop Demo constructed");
	}



	//put your interface implementation here, eg

	//void SetSomething(SomeType something) override
	//{
	//	m_something = something;
	//
	//	Notify(true);	//publish state change
	//}

	//SomeType GetSomething() const override
	//{
	//	return m_something;
	//}



	//Data::iStreamable callbacks

	void OnReset(Key32 context) override
	{
	}

	void OnRestore(Data::Archive::View & stream, Key32 context) override
	{
	}

	bool OnImport(UInt16 version, Data::Archive::View & stream, Key32 context) override
	{
		return false;
	}

	void OnStore(Data::Archive & stream) const override
	{
		//if you change/add stored data here, you will need to increment kChunkVersion, as previous chunks will be invalid
		//after changing the kChunkVersion, previous version chunks will be restored by the OnImport callback
	}

};

} }	//end internal namespace

Reflex::TRef <DragDropDemo::App> DragDropDemo::App::Create()
{
	return New<DragDropDemo::AppImpl>();
}

Reflex::Output DragDropDemo::output("Drag & Drop Demo");
