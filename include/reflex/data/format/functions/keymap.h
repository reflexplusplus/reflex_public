#pragma once

#include "../../types.h"
#include "../../propertyset.h"




//
//Primary API

namespace Reflex::Data
{

	REFLEX_DECLARE_KEY32(keymap);

	AlreadyRetained <KeyMap> AcquireKeyMap(PropertySet & root);

	Key32 RegisterKey(KeyMap & keymap, const CString::View & string);

	ConstAlreadyRetained <KeyMap> GetKeyMap(const PropertySet & root);

	CString::View GetKey(const KeyMap & keymap, Key32 key);

	void Assimilate(KeyMap & keymap, const KeyMap & b);

}
