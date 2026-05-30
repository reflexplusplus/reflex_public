#include "app.h"




//
//CustomDrawing::App implementation

namespace CustomDrawing { namespace {

struct AppImpl : public App
{
	static constexpr UInt16 kChunkVersion = 0;

	AppImpl()
		: App(MakeKey32("CustomDrawing"), kChunkVersion)
	{
		output.Log("Custom Drawing constructed");
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

} }

Reflex::TRef <CustomDrawing::App> CustomDrawing::App::Create()
{
	return New<CustomDrawing::AppImpl>();
}

Reflex::Output CustomDrawing::output("Custom Drawing");
