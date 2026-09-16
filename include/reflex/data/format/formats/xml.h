#pragma once

#include "../format.h"
#include "../../functions/properties.h"




//
//Primary API

namespace Reflex::Data
{

	AlreadyRetained <PropertySetArray> AcquireXmlNodes(PropertySet & node);

	AlreadyRetained <PropertySet> AddXmlNode(PropertySetArray & nodes, CString::View tag);

	ArrayView < ConstReference <PropertySet> > GetXmlNodes(const PropertySet & node);

	CString::View GetXmlTag(const PropertySet & node);


	extern const ConstAlreadyRetained <Format> kReflexXmlFormat;

	extern const ConstAlreadyRetained <Format> kReflexMarkupFormat;

}




//
//impl

REFLEX_NS(Reflex::Data)
constexpr Key32 kid = "id";
constexpr Key32 ktag = "tag";
REFLEX_END

inline Reflex::AlreadyRetained <Reflex::Data::PropertySetArray> Reflex::Data::AcquireXmlNodes(PropertySet & node)
{
	return AcquirePropertySetArray(node, kNullKey);
}

inline Reflex::AlreadyRetained <Reflex::Data::PropertySet> Reflex::Data::AddXmlNode(PropertySetArray & node, CString::View tag)
{
	auto child = AddPropertySet(node);

	Data::SetCString(child, ktag, tag);

	return child;
}

inline Reflex::ArrayView < Reflex::ConstReference <Reflex::Data::PropertySet> > Reflex::Data::GetXmlNodes(const PropertySet & node)
{
	return GetPropertySetArray(node, kNullKey);
}

inline Reflex::CString::View Reflex::Data::GetXmlTag(const PropertySet & node)
{
	return GetCString(node, ktag);
}
