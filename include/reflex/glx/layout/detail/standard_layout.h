#pragma once

#include "layout_model.h"




//
// Detail

REFLEX_NS(Reflex::GLX::Detail)

enum StandardLayoutFlags
{
	kStandardLayoutY,
	kStandardLayoutInvert,
	kStandardLayoutCenter,
	kStandardLayoutWrap,

	kStandardLayoutAutofitX,
	kStandardLayoutAutofitY,
	kStandardLayoutPadding,
	kStandardLayoutMaxOrScaledOrAspectRatio,
};

enum PositioningFlags
{
	kPositioningFlagsModeA,
	kPositioningFlagsModeB,

	kPositioningFlagsAxisA,
	kPositioningFlagsAxisB,

	kPositioningFlagsOrthoA,
	kPositioningFlagsOrthoB,

	kAlignFlagsReservedA,
	kAlignFlagsReservedB,
};

enum Positioning : UInt8
{
	kPositioningInline,
	kPositioningFloat,
	kPositioningAbsolute,
	kPositioningAbsoluteFloat,	//experimental

	kNumPositioning,
};

struct StandardLayout;



//renderer helpers

void AccommodateRenderer(Object & object, bool & responsive, Size & contentsize);

void AlignRenderer(Object & object, Float & contenth);

REFLEX_END




//
//Detail::StandardLayout

struct Reflex::GLX::Detail::StandardLayout : public LayoutModel
{
	StandardLayout(GLX::Object & owner) {}

	Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & owner, UInt8 flags) override;

	Float inline_size = 0.0f;

	Float nflex = 0.0f;
};
