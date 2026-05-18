#pragma once

#include "../warnings.h"
#include "../library.h"
#include "../defines.h"
#include "../detail/countable.h"




//
//Primary API

namespace Reflex::GLX
{

	class Style;

}




//
//Style

class Reflex::GLX::Style :
	public Node <Style,Data::PropertySet>,
	public Detail::Countable <MakeKey32("Style")>
{
public:

	REFLEX_OBJECT(GLX::Style, Data::PropertySet);

	static Style & null;



	//lifetime

	[[nodiscard]] static TRef <Style> Create(Key32 id, UInt16 stylesheet_flags = 0);



	//location

	using Node::Clear;

	using Node::Attach;

	using Node::Detach;

	using Node::InsertBefore;

	using Node::InsertAfter;



	//children

	virtual TRef <Style> AddSubStyle(Key32 type, Key32 id, bool is_stub);

	virtual const Style * QuerySubStyle(Key32 id, const Style * fallback = nullptr) const;		//allows "virtualised" brnach override (use Detail::GetChildStyle to locate only real children)

	ConstTRef <Style> operator[](Key32 id) const;

	REFLEX_IF_DEBUG(ConstTRef <Style> operator[](const char * id) const);



	//info

	const Key32 id;

	const UInt16 stylesheet_flags;	//currently css margin = 1



protected:

	Style(Key32 id, UInt16 stylesheet_flags);


	virtual void OnSetProperty(Address address, Reflex::Object & object) override;

	virtual void OnQueryProperty(Address address, Reflex::Object * & object) const override;

	virtual void OnReleaseData() override;


	UInt8 m_is_root_style = true;	//not State

	UInt8 m_non_virtual = true;

	Key32 m_states[4];

};

template <> struct Reflex::SubIndexType <Reflex::GLX::Style> { using Type = Key32; };

REFLEX_SET_TRAIT(Reflex::GLX::Style, IsSingleThreadExclusive)
REFLEX_SET_TRAIT(Reflex::GLX::Style, IsAbstract);




//
//impl

REFLEX_NS(Reflex::GLX::Detail)
constexpr Key32 kComputedStyle = "ComputedStyle";	//special key -> used by Detail::Compile, will not be 'virtualised' by Style::State
REFLEX_END

REFLEX_INLINE Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::Style::operator[](Key32 key) const
{
	auto child = QuerySubStyle(key, &Reflex::Detail::GetNullInstance<Style>());

	REFLEX_DEBUG_WARN(output, GLX::style_not_found, IsNull(*this) || IsValid(*child), "GLX::Style::operator[] '", GetKey(key), kSingleQuote, " not found");

	return child;
}
