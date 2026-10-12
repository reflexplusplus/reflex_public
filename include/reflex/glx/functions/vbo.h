#pragma once

#include "drawing.h"




//
//Primary API

namespace Reflex::GLX
{

	[[nodiscard]] Unretained <Graphic> CreateGraphic(const Points::View & points, System::Renderer::PrimitiveType primitive_type = System::Renderer::kPrimitiveTypeTriangles);

	[[nodiscard]] Unretained <Graphic> CreateGraphic(const ColourPoints::View & colour_points, System::Renderer::PrimitiveType primitive_type = System::Renderer::kPrimitiveTypeTriangles);


	[[nodiscard]] Unretained <Graphic> CreateCompositeGraphic(ArrayView < WillRetain <Graphic> > graphics);

	[[nodiscard]] Unretained <Graphic> CreateCompositeGraphic(const Colour & colour, ArrayView < WillRetain <Graphic> > graphics);

	[[nodiscard]] Unretained <Graphic> CreateCompositeGraphic(ArrayView < Pair < WillRetain <Graphic>, Colour > > graphics);

}




//
//impl

inline Reflex::Unretained <Reflex::GLX::Graphic> Reflex::GLX::CreateGraphic(const Points::View & points, System::Renderer::PrimitiveType type)
{
	return Core::g_renderer->CreatePrimitives(type, points);
}

inline Reflex::Unretained <Reflex::GLX::Graphic> Reflex::GLX::CreateGraphic(const ColourPoints::View & colourpoints, System::Renderer::PrimitiveType type)
{
	return Core::g_renderer->CreatePrimitives(type, colourpoints);
}
