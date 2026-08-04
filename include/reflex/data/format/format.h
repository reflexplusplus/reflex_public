#pragma once

#include "../propertyset.h"
#include "../types.h"




//
//Secondary API

namespace Reflex::Data
{

	class Format;

}




//
//Format

class Reflex::Data::Format : public Object
{
public:

	REFLEX_OBJECT(Data::Format, Object);

	static Format & null;


	virtual bool SupportsType(TypeID type_id) const = 0;


	void Reset(PropertySet & data) const;

	bool Decode(PropertySet & out, const Archive::View & in, UInt32 options = 0) const;

	bool Encode(Archive & out, const PropertySet & in) const;



protected:

	virtual void OnReset(PropertySet & data) const = 0;

	virtual bool OnDecode(PropertySet & out, const Archive::View & in, UInt32 options) const = 0;

	virtual bool OnEncode(Archive & out, const PropertySet & in, UInt32 options) const = 0;


	REFLEX_IF_DEBUG(bool CheckTypes(const PropertySet & data) const;)

};
