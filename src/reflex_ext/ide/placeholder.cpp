#include "placeholder.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

class PlaceholderProxyPath : public ProxyPath
{
	using ProxyPath::ProxyPath;
};

bool g_is_awake = false;

REFLEX_END_INTERNAL

const bool & Reflex::IDE::kIsAwake = g_is_awake;

Reflex::TRef <Reflex::Object> Reflex::IDE::Start(File::ResourcePool & resourcepool, const WString::View & logfile, Data::PropertySet & prefs)
{
	return {};
}

Reflex::TRef <Reflex::IDE::ResourceGroup> Reflex::IDE::ResourceGroup::Create(File::ResourcePool & resourcepool, Key32 uid, const WString::View & desc, const Function <void(ResourceGroup&)> & onreload)
{
	return REFLEX_CREATE(PlaceholderResourceGroup);
}

Reflex::TRef <Reflex::IDE::ProxyPath> Reflex::IDE::ProxyPath::Create(Key32 domain, Key32 subdomain, const WString::View & localpath)
{
	return REFLEX_CREATE(PlaceholderProxyPath, domain, subdomain, localpath);
}
