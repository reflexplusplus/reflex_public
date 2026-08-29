#pragma once

#include "../format.h"




//
//Primary API

namespace Reflex::Data
{

	void ResetPropertySet(const Format & format, PropertySet & propertyset);


	PropertySet DecodePropertySet(const Format & format, const Archive::View & in, const PropertySet & options = PropertySet::null);

	Archive EncodePropertySet(const Format & format, const PropertySet & in, const PropertySet & options = PropertySet::null);


	PropertySet CopyPropertySet(const Format & format, const PropertySet & from);		//deep copy


	void SerializePropertySet(Archive & stream, const SerializableFormat & format, const PropertySet & in, const PropertySet & options = PropertySet::null);

	void DeserializePropertySet(Archive::View & stream, const SerializableFormat & format, PropertySet & out, const PropertySet & options = PropertySet::null);

}




//
//impl

REFLEX_NS(Reflex::Data)

//error publishing, todo move elsewhere

REFLEX_DECLARE_KEY32(Error);

void ClearError(Object & object);

void SetError(Object & object, UInt line, CString && stage, CString && desc);

Optional <Tuple <UInt,CString::View,CString::View>> GetError(const Object & object);

REFLEX_END

inline void Reflex::Data::Format::Reset(PropertySet & out) const
{
	OnReset(out);
}

inline bool Reflex::Data::Format::Encode(Archive & out, const PropertySet & in, const PropertySet & options) const
{
	REFLEX_ASSERT(CheckTypes(in));

	return OnEncode(out, in, options);
}

inline bool Reflex::Data::Format::Decode(PropertySet & out, const Archive::View & in, const PropertySet & options) const
{
	return OnDecode(out, in, options);
}

inline Reflex::Data::SerializableFormat::DeserializeError Reflex::Data::SerializableFormat::Deserialize(Archive::View & stream, PropertySet & data, const PropertySet & options) const
{
	OnReset(data);

	return OnDeserialize(stream, data, options);
}

inline void Reflex::Data::SerializableFormat::Serialize(Archive & stream, const PropertySet & data, const PropertySet & options) const
{
	REFLEX_ASSERT(CheckTypes(data));

	OnSerialize(stream, data, options);
}

inline void Reflex::Data::ResetPropertySet(const Format & format, PropertySet & propertyset)
{
	format.Reset(propertyset);
}

inline Reflex::Data::Archive Reflex::Data::EncodePropertySet(const Format & format, const PropertySet & data, const PropertySet & options)
{
	Archive archive;

	format.Encode(archive, data, options);

	return archive;
}

inline Reflex::Data::PropertySet Reflex::Data::DecodePropertySet(const Format & format, const Archive::View & in, const PropertySet & options)
{
	PropertySet out;

	format.Decode(out, in, options);

	return out;
}

inline void Reflex::Data::SerializePropertySet(Archive & stream, const SerializableFormat & format, const PropertySet & in, const PropertySet & options)
{
	format.Serialize(stream, in, options);
}

inline void Reflex::Data::DeserializePropertySet(Archive::View & stream, const SerializableFormat & format, PropertySet & out, const PropertySet & options)
{
	format.Deserialize(stream, out, options);
}
