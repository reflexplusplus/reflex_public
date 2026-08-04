#pragma once

#include "global.h"
#include "app.h"
#include "ui/functions.h"




//
//Secondary API

namespace Reflex::Bootstrap
{

	template <class APP, class ... VARGS> inline TRef <Global> StartApp(System::App::Configuration & config, const CString::View & vendor, const CString::View & product, Key32 resources_subdomain, const char * entry, VARGS && ... vargs);

	template <class APP, class VIEW> inline void PublishAppView(System::App::Configuration & config);

}




//
//impl

REFLEX_NS(Reflex::Bootstrap::Detail)

inline TRef <App> Initialise(App & client)
{
	auto session = client.session;

	if (auto chunk = Data::GetBinary(global->prefs, client.magic))
	{
		session->Deserialize(chunk);
	}
	else
	{
		session->Reset();
	}

	return client;
}

REFLEX_END

template <class APP, class ... VARGS> inline Reflex::TRef <Reflex::Bootstrap::Global> Reflex::Bootstrap::StartApp(System::App::Configuration & config, const CString::View & vendor, const CString::View & product, Key32 resources_subdomain, const char * entry, VARGS && ... vargs)
{
	auto global = Global::Acquire(vendor, product, Detail::ExtractProjectDir(entry), resources_subdomain);

	if constexpr (sizeof...(VARGS) == 0)
	{
		config.instance_ctr = [](Object & global, System::App & instance)
		{
			return Detail::Initialise(APP::Create());
		};
	}
	else if constexpr (sizeof...(VARGS) == 1)
	{
		config.instance_ctr = [args = MakeTuple(std::forward<VARGS>(vargs)...)](Object & global, System::App & instance)
		{
			return Detail::Initialise(APP::Create(args.a));
		};
	}
	else if constexpr (sizeof...(VARGS) == 2)
	{
		config.instance_ctr = [args = MakeTuple(std::forward<VARGS>(vargs)...)](Object & global, System::App & instance)
		{
			return Detail::Initialise(APP::Create(args.a, args.b));
		};
	}

	return global;
}

template <class APP, class VIEW> inline void Reflex::Bootstrap::PublishAppView(System::App::Configuration & config)
{
	Detail::PublishAppView(config, [](Object & instance) -> TRef <GLX::Object>
	{
		if (auto p = DynamicCast<APP>(instance))
		{
			return New<VIEW>(*p);
		}
		else
		{
			return New<GLX::Object>();
		}
	});
}
