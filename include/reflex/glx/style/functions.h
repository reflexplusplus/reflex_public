#pragma once

#include "../object.h"
#include "stylesheet.h"




//
//Primary API

namespace Reflex::GLX
{

	ConstTRef <StyleSheet> RetrieveStyleSheet(const WString::View & path, const Data::PropertySet & options = Data::PropertySet::null);


	ConstTRef <Style> FindStyle(const Style & style, Key32 id);

	ConstTRef <Style> FindStyle(const Object & object, Key32 id);

}




//
//impl

REFLEX_NS(Reflex::GLX::Detail)

TRef <Reflex::Object> DecodeStyleSheet(const File::ResourcePool::StreamContext & ctx, System::FileHandle & instream);

ConstTRef <Style> FindStyle(const Style & start, const ArrayView <Key32> & path);

template <class TYPE> ConstTRef <TYPE> FindResource(const Style & style, Key32 id);

const Reflex::Object * FindResource(const Style & style, Address adr, const Reflex::Object * fallback);

ConstTRef <Reflex::Object> RetrieveRelativeResource(const WString::View & filename, const Data::PropertySet & options, TypeID type_id, File::ResourcePool::Ctr ctr);

template <class TYPE> REFLEX_INLINE ConstTRef <TYPE> RetrieveRelativeResource(const WString::View & filename, const Data::PropertySet & options, File::ResourcePool::Ctr ctr)
{
	return Cast<TYPE>(RetrieveRelativeResource(filename, options, GetTypeID<TYPE>(), ctr));
}

bool EnumerateIncludes(const StyleSheet & stylesheet, const Function <bool(const StyleSheet&)> & enumerator);

bool ExtractProperty(const Style & style, Key32 address, Reflex::Object & out); //after StyleSheet decoding, style-properties are generally Data::Float32Array, this converts to c++ type

inline void ApplySubStyle(Object & object, const Style & style, Key32 id)
{
	object.SetStyle(*style.QuerySubStyle(id, object.GetStyle().Adr()));
}

REFLEX_END

inline Reflex::ConstTRef <Reflex::GLX::Style> Reflex::GLX::FindStyle(const Style & style, Key32 id)
{
	if (auto child = style.QuerySubStyle(id))
	{
		return child;
	}
	else
	{
		return Detail::FindStyle(style, { id });
	}
}

REFLEX_INLINE Reflex::ConstTRef <Reflex::GLX::StyleSheet> Reflex::GLX::RetrieveStyleSheet(const WString::View & path, const Data::PropertySet & options)
{
	File::ResourcePool::Lock lock(Core::desktop->resourcepool);

	return lock.Retrieve<StyleSheet>(path, options, &Detail::DecodeStyleSheet);
}

template <class TYPE> REFLEX_INLINE Reflex::ConstTRef <TYPE> Reflex::GLX::Detail::FindResource(const Style & style, Key32 id)
{
	return Cast<TYPE>(Detail::FindResource(style, MakeAddress<TYPE>(id), &Reflex::Detail::GetNullInstance<TYPE>()));
}
