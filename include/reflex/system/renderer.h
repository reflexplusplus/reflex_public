#pragma once

#include "types.h"




//
//Secondary API

namespace Reflex::System
{

	class Renderer;

}




//
//Renderer

class Reflex::System::Renderer : public Object
{
public:

	REFLEX_OBJECT(System::Renderer, Object);

	static Renderer & null;

	REFLEX_DECLARE_KEY32(HD);	//HD, bool
	REFLEX_DECLARE_KEY32(TX);	//render to texture, bool



	//types

	enum PrimitiveType : UInt8
	{
		kPrimitiveTypeLines,
		kPrimitiveTypeLineStrip,
		kPrimitiveTypeTriangles,
		kPrimitiveTypeTriangleStrip,
		kPrimitiveTypePoints,

		kNumPrimitiveType,
	};

	enum FilterMode : UInt8
	{
		kFilterModeNone,

		kFilterModePixelate,	//2 parameters: block size in bitmap-space pixels.
		kFilterModeBlur,		//3 to 32 parameters: step.xy in bitmap-space pixels ({1.0f, 0.0f} for horizontal, {0.0f, 1.0f} for vertical), the radius, then the gaussian weights (_radius+1_ entries).

		kNumFilterMode,
	};

	struct Transform;

	class Canvas;

	class Graphic;

	using Config = Map <Key32,UInt32>;

	using EngineCtr = FunctionPointer <TRef <Renderer>(const Config & config)>;



	//lifetime

	static Array <Pair <CString::View, EngineCtr> > GetEngines();



	//info

	virtual CString::View GetEngineName() const = 0;

	virtual bool Status() const = 0;

	virtual Config GetConfig() const = 0;

	virtual bool SupportsImageFormat(ImageFormat format) const = 0;



	//access (must set before use)

	virtual void BeginAccess() = 0;

	virtual void EndAccess() = 0;



	//Canvas (render targets)

	[[nodiscard]] virtual TRef <Canvas> CreateCanvas(void * systemwindow) = 0;

	[[nodiscard]] virtual TRef <Canvas> CreateBitmap(bool alphachannel, bool antialias) = 0;



	//Graphic

	[[nodiscard]] virtual TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <fPoint> & points) = 0;

	[[nodiscard]] virtual TRef <Graphic> CreatePrimitives(PrimitiveType primitive, const ArrayView <ColourPoint> & points) = 0;



	//render (a valid Canvas must be set as current)

	virtual void Clear(const Colour & color) = 0;

	virtual void EnableBlend(bool enable) = 0;

	virtual void SetDitheringAmount(Float amount) = 0;

	virtual void SetColourTransform(const ArrayView <Float> & m) = 0;

	virtual void SetClip(const iRect & rect) = 0;

	virtual void SetMask(const Graphic & mask, const Transform & transform, bool invert) = 0;

	virtual void ClearMask() = 0;



	//info

	virtual Canvas * GetCurrent() = 0;

};

REFLEX_SET_TRAIT(Reflex::System::Renderer, IsSingleThreadExclusive)




//
//Renderer::Transform

struct Reflex::System::Renderer::Transform
{
	fPoint origin;

	fSize scale = { 1.0f, 1.0f };

	Float32 rotation = 0.0f;

	Float32 opacity = 1.0f;
};




//
//Renderer::Canvas

REFLEX_SET_TRAIT(Reflex::System::Renderer::Canvas, IsSingleThreadExclusive)

class Reflex::System::Renderer::Canvas : public Object
{
public:

	REFLEX_OBJECT(System::Renderer::Canvas, Object);

	static Canvas & null;

	static constexpr iSize kMinValidSize = { 1, 1 };	//clients responsibility to ensure size > 0



	//size

	virtual bool SetSize(const iSize & size, Int32 dpifactor) = 0;

	virtual const iSize & GetSize() const = 0;

	virtual Int32 GetPixelDensity() const = 0;



	//render target (for render to window or bitmap)

	virtual void SetCurrent() = 0;

	virtual void Flush() = 0;



	//write (does nothing on window canvases)

	virtual void Write(ImageFormat format, const ArrayView <UInt8> & data) = 0;



	//create texture

	virtual TRef <Graphic> CreateTextures(const ArrayView < Pair <fRect> >& rects) const = 0;

	virtual TRef <Graphic> CreateTexturesWithFilter(const ArrayView < Pair <fRect> >& rects, FilterMode mode, const ArrayView <Float>& parameters) const = 0;
};




//
//Renderer::Graphic

REFLEX_SET_TRAIT(Reflex::System::Renderer::Graphic, IsSingleThreadExclusive)

class Reflex::System::Renderer::Graphic : public Object
{
public:

	REFLEX_OBJECT(System::Renderer::Graphic,Object);

	static Graphic & null;



	//render

	virtual void Render(const Transform & transform, const Colour & colour) const = 0;

};




//
//Detail

REFLEX_NS(Reflex::System::Detail)

bool InstantiateDesktopOpenGL();	//call this on win/mac if linking to optional OpenGL module

REFLEX_END
