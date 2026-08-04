#include "../../../../include/reflex/glx/functions/properties.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

template <class TYPE> REFLEX_INLINE const TYPE & ReadPropertyValue(const Data::PropertySet & object, Key32 property_id, const TYPE & fallback)
{
	using PropertyType = ObjectOf<TYPE>;
	
	if (auto exists = object.QueryProperty<PropertyType>(property_id))
	{
		return exists->value;
	}
	else
	{
		return fallback;
	}
}

REFLEX_END_INTERNAL

void Reflex::GLX::UnsetPoint(Data::PropertySet & object, Key32 property_id)
{
	object.UnsetProperty<PointProperty>(property_id);
}

void Reflex::GLX::SetPoint(Data::PropertySet & object, Key32 property_id, Point value)
{
	object.SetProperty(property_id, New<PointProperty>(value));
}

Reflex::GLX::Point Reflex::GLX::GetPoint(const Data::PropertySet & object, Key32 property_id, Point fallback)
{
	return ReadPropertyValue(object, property_id, fallback);
}

Reflex::ConstTRef <Reflex::GLX::PointProperty> Reflex::GLX::GetPointProperty(const Data::PropertySet & object, Key32 property_id)
{
	return GetProperty<PointProperty>(object, property_id);
}

void Reflex::GLX::UnsetSize(Data::PropertySet & object, Key32 property_id)
{
	object.UnsetProperty<SizeProperty>(property_id);
}

void Reflex::GLX::SetSize(Data::PropertySet & object, Key32 property_id, Size value)
{
	object.SetProperty(property_id, New<SizeProperty>(value));
}

Reflex::GLX::Size Reflex::GLX::GetSize(const Data::PropertySet & object, Key32 property_id, Size fallback)
{
	return ReadPropertyValue(object, property_id, fallback);
}

Reflex::ConstTRef <Reflex::GLX::SizeProperty> Reflex::GLX::GetSizeProperty(const Data::PropertySet & object, Key32 property_id)
{
	return GetProperty<SizeProperty>(object, property_id);
}

void Reflex::GLX::UnsetMargin(Data::PropertySet & object, Key32 property_id)
{
	object.UnsetProperty<MarginProperty>(property_id);
}

void Reflex::GLX::SetMargin(Data::PropertySet & object, Key32 property_id, const Margin & value)
{
	object.SetProperty(property_id, New<MarginProperty>(value));
}

Reflex::GLX::Margin Reflex::GLX::GetMargin(const Data::PropertySet & object, Key32 property_id, const Margin & fallback)
{
	return ReadPropertyValue(object, property_id, fallback);
}

Reflex::ConstTRef <Reflex::GLX::MarginProperty> Reflex::GLX::GetMarginProperty(const Data::PropertySet & object, Key32 property_id)
{
	return GetProperty<MarginProperty>(object, property_id);
}

void Reflex::GLX::UnsetColour(Data::PropertySet & object, Key32 property_id)
{
	object.UnsetProperty<ColourProperty>(property_id);
}

void Reflex::GLX::SetColour(Data::PropertySet & object, Key32 property_id, const Colour & value)
{
	object.SetProperty(property_id, New<ColourProperty>(value));
}

Reflex::GLX::Colour Reflex::GLX::GetColour(const Data::PropertySet & object, Key32 property_id, const Colour & fallback)
{
	return ReadPropertyValue(object, property_id, fallback);
}

Reflex::ConstTRef <Reflex::GLX::ColourProperty> Reflex::GLX::GetColourProperty(const Data::PropertySet & object, Key32 property_id)
{
	return GetProperty<ColourProperty>(object, property_id);
}
