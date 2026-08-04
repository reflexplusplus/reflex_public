#include "placeholder.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

NullResourceGroup g_null_resource_group;

REFLEX_END_INTERNAL

Reflex::IDE::ResourceGroup & Reflex::IDE::ResourceGroup::null = Reflex::IDE::g_null_resource_group;

const Reflex::WString::View Reflex::IDE::kC = L"c";
const Reflex::WString::View Reflex::IDE::kH = L"h";
const Reflex::WString::View Reflex::IDE::kGLX = L"glx";
const Reflex::WString::View Reflex::IDE::kTXT = L"txt";
const Reflex::WString::View Reflex::IDE::kJSON = L"json";
const Reflex::WString::View Reflex::IDE::kXML = L"xml";

Reflex::IDE::ProxyPath::ProxyPath(Key32 domain, Key32 sub_domain, const WString::View & local_path)
	: Locator(domain, { 0, kMaxUInt32 }),
	sub_domain(sub_domain),
	local_path(local_path)
{
}
