#pragma once

#include "../layer.h"




//
//macros

REFLEX_NS(Reflex::GLX::Detail)

extern const Layer::Class g_vector_layers[10];
extern const Layer::Class g_gradient_layers[4];
extern const Layer::Class g_image_layers[3];
extern const Layer::Class g_text_layers[2];
extern const Layer::Class g_wrapper_layers[11];
extern const Layer::Class g_texture_fx_layers[2];
extern const Layer::Class g_colour_fx_layers[9];
extern const Layer::Class g_canvas_layers[4];
extern const Layer::Class g_special_layers[3];
extern const Layer::Class g_viewport_layers[2];

enum AlignmentEx : UInt8
{
	kNearNear,
	kCenterNear,
	kFarNear,
	kFitNear,

	kNearCenter,
	kCenterCenter,
	kFarCenter,
	kFitCenter,

	kNearFar,
	kCenterFar,
	kFarFar,
	kFitFar,

	kNearFit,
	kCenterFit,
	kFarFit,
	kFitFit,
};

enum AxisProperty : UInt8
{
	kAxisPropertyNone,
	kAxisPropertyX,
	kAxisPropertyY,
	kAxisPropertyXY,
};

inline AlignmentEx OrientationToAlignmentEx(Pair <Orientation> xy)
{
	return AlignmentEx(((Int(xy.b) * 4) + Int(xy.a)));
}

extern const Alignment kAlignmentExToAlignment[16];

extern const Size kAxisToSize[4];

const CString::View kAutoFit = "autofit";

REFLEX_END




//
//impl

REFLEX_INLINE Reflex::GLX::Alignment Reflex::GLX::Detail::OrientationToAlignment(Pair <Orientation> xy)
{
	return kAlignmentExToAlignment[OrientationToAlignmentEx(xy)];
}
