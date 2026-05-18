#pragma once

#include "defines.h"




//
//De

REFLEX_NS(Reflex::GLX::Detail)

class ImageSet;


ConstTRef <ImageSet> CreateImageSet(const Data::PropertySet & desc);

ConstTRef <ImageSet> CreateImageSetFromSVG(const Data::PropertySet & desc);


void SetImage(GLX::Object & object, Key32 id, ConstTRef <System::Renderer::Canvas> bitmap);

void SetImage(GLX::Object & object, Key32 id, ConstTRef <Graphic> graphic, Size content_size);

void ClearImage(GLX::Object & object, Key32 id);

REFLEX_END




//
//Detail::ImageSet

class Reflex::GLX::Detail::ImageSet : public Reflex::Object
{
public:

	REFLEX_OBJECT(GLX::Detail::ImageSet, Reflex::Object);

	static ImageSet & null;

	using Frame = Tuple < Key32, ConstReference <System::Renderer::Graphic>, Size >;



	//lifetime

	ImageSet(ConstTRef <System::Renderer::Canvas> source_bitmap);

	~ImageSet();



	//content

	void AddFrame(Key32 id, ConstTRef <Graphic> graphic, Size size);

	void AddFrame(Key32 id, const Rect & rect);

	ArrayView <Frame> GetFrames() const { return m_frames; }



	//info

	const ConstTRef <System::Renderer::Canvas> source_bitmap;



private:

	Array <Frame> m_frames;

};
