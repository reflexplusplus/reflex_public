#pragma once

#include "../../core/object.h"




//
//Detail

REFLEX_NS(Reflex::GLX::Detail)

class LayoutModel;


using LayoutModelCtr = FunctionPointer <TRef<LayoutModel>(GLX::Object&)>;


template <class OBJECT> static Core::Object::AccommodateFn CastAccommodateFn(FunctionPointer <void(OBJECT & object, bool & isresponsive, System::fSize & contentsize)> accommodate);

template <class OBJECT> static Core::Object::AlignFn CastAlignFn(FunctionPointer <void(OBJECT & object, bool isresponsive, Float & contenth)> align);

template <class OBJECT> static Pair <Core::Object::AccommodateFn,Core::Object::AlignFn> CastLayoutFns(FunctionPointer <void(OBJECT & object, bool & isresponsive, System::fSize & contentsize)> accommodate, FunctionPointer <void(OBJECT & object, bool isresponsive, Float & contenth)> align);

REFLEX_END




//
//Detail::LayoutModel

class Reflex::GLX::Detail::LayoutModel : public Reflex::Object
{
public:

	static LayoutModel & null;

	using AccommodateFn = Core::Object::AccommodateFn;

	using AlignFn = Core::Object::AlignFn;


	virtual Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) = 0;

};





//
//impl

REFLEX_SET_TRAIT(Reflex::GLX::Detail::LayoutModel, IsSingleThreadExclusive);

template <class OBJECT> inline Reflex::GLX::Detail::LayoutModel::AccommodateFn Reflex::GLX::Detail::CastAccommodateFn(FunctionPointer <void(OBJECT & object, bool & isresponsive, System::fSize & contentsize)> accommodate)
{
	return reinterpret_cast<Core::Object::AccommodateFn>(accommodate);
}

template <class OBJECT> inline Reflex::GLX::Detail::LayoutModel::AlignFn Reflex::GLX::Detail::CastAlignFn(FunctionPointer <void(OBJECT & object, bool isresponsive, Float & contenth)> align)
{
	return reinterpret_cast<Core::Object::AlignFn>(align);
}

template <class OBJECT> inline Reflex::Pair <Reflex::GLX::Detail::LayoutModel::AccommodateFn, Reflex::GLX::Detail::LayoutModel::AlignFn> Reflex::GLX::Detail::CastLayoutFns(FunctionPointer <void(OBJECT & object, bool & isresponsive, System::fSize & contentsize)> accommodate, FunctionPointer <void(OBJECT & object, bool isresponsive, Float & contenth)> align)
{
	return { CastAccommodateFn<OBJECT>(accommodate), CastAlignFn<OBJECT>(align) };
}
