#pragma once

#include "geometry.h"




//
//Primary API

namespace Reflex::GLX
{

	using Colour = System::Colour;

	using Graphic = System::Renderer::Graphic;

	class StyleSheet;

	class Style;

	class Object;


	REFLEX_USE_ENUM(System,KeyCode);

	REFLEX_USE_ENUM(System,ModifierKeys);
	constexpr ModifierKeys kModifierKeyNone = ModifierKeys(0);
	#if defined(REFLEX_OS_MACOS) || defined(REFLEX_OS_IOS)
	constexpr ModifierKeys kModifierKeyPrimary = kModifierKeySystem;
	#else
	constexpr ModifierKeys kModifierKeyPrimary = kModifierKeyCtrl;
	#endif


	REFLEX_USE_ENUM(System,MouseCursor);

	REFLEX_USE_ENUM(Core,Trap);

	enum Alignment : UInt8
	{
		kAlignmentTopLeft,
		kAlignmentTop,
		kAlignmentTopRight,

		kAlignmentLeft,
		kAlignmentCenter,
		kAlignmentRight,

		kAlignmentBottomLeft,
		kAlignmentBottom,
		kAlignmentBottomRight,

		kNumAlignment,
	};

	enum ClickFlags : UInt8
	{
		kClickFlagRmb = 1,
		kClickFlagDbl = 2,
	};

	enum TransactionStage : UInt8
	{
		kTransactionStageNull,

		kTransactionStageBegin,
		kTransactionStagePerform,
		kTransactionStageEnd,
		kTransactionStageCancel,

		//deprecated aliases
		kTransactionNull = kTransactionStageNull,
		kTransactionBegin = kTransactionStageBegin,
		kTransactionPerform = kTransactionStagePerform,
		kTransactionEnd = kTransactionStageEnd,
		kTransactionCancel = kTransactionStageCancel,
	};



	//states

	constexpr Key32 kInactiveState = MakeKey32("inactive");

	constexpr Key32 kSelectedState = MakeKey32("selected");

	constexpr Key32 kActiveState = MakeKey32("active");

	constexpr Key32 kUsedState = MakeKey32("used");

	constexpr Key32 kHoverState = MakeKey32("hover");

	constexpr Key32 kFocusedState = MakeKey32("focused");



	//flags

	constexpr UInt32 kresize = MakeKey32("resize");



	//values

	constexpr Point kOrigin = { 0.0f, 0.0f };

	constexpr Size kNormal = { 1.0f, 1.0f };

	constexpr Size kLarge = { 999999999.0f, 999999999.0f };


	constexpr Colour kTransparent = { 0.0f, 0.0f, 0.0f, 0.0f };

	constexpr Colour kWhite = { 1.0f, 1.0f, 1.0f, 1.0f };

	constexpr Colour kBlack = { 0.0f, 0.0f, 0.0f, 1.0f };


	using PointProperty = ObjectOf <Point>;

	using SizeProperty = ObjectOf <Size>;

	using ColourProperty = ObjectOf <Colour>;

	using ColorProperty = ColourProperty;

	using MarginProperty = ObjectOf <Margin>;

	using RangeProperty = ObjectOf <Range>;


	constexpr Range kNormalRange = { 0.0f, 1.0f };


	extern Output output;

}




//
//impl

REFLEX_NS(Reflex::GLX)

REFLEX_DECLARE_KEY32(id);
REFLEX_DECLARE_KEY32(data);
REFLEX_DECLARE_KEY32(value);
REFLEX_DECLARE_KEY32(range);
REFLEX_DECLARE_KEY32(opacity);
REFLEX_DECLARE_KEY32(indent);
REFLEX_DECLARE_KEY32(align);
REFLEX_DECLARE_KEY32(offset);
REFLEX_DECLARE_KEY32(axis);
REFLEX_DECLARE_KEY32(from);
REFLEX_DECLARE_KEY32(to);
REFLEX_DECLARE_KEY32(animate);
REFLEX_DECLARE_KEY32(content);

constexpr UInt32 kBgColourPropertyKeys[2] = { MakeKey32("bg_colour"), MakeKey32("bg_color") };
constexpr UInt32 kColourPropertyKeys[2] = { MakeKey32("colour"), MakeKey32("color") };
constexpr Key32 kXY[2] = { MakeKey32("x"), MakeKey32("y") };

REFLEX_END

REFLEX_STATIC_ASSERT(Reflex::kIsTrivial<Reflex::Pair<bool>>);
REFLEX_STATIC_ASSERT(Reflex::kIsTrivial<Reflex::GLX::Point>);
REFLEX_STATIC_ASSERT(Reflex::kIsTrivial<Reflex::GLX::Size>);
REFLEX_STATIC_ASSERT(Reflex::kIsTrivial<Reflex::GLX::Margin>);
REFLEX_STATIC_ASSERT(Reflex::kIsTrivial<Reflex::GLX::Range>);

REFLEX_NS(Reflex::Detail)
template <> struct ExternalNull <GLX::ColourProperty> { inline static StaticObject <GLX::ColourProperty> instance = GLX::kWhite; };
REFLEX_END
