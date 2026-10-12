#include "app.h"

namespace Widgets { namespace {

using namespace Reflex;

struct AppImpl : public App
{
	AppImpl() : App(MakeKey32("Widgets"), 0) {}

	void OnReset(Key32) override {}
	void OnRestore(Data::Archive::View &, Key32) override {}
	bool OnImport(UInt16, Data::Archive::View &, Key32) override { return false; }
	void OnStore(Data::Archive &) const override {}
};

} }

Reflex::Unretained <Widgets::App> Widgets::App::Create()
{
	return New<AppImpl>();
}

Reflex::Output Widgets::output("Widgets");
