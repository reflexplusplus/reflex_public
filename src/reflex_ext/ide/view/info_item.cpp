#include "info_item.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Object> Reflex::IDE::Detail::CreateInfoItem(const WString & key, const WString::View & value, bool path)
{
	return REFLEX_CREATE(InfoItem, key, value, path);
}
