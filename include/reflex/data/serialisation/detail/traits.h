#pragma once

#include "../../types.h"




//
//macros

#define REFLEX_DATA_SET_STREAM_INDEX_TYPE(TYPE, INDEX) namespace Reflex::Data::Detail { template <> struct IndexType <TYPE> { typedef INDEX Type; }; }




//
//impl

REFLEX_NS(Reflex::Data::Detail)

template <class TYPE> struct IsRawPackable { static constexpr bool value = kIsRawCopyable<TYPE>; };

REFLEX_PUBLISH_TRAIT_VALUE(IsRawPackable);

template <> struct IsRawPackable <bool> { static constexpr bool value = false; };	//XCODE

template <> struct IsRawPackable <WChar> { static constexpr bool value = false; };	//variable size

template <class TYPE> struct IsRawPackable < Array <TYPE> > { static constexpr bool value = kIsRawPackable<TYPE>; };

template <class TYPE> struct IsRawPackable < ArrayRegion <TYPE> > { static constexpr bool value = kIsRawPackable<TYPE>; };

template <class TYPE> struct IsRawPackable < TRef <TYPE> > { static constexpr bool value = false; };	//XCODE


template <class TYPE> struct IsStreamable { static constexpr bool value = !IsPointer<TYPE>::value; };

REFLEX_PUBLISH_TRAIT_VALUE(IsStreamable);

template <class TYPE> struct IsStreamable < Reference <TYPE> > { static constexpr bool value = false; };

template <class TYPE> struct IsStreamable < TRef <TYPE> > { static constexpr bool value = false; };


template <class TYPE> struct IndexType { typedef UInt32 Type; };

REFLEX_END

REFLEX_DATA_SET_STREAM_INDEX_TYPE(Array<char>,UInt16);

REFLEX_DATA_SET_STREAM_INDEX_TYPE(Array<WChar>,UInt16);