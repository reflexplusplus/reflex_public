#pragma once

#include "../../../../include/reflex/data/format/formats/standard.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct RIFF : public SerializableFormat
{
	REFLEX_OBJECT(RIFF, SerializableFormat);

	bool SupportsType(UInt32 tid) const override;

	void OnReset(PropertySet & dynamic) const override;

	DeserializeError OnDeserialize(Archive::View & stream, PropertySet & data, UInt32 options) const override;

	void OnSerialize(Archive & stream, const PropertySet & data) const override;
};

REFLEX_END_INTERNAL
