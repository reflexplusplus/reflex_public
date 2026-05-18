#pragma once

#include "../object.h"
#include "../functions/drawing.h"




//
//Primary API

namespace Reflex::GLX
{

	struct CanvasProperties
	{
		Size size;
		Size pixel_size;
	};

	struct CanvasContext : CanvasProperties
	{
		Points & output;
	};

	struct ColourCanvasContext : CanvasProperties
	{
		ColourPoints & output;
	};

	struct GraphicCanvasContext : CanvasProperties
	{
		TRef <Graphic> output;
	};


	void SetCanvas(Object & object, Key32 id, const Function <void(CanvasContext & ctx)> & ondraw);

	void SetColourCanvas(Object & object, Key32 id, const Function <void(ColourCanvasContext & ctx)> & ondraw);

	void SetGraphicCanvas(Object & object, Key32 id, const Function <void(GraphicCanvasContext & ctx)> & ondraw);

	void UnsetCanvas(Object & object, Key32 id);


	using ColorCanvasContext = ColourCanvasContext;

	void SetColorCanvas(Object & object, Key32 id, const Function <void(ColorCanvasContext & ctx)> & ondraw);

}




//
//impl

inline void Reflex::GLX::SetColorCanvas(Object & object, Key32 id, const Function <void(ColorCanvasContext & ctx)> & ondraw)
{
	SetColourCanvas(object, id, ondraw);
}