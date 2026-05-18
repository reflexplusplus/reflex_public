#pragma once

#include "../../types.h"
#include "../../propertyset.h"




//
//Primary API

namespace Reflex::Data
{

	REFLEX_DECLARE_KEY32(keymap);

	TRef <KeyMap> AcquireKeyMap(PropertySet & root);

	Key32 RegisterKey(KeyMap & keymap, const CString::View & string);

	ConstTRef <KeyMap> GetKeyMap(const PropertySet & root);

	CString::View GetKey(const KeyMap & keymap, Key32 key);

	void Assimilate(KeyMap & keymap, const KeyMap & b);

}
