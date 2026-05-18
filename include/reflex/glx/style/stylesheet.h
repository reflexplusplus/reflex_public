#pragma once

#include "style.h"




//
//Primary API

namespace Reflex::GLX
{

	class StyleSheet;

}




//
//StyleSheet

class Reflex::GLX::StyleSheet : public Style
{
public:

	REFLEX_OBJECT(GLX::StyleSheet,Style);

	static StyleSheet & null;



	//lifetime

	StyleSheet(Key32 id, Float32 scale = 1.0f);



	//overrides

	virtual const Style * QuerySubStyle(Key32 key, const Style * fallback) const override;



	//includes

	ConstTRef <StyleSheet> AddInclude(const CString & path, bool import);

	ArrayView < ConstReference <StyleSheet> > GetIncludes(bool imports) const;


	
	//info

	const Key32 path;

	const Float kScale;



protected:

	virtual void OnSetProperty(Address address, Reflex::Object & object) override;



private:

	Array < ConstReference <StyleSheet> > m_includes_imports[2];

};

template <> struct Reflex::SubIndexType <Reflex::GLX::StyleSheet> { using Type = Key32; };

REFLEX_SET_TRAIT(Reflex::GLX::StyleSheet, IsSingleThreadExclusive)




//
//impl

REFLEX_INLINE Reflex::ArrayView < Reflex::ConstReference <Reflex::GLX::StyleSheet> > Reflex::GLX::StyleSheet::GetIncludes(bool imports) const
{
	return m_includes_imports[imports];
}
