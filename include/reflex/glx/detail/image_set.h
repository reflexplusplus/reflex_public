#pragma once

#include "defines.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

class ImageSet;


Unretained <const ImageSet> CreateImageSet(const Data::PropertySet & desc);

Unretained <const ImageSet> CreateImageSetFromSVG(const Data::PropertySet & desc);


void SetImage(GLX::Object & object, Key32 id, ConstWillRetain <System::Renderer::Canvas> bitmap);

void SetImage(GLX::Object & object, Key32 id, ConstWillRetain <Graphic> graphic, Size content_size);

void UnsetImage(GLX::Object & object, Key32 id);

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

	ImageSet(ConstWillRetain <System::Renderer::Canvas> source_bitmap);

	~ImageSet();



	//content

	void AddFrame(Key32 id, ConstWillRetain <Graphic> graphic, Size size);

	void AddFrame(Key32 id, const Rect & rect);

	ArrayView <Frame> GetFrames() const { return m_frames; }



	//info

	const ConstAlreadyRetained <System::Renderer::Canvas> source_bitmap;



private:

	Array <Frame> m_frames;

};




REFLEX_NS(Reflex::GLX::Detail)

inline void ClearImage(GLX::Object & object, Key32 id) { UnsetImage(object, id); }

REFLEX_END
