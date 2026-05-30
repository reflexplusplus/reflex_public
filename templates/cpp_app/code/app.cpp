#include "app.h"




//
//_PRODUCT-NAME-SYMBOL_::App implementation

namespace _PRODUCT-NAME-SYMBOL_ { namespace {	//begin internal namespace

using namespace Reflex;

struct AppImpl : public App
{
	static constexpr UInt16 kChunkVersion = 0;

	AppImpl()
		: App(MakeKey32("_PRODUCT-NAME-SYMBOL_"), kChunkVersion)
	{
		output.Log("_PRODUCT-NAME_ constructed");
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

Reflex::TRef <_PRODUCT-NAME-SYMBOL_::App> _PRODUCT-NAME-SYMBOL_::App::Create()
{
	return New<_PRODUCT-NAME-SYMBOL_::AppImpl>();
}

Reflex::Output _PRODUCT-NAME-SYMBOL_::output("_PRODUCT-NAME_");
