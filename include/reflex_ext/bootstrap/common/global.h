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


	virtual TRef <Object> CreateDeepLinkListener(const Function<void(CString::View)> & callback) = 0;


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
