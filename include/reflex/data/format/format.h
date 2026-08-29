#pragma once

#include "../propertyset.h"
#include "../types.h"




//
//Secondary API

namespace Reflex::Data
{

	class Format;		//generic interface for structured data storage format

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

	bool Decode(PropertySet & out, const Archive::View & in, const PropertySet & options = PropertySet::null) const;

	bool Encode(Archive & out, const PropertySet & in, const PropertySet & options = PropertySet::null) const;



protected:

	virtual void OnReset(PropertySet & data) const = 0;

	virtual bool OnDecode(PropertySet & out, const Archive::View & in, const PropertySet & options) const = 0;

	virtual bool OnEncode(Archive & out, const PropertySet & in, const PropertySet & options) const = 0;


	REFLEX_IF_DEBUG(bool CheckTypes(const PropertySet & data) const;)

};
