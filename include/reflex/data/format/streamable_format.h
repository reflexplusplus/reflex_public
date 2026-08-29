#pragma once

#include "format.h"




//
//Secondary API

namespace Reflex::Data
{

	class SerializableFormat;

}




//
//SerializableFormat

class Reflex::Data::SerializableFormat : public Format
{
public:

	REFLEX_OBJECT(Data::SerializableFormat, Format);

	static SerializableFormat & null;


	enum DeserializeError
	{
		kDeserializeErrorNone,
		kDeserializeErrorInvalidHeader,
		kDeserializeErrorUnsupportedVersion,
		kDeserializeErrorInvalidStream,
		kDeserializeErrorUnknownType,
	};

	DeserializeError Deserialize(Archive::View & stream, PropertySet & data, const PropertySet & options = PropertySet::null) const;

	void Serialize(Archive & stream, const PropertySet & data, const PropertySet & options = PropertySet::null) const;



protected:

	virtual DeserializeError OnDeserialize(Archive::View & stream, PropertySet & data, const PropertySet & options) const = 0;

	virtual void OnSerialize(Archive & stream, const PropertySet & data, const PropertySet & options) const = 0;



	virtual bool OnDecode(PropertySet & out, const Archive::View & in, const PropertySet & options) const override;

	virtual bool OnEncode(Archive & out, const PropertySet & in, const PropertySet & options) const override;

};
