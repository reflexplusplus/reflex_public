#include "reflex/glx/detail/functions.h"
#include "reflex/glx/detail/reference.h"
#include "reflex/glx/functions/mods.h"
#include "reflex/glx/functions/lookup.h"
#include "reflex/glx/functions/input.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

constexpr Float kScrollInverted[2] = { 1.0f, -1.0f };
constexpr Float kScrollFast[2] = { 1.0f, 4.0f };

constexpr UInt32 kTransactionState = K32("{TransactionState}");

struct TransactionState : public ObjectOf < Pair <TransactionStage, UInt> >
{
	TransactionState()
		: ObjectOf(kTransactionStageBegin, 0)
	{
	}
};

const WChar kEllipsisData[] = { WChar(8230), WChar(0)};

const WString::View kEllipsis = { kEllipsisData, 1 };

const SIMD::FloatV4 k255f = 255.0f;

REFLEX_END_INTERNAL

const Reflex::Key32 Reflex::GLX::Detail::kAlignmentKeys[kNumAlignment] =
{
	K32("top_left"),
	K32("top"),
	K32("top_right"),
	K32("left"),
	K32("center"),
	K32("right"),
	K32("bottom_left"),
	K32("bottom"),
	K32("bottom_right")
};

const Reflex::Pair <Reflex::Float> Reflex::GLX::Detail::kOrientationToAlign1D[4] = { {0.0f, 0.0f}, {0.5f,0.5f}, {1.0f,1.0f}, {0.0f,0.0f} };

bool Reflex::GLX::Detail::GetBool(const Data::PropertySet & properties, Key32 id, bool default_value)
{
	if (auto q = properties.QueryProperty<Data::BoolProperty>(id)) return q->value;

	if (auto attribute = GetProperty<Data::Key32Property>(properties, id))
	{
		return attribute->value == ktrue;
	}

	return default_value;
}

Reflex::GLX::Point Reflex::GLX::Detail::Align(Size owner, Size size, Alignment alignment)
{
	auto origin = kAlignOrigin[alignment];

	return Reinterpret<Point>((owner * origin) - (size * origin));
}

Reflex::GLX::Point Reflex::GLX::Detail::Align(Size owner, const Rect & rect, Alignment alignment)
{
	auto origin = kAlignOrigin[alignment];

	auto invert = kAlignInvert[alignment];

	Point rtn = Reinterpret<Point>((owner * origin) - (rect.size * origin));

	rtn.x += (rect.origin.x * invert.w);
	rtn.y += (rect.origin.y * invert.h);

	return rtn;
}

Reflex::GLX::Point Reflex::GLX::Detail::AlignText(const Font & font, Float lineh, const WString::View & string, Size size, Alignment alignment)
{
	Size textsize = { font.GetTextWidth(string), lineh };

	Point position = Align(size, textsize, alignment);

	position.y += textsize.h;

	return SnapToPixels(position);
}

bool Reflex::GLX::Detail::TruncateRight(const Font & font, Float width, WString & string)
{
	WString::View view = string;

	if (font.GetTextWidth(view) > width)
	{
		Float dotsw = font.GetTextWidth(kEllipsis);

		while (font.GetTextWidth(view) + dotsw > width)
		{
			if (view.size)
			{
				view.size--;
			}
			else
			{
				return false;
			}
		}

		string = Join(view, kEllipsis);
	}

	return true;
}

bool Reflex::GLX::Detail::TruncateLeft(const Font & font, Float width, WString & string)
{
	WString::View view = string;

	if (font.GetTextWidth(view) > width)
	{
		Float dotsw = font.GetTextWidth(kEllipsis);

		while (font.GetTextWidth(view) + dotsw > width)
		{
			if (view.size)
			{
				view.data++;
				view.size--;
			}
			else
			{
				return false;
			}
		}

		string = Join(kEllipsis, view);
	}

	return true;
}

bool Reflex::GLX::Detail::TruncatePath(const Font & font, Float width, WString & string)
{
	WString::View view = string;

	if (font.GetTextWidth(view) > width)
	{
		WString::View ellipsis(L".../");

		Float dotsw = font.GetTextWidth(ellipsis);

		do
		{
			if (auto pos = Search(view, File::kStroke))
			{
				view = Splice(view, pos.value + 1).b;
			}
			else
			{
				return TruncateLeft(font, width, string);
			}
		}
		while (font.GetTextWidth(view) + dotsw > width);

		string = Join(ellipsis, view);
	}

	return true;
}

Reflex::UInt8 Reflex::GLX::Detail::ParseEnum(UInt32 value, const ArrayView <UInt32> & values, UInt8 default_)
{
	UInt8 idx = 0;

	for (auto & i : values)
	{
		if (i == value) return idx;

		idx++;
	}

	return default_;
}

void Reflex::GLX::Detail::PerformStandardScroll(Object & viewbar, Float vo, Float delta, bool y, bool inverted, bool fast)
{
	vo -= delta * kScrollInverted[inverted] * kScrollFast[fast];

	auto e = Make<Event>(kTransaction);
	
	Data::SetFloat32(e, koffset, vo);

	auto stage = Data::Detail::AcquireProperty<Data::UInt8Property>(e, kstage);

	REFLEX_LOOP(idx, 3)
	{
		stage->value = UInt8(kTransactionStageBegin + idx);

		viewbar.Emit(e);
	}
}

bool Reflex::GLX::Detail::BeginTransaction(Object & object, UInt idx)
{
	auto stateref = Data::Detail::AcquireProperty<TransactionState>(object, kTransactionState);

	auto & state = stateref->value;

	if (state.a != kTransactionStagePerform)
	{
		state = { kTransactionStagePerform, idx };

		EmitTransaction(object, kTransactionStageBegin, idx);

		return true;
	}
	else
	{
		return false;
	}
}

void Reflex::GLX::Detail::PerformTransaction(Object & object, UInt idx, Float32 value)
{
	auto stateref = Data::Detail::AcquireProperty<TransactionState>(object, kTransactionState);

	auto & state = stateref->value;

	if (state.a == kTransactionStagePerform)
	{
		EmitTransaction(object, kTransactionStagePerform, idx, value);
	}
	else
	{
		state = { kTransactionStagePerform, idx };

		Core::WeakReference ref(object);

		EmitTransaction(object, kTransactionStageBegin, idx);

		EmitTransaction(*ref, kTransactionStagePerform, idx, value);
	}
}

void Reflex::GLX::Detail::EndTransaction(Object & object, UInt idx, bool cancel)
{
	if (auto pstate = object.QueryProperty<TransactionState>(kTransactionState))
	{
		auto & state = pstate->value;

		if (state.b == idx)
		{
			Core::WeakReference ref(object);

			if (state.a == kTransactionStagePerform)
			{
				state.a = cancel ? kTransactionStageCancel : kTransactionStageEnd;

				EmitTransaction(object, state.a, state.b);
			}

			ref->UnsetProperty<TransactionState>(kTransactionState);
		}
	}
}

Reflex::System::iRect Reflex::GLX::Detail::ToInt(const Rect & rect)
{
	auto irect = Truncate(SIMD::LoadUnaligned(&rect.origin.x) + Core::kRoundingTolerance);

	return Reinterpret<System::iRect>(irect);
}

Reflex::GLX::Colour Reflex::GLX::Detail::ToColour(const ArrayView <Float32> & floats, const Colour & fallback)
{
	switch (floats.size)
	{
		case 3:
		{
			SIMD::FloatV4 rgba = { floats[0], floats[1], floats[2], 255.0f };

			rgba /= k255f;

			rgba[3] = 1.0f;

			return Reinterpret<Colour>(rgba);
		}
		case 4:
		{
			SIMD::FloatV4 rgba = SIMD::LoadUnaligned(floats.data);

			rgba /= k255f;

			return Reinterpret<Colour>(rgba);
		}	
		case 1:
		{
			SIMD::FloatV4 rgba = floats.GetFirst() / 255.0f;

			rgba[3] = 1.0f;

			return Reinterpret<Colour>(rgba);
		}
		case 2:
		{
			auto grey = floats[0] / 255.0f;

			auto alpha = floats[1] / 255.0f;

			return { grey, grey, grey, alpha };
		}

	}

	return fallback;
}

Reflex::GLX::Colour Reflex::GLX::Detail::ToColour(const CString::View & hex, const Colour & fallback)
{
	UInt8 bytes[8];

	switch (hex.size)
	{
	case 6:
		Data::Detail::HexToBytes({ bytes, 3 }, hex);
		return RGB(bytes[0], bytes[1], bytes[2]);

	case 8:
		Data::Detail::HexToBytes({ bytes, 4 }, hex);
		return RGB(bytes[0], bytes[1], bytes[2], bytes[3]);

	case 3:
	case 4:
	{
		char hex_expanded[8];
		hex_expanded[6] = 'F';
		hex_expanded[7] = 'F';
		UInt idx = 0;
		for (auto & i : hex)
		{
			hex_expanded[idx++] = i;
			hex_expanded[idx++] = i;
		}
		Data::Detail::HexToBytes({ bytes, 4 }, { hex_expanded, 8 });
		return RGB(bytes[0], bytes[1], bytes[2], bytes[3]);
	}

	default:
		return fallback;
	}
}

const Reflex::Key32 Reflex::GLX::Detail::kOrientationKeys[kNumOrientation] = { K32("near"), K32("center"), K32("far"), K32("fit") };

const Reflex::GLX::Scale Reflex::GLX::Detail::kAlignOrigin[9] =
{
	{ 0.0f, 0.0f },
	{ 0.5f, 0.0f },
	{ 1.0f, 0.0f },
	{ 0.0f, 0.5f },
	{ 0.5f, 0.5f },
	{ 1.0f, 0.5f },
	{ 0.0f, 1.0f },
	{ 0.5f, 1.0f },
	{ 1.0f, 1.0f },
};

const Reflex::GLX::Scale Reflex::GLX::Detail::kAlignInvert[9] =
{
	{ 1.0f, 1.0f },
	{ 1.0f, 1.0f },
	{ -1.0f, 1.0f },
	{ 1.0f, 1.0f },
	{ 1.0f, 1.0f },
	{ -1.0f, 1.0f },
	{ 1.0f, -1.0f },
	{ 1.0f, -1.0f },
	{ -1.0f, -1.0f },
};
