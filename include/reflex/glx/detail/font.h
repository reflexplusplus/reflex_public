#pragma once

#include "countable.h"
#include "../functions.h"




//
//Detail

namespace Reflex::GLX::Detail
{

	class FontFile;

	class Font;


	TRef <Reflex::Object> DecodeFontFile(const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream);

	ConstTRef <Data::ArchiveObject> RetrieveFontFile(const WString::View & path);

}




//
//Detail::FontFile

class Reflex::GLX::Detail::FontFile : public Data::ArchiveObject
{
public:

	REFLEX_OBJECT(GLX::Detail::FontFile, Data::ArchiveObject);

	using Data::ArchiveObject::ArchiveObject;
};

REFLEX_SET_TRAIT(Reflex::GLX::Detail::FontFile, IsSingleThreadExclusive)




//
//Detail::Font

class Reflex::GLX::Detail::Font :
	public Reflex::Object,
	public Countable <MakeKey32("Font")>
{
public:

	REFLEX_OBJECT(GLX::Detail::Font, Object);

	static Font & null;


	enum RenderMode : UInt8
	{
		kRenderDefault,	//FT_NORMAL
		kRenderSmooth,	//FT_LIGHT
		kRenderCrisp,	//FT_MONO
		kRenderLCD_H,
		kRenderLCD_V,

		kNumRenderMode
	};

	struct FaceDesc
	{
		ConstTRef <Data::ArchiveObject> fontfile;

		Size size;
		Float yoffset = 0.0f;	//todo Point
		Float spacing = 0.0f;
		Font::RenderMode mode = kRenderDefault;
		bool antialias = false;
	};



	//lifetime (cached/shared)

	static TRef <Font> Create(const Data::PropertySet & parameters);

	static TRef <Font> Create(const ArrayView <FaceDesc> & faces);	



	//info

	virtual Pair <Float32> GetHeightAndTail() const = 0;	//TODO this should depend on string

	virtual Float32 GetTextWidth(const WString::View & text) const = 0;



	//text

	virtual Pair < TRef <System::Renderer::Graphic>,Float32 > CreateText(const WString::View & text, Point position = kOrigin, Size scale = kNormal) const = 0;

};

REFLEX_SET_TRAIT(Reflex::GLX::Detail::Font, IsSingleThreadExclusive)




//
//impl

inline Reflex::ConstTRef <Reflex::Data::ArchiveObject> Reflex::GLX::Detail::RetrieveFontFile(const WString::View & path)
{
	return RetrieveRelativeResource<FontFile>(path, Data::PropertySet::null, &DecodeFontFile);
}
