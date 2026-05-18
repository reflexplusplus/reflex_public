#pragma once

#include "../defines.h"




//
//Primary API

namespace Reflex::GLX
{

	void UnsetPoint(Data::PropertySet & object, Key32 property_id);

	void SetPoint(Data::PropertySet & object, Key32 property_id, Point value);

	Point GetPoint(const Data::PropertySet & object, Key32 property_id, Point fallback = {});

	ConstTRef <PointProperty> GetPointProperty(const Data::PropertySet & object, Key32 property_id);


	void UnsetSize(Data::PropertySet & object, Key32 property_id);

	void SetSize(Data::PropertySet & object, Key32 property_id, Size value);

	Size GetSize(const Data::PropertySet & object, Key32 property_id, Size fallback = {});

	ConstTRef <SizeProperty> GetSizeProperty(const Data::PropertySet & object, Key32 property_id);


	void UnsetMargin(Data::PropertySet & object, Key32 property_id);

	void SetMargin(Data::PropertySet & object, Key32 property_id, const Margin & value);

	Margin GetMargin(const Data::PropertySet & object, Key32 property_id, const Margin & fallback = {});

	ConstTRef <MarginProperty> GetMarginProperty(const Data::PropertySet & object, Key32 property_id);


	void UnsetColour(Data::PropertySet & object, Key32 property_id);

	void SetColour(Data::PropertySet & object, Key32 property_id, const Colour & value);

	Colour GetColour(const Data::PropertySet & object, Key32 property_id, const Colour & fallback = kWhite);

	ConstTRef <ColourProperty> GetColourProperty(const Data::PropertySet & object, Key32 property_id);


	void UnsetColor(Data::PropertySet & object, Key32 property_id);

	void SetColor(Data::PropertySet & object, Key32 property_id, const Colour & value);

	Colour GetColor(const Data::PropertySet & object, Key32 property_id, const Colour & fallback = kWhite);

	ConstTRef <ColourProperty> GetColorProperty(const Data::PropertySet & object, Key32 property_id);

}




//
//impl

inline void Reflex::GLX::UnsetColor(Data::PropertySet & object, Key32 property_id)
{
	UnsetColour(object, property_id);
}

inline void Reflex::GLX::SetColor(Data::PropertySet & object, Key32 property_id, const Colour & value)
{
	SetColour(object, property_id, value);
}

inline Reflex::GLX::Colour Reflex::GLX::GetColor(const Data::PropertySet & object, Key32 property_id, const Colour & fallback)
{
	return GetColour(object, property_id, fallback);
}

inline Reflex::ConstTRef <Reflex::GLX::ColourProperty> Reflex::GLX::GetColorProperty(const Data::PropertySet & object, Key32 property_id)
{
	return GetColourProperty(object, property_id);
}
