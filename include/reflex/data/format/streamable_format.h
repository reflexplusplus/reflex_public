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


	void Deserialize(Archive::View & stream, PropertySet & data) const;

	void Serialize(Archive & stream, const PropertySet & data) const;



protected:

	virtual void OnDeserialize(Archive::View & stream, PropertySet & data, UInt32 options) const = 0;

	virtual void OnSerialize(Archive & stream, const PropertySet & data) const = 0;



private:

	virtual bool OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const override final;

	virtual bool OnEncode(Archive & out, const PropertySet & in, UInt32 options) const override final;

};
