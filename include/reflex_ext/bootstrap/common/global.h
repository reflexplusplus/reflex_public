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

	[[nodiscard]] static TRef <Global> Acquire(const CString::View & vendor, const CString::View & product, const WString::View & projectdir, Key32 resource_group = kNullKey);


	virtual TRef <Object> CreateDeepLinkListener(const Function<void(CString::View)> & callback) = 0;


	virtual TRef <Object> EnableIde(bool enable) = 0;

	virtual bool IdeEnabled() const = 0;

	
	virtual void CommitPreferences() = 0;



	const TRef <File::VirtualFileSystem> filesystem;		//includes resources locator for :res/

	const TRef <File::ResourcePool> resourcepool;

	const TRef <File::PersistentPropertySet> prefs;

	const CString vendor, product;



protected:

	Global(const CString::View & vendor, const CString::View & product, const WString::View & projectdir, Key32 resources_subdomain);

};
