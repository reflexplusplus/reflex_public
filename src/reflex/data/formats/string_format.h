#pragma once

#include "../../../../include/reflex/data/format/formats/standard.h"
#include "../tokeniser.h"




//
//

REFLEX_NS(Reflex::Data)

struct StringFormat : public Format
{
	void OnReset(PropertySet & node) const override
	{
		UnsetAll<CStringProperty>(node);
	}

	bool SupportsType(TypeID type_id) const override
	{
		return type_id == REFLEX_TYPEID(CStringProperty);
	}

	static void Throw(UInt line = 0, CString::View desc = "parse error")
	{
		throw(MakeTuple(line, CString(desc)));
	}
};

REFLEX_END
