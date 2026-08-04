#pragma once

#include "functions.h"




//
//declarations

REFLEX_NS(Reflex::System::Common)

struct CanvasBase;

REFLEX_END




//
//canvasbase

struct Reflex::System::Common::CanvasBase : public Renderer::Canvas
{
public:

	virtual bool SetSize(const iSize & size, Int32 pixdensity) override;

	virtual const iSize & GetSize() const override;

	virtual Int32 GetPixelDensity() const override;

	virtual void Write(ImageFormat format, const ArrayView <UInt8> & data) override {}

	virtual void Flush() override {}

	virtual TRef <Renderer::Graphic> CreateTextures(const ArrayView < Pair <fRect> >& rects) const override { return Renderer::Graphic::null; }

	virtual TRef <Renderer::Graphic> CreateTexturesWithFilter(const ArrayView < Pair <fRect> > & rects, Renderer::FilterMode mode, const ArrayView <Float>& parameters) const override { return Renderer::Graphic::null; }


	Int32 m_pixeldens = 1;

	UInt32 m_internaltype = 0;	//optimisation for engine

	iSize m_size = { 1,1 };

	fSize m_pixel_size = { 1.0f, 1.0f };

};




//
//

inline bool Reflex::System::Common::CanvasBase::SetSize(const iSize & size, Int32 pixdensity)
{
	REFLEX_ASSERT_EX(size.w && size.h, "System::Canvas::SetSize");

	m_size = size;

	m_pixeldens = pixdensity;

	m_pixel_size.w = 1.0f / Float32(pixdensity);

	m_pixel_size.h = m_pixel_size.w;

	return true;
}

inline const Reflex::System::iSize & Reflex::System::Common::CanvasBase::GetSize() const
{
	return m_size;
}

inline Reflex::Int32 Reflex::System::Common::CanvasBase::GetPixelDensity() const
{
	return m_pixeldens;
}
