#pragma once

#include "detail.h"




//
//Primary API

namespace Reflex::Bootstrap
{

	class Global;

	
	extern Reflex::Detail::Module module;

	extern const TRef <Global> global;

}




//
//Global

class Reflex::Bootstrap::Global : public Data::PropertySet
{
public:

	REFLEX_OBJECT(Bootstrap::Global, Data::PropertySet);


	[[nodiscard]] static TRef <Global> Acquire(CString::View vendor, CString::View product, WString::View project_dir, Key32 resource_group = kNullKey);


	[[nodiscard]] virtual TRef <Object> CreateDeepLinkListener(const Function<void(CString::View)> & callback) = 0;


	virtual TRef <Object> EnableIde(bool enable) = 0;

	virtual bool IdeEnabled() const = 0;

	
	virtual void CommitPreferences() = 0;



	const TRef <File::VirtualFileSystem> filesystem;		//includes resources locator for :res/

	const TRef <File::ResourcePool> resourcepool;

	const TRef <File::PersistentPropertySet> prefs;

	const CString vendor, product;



protected:

	Global(CString::View vendor, CString::View product, WString::View project_dir, Key32 resources_subdomain);

};




//
//Secondary API

#define REFLEX_BOOTSTRAP_NULL_INSTANCE_EX(MODULE, NS, TYPE, IMPL, VAR, ORDER) namespace NS { Reflex::Detail::Module::Member <IMPL> VAR(MODULE, ORDER); } NS::TYPE & NS::TYPE::null = NS::VAR
#define REFLEX_BOOTSTRAP_NULL_INSTANCE(NS, TYPE) REFLEX_BOOTSTRAP_NULL_INSTANCE_EX(Reflex::Bootstrap::module, NS, TYPE, TYPE, g##_null_##TYPE, 0)
