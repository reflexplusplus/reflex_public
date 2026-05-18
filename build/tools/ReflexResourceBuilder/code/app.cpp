#include "app.h"
#include "parser.h"




//
//App impl

REFLEX_BEGIN_INTERNAL(ResourceBuilder)

struct AppImpl : public App
{
	AppImpl()
		: App(K32("ResourceBuilder"))	//generates 4 byte header for the file format
	{
	}

	TRef <System::Thread> Compile(const WString & path, ObjectOf <Float> & progress) override
	{
		m_thread = System::Thread::Create([path, progressref = AutoRelease(progress)]()
		{
			ResourceBuilder::Compile(path, progressref->value);
		});

		return m_thread;
	}


	Reference <System::Thread> m_thread;
};

REFLEX_END_INTERNAL

TRef <ResourceBuilder::App> ResourceBuilder::App::Create()
{
	return New<ResourceBuilder::AppImpl>();
}

Reflex::Output ResourceBuilder::output("ReflexResourceBuilder");
