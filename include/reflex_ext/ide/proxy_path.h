#pragma once

#include "[require].h"




//
//Primary API

namespace Reflex::IDE
{

	class ProxyPath;

}




//
//IDE::ProxyPath 

class Reflex::IDE::ProxyPath : public File::VirtualFileSystem::Locator
{
public:

	REFLEX_OBJECT(IDE::ProxyPath, File::VirtualFileSystem::Locator);


	
	//lifetime

	[[nodiscard]] static TRef <ProxyPath> Create(Key32 domain, Key32 sub_domain, const WString::View & local_path);


	
	//info
	
	const Key32 sub_domain;

	const WString local_path;



protected:

	ProxyPath(Key32 domain, Key32 sub_domain, const WString::View & local_path);

};

REFLEX_SET_TRAIT(Reflex::IDE::ProxyPath, IsAbstract);
