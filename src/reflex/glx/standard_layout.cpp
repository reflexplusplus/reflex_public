#include "standard_layout.h"
#include "../../../include/reflex/glx/detail/axis.h"
#include "../../../include/reflex/glx/object.h"




//
//functions

REFLEX_BEGIN_INTERNAL(Reflex::GLX::Detail)

struct NullLayout : public LayoutModel
{
	Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 flags) override 
	{
		return 
		{
			[](GLX::Object & object, bool & isresponsive, System::fSize & contentsize)
			{
			},
			[](GLX::Object & object, bool isresponsive, Float & contenth)
			{
			}
		};
	}
};


struct InlineData
{
	Float pos;
	Float flex;
	Float nflex;
	Float ortho_pos;	//for wrap
};


template <bool Y, bool INVERT, bool CENTER, bool WRAP, bool AUTOFIT_X, bool AUTOFIT_Y, bool PAD, bool MAX_SCALED_ASPECT>
void OnAccommodate(Object & object, bool & responsive, Size & contentsize);

REFLEX_TBINDER_8P(OnAccommodate);

template <bool Y, bool INVERT, bool CENTER, bool WRAP, bool AUTOFIT_X, bool AUTOFIT_Y, bool PAD, bool MAX_SCALED_ASPECT>
static void OnAlign(Object & object, bool responsive, Float & contenth);

REFLEX_TBINDER_8P(OnAlign);


typedef FunctionPointer <void(const Rect &, InlineData &, Object &)> PositioningFn;

template <bool Y, bool INVERT> PositioningFn GetPositioningFn(const Object & item);


template <class PARENT_AXIS, bool PARENT_INVERT, bool PARENT_WRAP, bool ITEM_FLEX, Orientation ORTHO_POS> static void InlineItem(const Rect & inner, InlineData & data, Object & item);

void AbsoluteItem(const Rect & inner, Object & item);


template <bool INVERT> static void WrapInlineItem(const Rect & inner, InlineData & data, Object & item);


struct Positioner
{
	template <bool Y, bool INVERT, bool ALIGNMENT_A, bool ALIGNMENT_D, bool AXIS_ORIENTATION_A, bool AXIS_ORIENTATION_B, bool ORTHO_ORIENTATION_A, bool ORTHO_ORIENTATION_B> static void PositionItem(const Rect & inner, InlineData & data, Object & item);

	REFLEX_TBINDER_8P(PositionItem);

	static inline const PositioningFn * kPositioningFns = &REFLEX_TBIND(PositionItem, 0);
};

template <bool Y, bool INVERT> REFLEX_INLINE PositioningFn GetPositioningFn(const Object & item)
{
	return Positioner::kPositioningFns[Bits<Y,INVERT>::value | (item.GetPositioningFlags() << 2)];
}

template <bool Y, bool INVERT, bool ALIGNMENT_A, bool ALIGNMENT_B, bool AXIS_ORIENTATION_A, bool AXIS_ORIENTATION_B, bool ORTHO_ORIENTATION_A, bool ORTHO_ORIENTATION_B> void Positioner::PositionItem(const Rect & inner, InlineData & data, Object & item)
{
	typedef ConditionalType <Y,YAxis,XAxis> Axis;

	constexpr auto positioning = Bits<ALIGNMENT_A,ALIGNMENT_B>::value;

	constexpr auto AXIS_ORIENTATION = Orientation(Bits<AXIS_ORIENTATION_A,AXIS_ORIENTATION_B>::value);
	
	constexpr auto ORTHO_ORIENTATION = Orientation(Bits<ORTHO_ORIENTATION_A,ORTHO_ORIENTATION_B>::value);

	if constexpr (positioning == kPositioningInline)
	{
		InlineItem<Axis, INVERT, false, bool(AXIS_ORIENTATION_A & AXIS_ORIENTATION_B), ORTHO_ORIENTATION>(inner, data, item);
	}
	else if constexpr (positioning == kPositioningFloat)
	{
		FloatItem<AXIS_ORIENTATION,ORTHO_ORIENTATION>(inner, item);
	}
	else if constexpr (positioning == kPositioningAbsolute)
	{
		AbsoluteItem(inner, item);
	}
	else if constexpr (positioning == kPositioningAbsoluteFloat)
	{
		Rect rect = { GetProperty<PointProperty>(item, kNullKey)->value, {} };

		if constexpr (AXIS_ORIENTATION == kOrientationFit)
		{
			rect.origin.x += inner.origin.x;

			rect.size.w = inner.size.w;
		}

		if constexpr (ORTHO_ORIENTATION == kOrientationFit)
		{
			rect.origin.y += inner.origin.y;

			rect.size.h = inner.size.h;
		}

		FloatItem<AXIS_ORIENTATION,ORTHO_ORIENTATION>(rect, item);
	}

	item.SetScale(Reflex::MakeSize(item.GetComputedStyle()->GetScale()));
}

template <bool INVERT> void WrapInlineItem(const Rect & inner, InlineData & data, Object & item)
{
	InlineItem<XAxis,INVERT,true,false,kOrientationNear>(inner, data, item);
}

template <class PARENT_AXIS, bool PARENT_INVERT, bool PARENT_WRAP, bool ITEM_FLEX, Orientation ORTHO_ALIGNMENT> REFLEX_INLINE void InlineItem(const Rect & inner, InlineData & data, Object & item)
{
	typedef PARENT_AXIS Axis;

	typedef typename Axis::Ortho Ortho;

	item.ComputeLayout();

	auto & contentsize = item.contentsize;

	auto cstyle = item.GetComputedStyle();

	auto & margin = cstyle->GetMargin();

	auto & near = margin.near;

	auto & far = margin.far;

	Float ortho_origin;

	Float ortho_size;

	if constexpr (PARENT_WRAP)
	{
		ortho_size = Ortho::GetSize(contentsize);

		ortho_origin = data.ortho_pos + Ortho::GetPoint(inner.origin) + Ortho::GetSize(near);
	}
	else
	{
		switch (ORTHO_ALIGNMENT)
		{
		case kOrientationNear:
			ortho_size = Ortho::GetSize(contentsize);
			ortho_origin = Ortho::GetPoint(inner.origin) + Ortho::GetSize(near);
			break;

		case kOrientationCenter:
			ortho_size = Ortho::GetSize(contentsize);
			ortho_origin = Center<Ortho>(inner, margin, contentsize);
			break;

		case kOrientationFar:
			ortho_size = Ortho::GetSize(contentsize);
			ortho_origin = Ortho::GetPoint(inner.origin) + Ortho::GetSize(inner.size) - (ortho_size + Ortho::GetSize(far));
			break;

		case kOrientationFit:
			ortho_size = Max(Ortho::GetSize(inner.size) - (Ortho::GetSize(near) + Ortho::GetSize(far)), 0.0f);
			ortho_origin = Ortho::GetPoint(inner.origin) + Ortho::GetSize(near);
			break;
		};
	}

	Float & pos = data.pos;

	Float axis_size = Axis::GetSize(contentsize);

	if constexpr ((!PARENT_WRAP) & ITEM_FLEX)
	{
		Float flex = Min(RoundDown<true>(data.flex / data.nflex), Axis::GetSize(cstyle->GetMinMax().b) - axis_size);

		data.flex -= flex;

		data.nflex -= 1.0f;

		axis_size = Max(axis_size + flex, 0.0f);	//means cannot be bigger than parent, alternative is Max(axis_size + flex, axis_size)	(can extend parent)
	}

	if constexpr (PARENT_INVERT)
	{
		pos -= Axis::GetSize(far);
		pos -= axis_size;
	}
	else
	{
		pos += Axis::GetSize(near);
	}

	item.SetRect({ Axis::MakePoint(pos, ortho_origin), Axis::MakeSize(axis_size, ortho_size) / Reflex::MakeSize(cstyle->GetScale()) });

	if constexpr (PARENT_INVERT)
	{
		pos -= Axis::GetSize(near);
	}
	else
	{
		pos += axis_size;

		pos += Axis::GetSize(far);
	}
}

REFLEX_INLINE void AbsoluteItem(const Rect & inner, Object & item)
{
	Size size = ComputeContentSize(item) / Reflex::MakeSize(item.GetComputedStyle()->GetScale());

	item.SetSize(size);
}

inline const Float kOrientationToFlex[kNumOrientation] = { 0.0f, 0.0f, 0.0f, 1.0f };

Reflex::Detail::StaticObject <NullLayout> g_null_layout;


template <bool Y, bool INVERT, bool CENTER, bool WRAP, bool AUTOFIT_X, bool AUTOFIT_Y, bool PAD, bool MAX_SCALED_ASPECT>
void OnAccommodate(Object & object, bool & isresponsive, Size & contentsize)
{
	typedef ConditionalType <Y,YAxis,XAxis> AXIS;

	typedef typename AXIS::Ortho Ortho;


	auto self = Cast<StandardLayout>(object.GetLayoutModel().RemoveConst());

	auto computedstyle = object.GetComputedStyle();

	auto & size = object.GetRect().size;


	Rect inner = { kOrigin, size };

	if constexpr (PAD) inner = Indent(size, computedstyle->GetPadding());



	//collect content_size

	isresponsive = WRAP;

	if constexpr (!WRAP && MAX_SCALED_ASPECT)
	{
		isresponsive = True(computedstyle->GetPropertyFlags() & ComputedStyle::kPropertyFlagAspectRatio);
	}

	self->inline_size = 0.0f;

	self->nflex = 0.0f;


	AccommodateRenderer(object, isresponsive, contentsize);


	auto itr = object.GetFirst();

	while (itr)
	{
		auto & item = *itr;


		item.ComputeLayout();

		auto computedstyle = item.GetComputedStyle();

		auto & size = item.contentsize;


		Size sum = size + Sum(computedstyle->GetMargin());

		auto positioning = Detail::GetPositioning(item);

		if (positioning.a == kPositioningInline)
		{
			self->nflex += kOrientationToFlex[positioning.b];

			self->inline_size += AXIS::GetSize(sum);

			Ortho::SetSize(contentsize, Max(Ortho::GetSize(contentsize), Ortho::GetSize(sum)));
		}
		else
		{
			contentsize = Max(contentsize, sum);
		}

		if constexpr (!WRAP)
		{
			isresponsive = isresponsive || item.isresponsive;
		}

		itr = item.GetNext();
	}

	AXIS::SetSize(contentsize, Max(AXIS::GetSize(contentsize), self->inline_size));

	Size paddingsum = Sum(computedstyle->GetPadding());

	contentsize += paddingsum;



	//calculate content size

	if constexpr (!AUTOFIT_X) contentsize.w = paddingsum.w;

	if constexpr (!AUTOFIT_Y) contentsize.h = paddingsum.h;

	auto [min,max] = computedstyle->GetMinMax();

	contentsize = Max(contentsize, min);

	if constexpr (MAX_SCALED_ASPECT)
	{
		contentsize = Min(contentsize, max);

		contentsize *= Reflex::MakeSize(computedstyle->GetScale());

		contentsize.w = RoundUp<true>(contentsize.w);

		contentsize.h = RoundUp<true>(contentsize.h);
	}
}

template <bool Y, bool INVERT, bool CENTER, bool WRAP, bool AUTOFIT_X, bool AUTOFIT_Y, bool PAD, bool MAX_SCALED_ASPECT>
void OnAlign(Object & object, bool isresponsive, Float & contenth)
{
	typedef ConditionalType <Y,YAxis,XAxis> AXIS;

	typedef typename AXIS::Ortho Ortho;


	auto self = Cast<StandardLayout>(object.GetLayoutModel().RemoveConst());

	auto cstyle = object.GetComputedStyle();

	auto & padding = cstyle->GetPadding();

	auto & size = object.GetRect().size;

	Rect inner = { kOrigin, size };

	if constexpr (PAD) inner = Indent(size, cstyle->GetPadding());

	REFLEX_ASSERT(WRAP ? isresponsive : true);


	//TEST

	Float32 renderercontenth = 0.0f;

	Float axis_size = AXIS::GetSize(inner.size);

	InlineData inlinedata = { AXIS::GetPoint(inner.origin), axis_size - self->inline_size, self->nflex, 0.0f };

	if constexpr (INVERT) inlinedata.pos += axis_size;

	if constexpr (WRAP)
	{
		contenth = 0.0f;

		if (axis_size > 0.0f)
		{
			AlignRenderer(object, renderercontenth);

			Float wrap = INVERT ? AXIS::GetPoint(inner.origin) : AXIS::GetPoint(inner.origin) + axis_size;


			auto itr = object.GetFirst();

			Float rowh = 0.0f;


			auto center_rowstart = itr;

			Float center_rowend = 0.0f;


			while (itr)
			{
				auto & item = *itr;

				WrapInlineItem<INVERT>(inner, inlinedata, item);

				auto & item_margin = item.GetComputedStyle()->GetMargin();

				Size sum = item.contentsize + Sum(item_margin);

				if (INVERT ? inlinedata.pos < wrap : inlinedata.pos > wrap)
				{
					if constexpr (CENTER)
					{
						auto shift = Reflex::RoundDown((INVERT ? (inner.origin.x - center_rowend) : (inner.size.w - (center_rowend - inner.origin.x))) * 0.5f);

						while (center_rowstart != itr)
						{
							auto rect = center_rowstart->GetRect();

							rect.origin.x += shift;

							center_rowstart->SetRect(rect);

							center_rowstart = center_rowstart->GetNext();
						}

						center_rowstart = itr;
					}

					inlinedata.pos = AXIS::GetPoint(inner.origin);

					if constexpr (INVERT) inlinedata.pos += axis_size;

					inlinedata.ortho_pos += rowh;

					WrapInlineItem<INVERT>(inner, inlinedata, item);

					contenth += rowh;

					rowh = sum.h;
				}
				else
				{
					rowh = Max(rowh, sum.h);// -Ortho::GetPoint(inner.origin);
				}

				contenth = Max(contenth, inlinedata.ortho_pos + sum.h);// -Ortho::GetPoint(inner.origin);

				auto & endrect = item.GetRect();

				center_rowend = INVERT ? endrect.origin.x - item_margin.near.w : endrect.origin.x + endrect.size.w + item_margin.far.w;

				itr = item.GetNext();
			}

			if constexpr (CENTER)
			{
				auto shift = Reflex::RoundDown((INVERT ? (inner.origin.x - center_rowend) : (inner.size.w - (center_rowend - inner.origin.x))) * 0.5f);

				while (center_rowstart)
				{
					auto rect = center_rowstart->GetRect();

					rect.origin.x += shift;

					center_rowstart->SetRect(rect);

					center_rowstart = center_rowstart->GetNext();
				}
			}
		}

		Float paddingsum = padding.near.h + padding.far.h;

		contenth += paddingsum;

		//if constexpr (!AUTOFIT_Y) contenth = 0.0f;

		//auto & minmax = cstyle->GetMinMax();

		//contenth = Max(contenth, minmax.a.h);

		//if constexpr (MAX_OR_SCALED)
		//{
		//	contenth = Min(contenth, minmax.b.h);

		//	contenth = RoundNearest<SNAP>(contenth * cstyle->GetScale());
		//}
	}
	else
	{
		auto nflex = self->nflex;

		if constexpr (CENTER)
		{
			//TODO clean this up, just do CENTER as a nudeg per item afterwards, including compensation for remaining flex.
			//dont need to do in responsive first pass, only final

			if (!nflex)
			{
				Float offset = RoundNearest<true>((axis_size - self->inline_size) * 0.5f);

				if constexpr (INVERT)
				{
					inlinedata.pos -= offset;
				}
				else
				{
					inlinedata.pos += offset;
				}
			}
		}

		if (isresponsive)	//this means some children are rspnsv
		{
			if (size.w > 0.0f)
			{
				AlignRenderer(object, renderercontenth);

				contenth = 0.0f;

				self->inline_size = 0.0f;

				auto itr = object.GetFirst();

				while (itr)
				{
					auto & item = *itr;

					auto positioningfn = GetPositioningFn<Y, INVERT>(item);

					(*positioningfn)(inner, inlinedata, item);

					auto cstyle = item.GetComputedStyle();

					Size sum = item.contentsize + Sum(cstyle->GetMargin());

					if (Detail::GetPositioning(item).a == kPositioningInline)
					{
						self->inline_size += AXIS::GetSize(sum);

						if constexpr (AXIS::kX) contenth = Max(contenth, sum.h);	//Ortho::SetSize(contentsize, Max(Ortho::GetSize(contentsize), Ortho::GetSize(sum)));
					}
					else
					{
						contenth = Max(contenth, sum.h);
					}

					itr = item.GetNext();
				}

				if constexpr (AXIS::kY) contenth = Max(contenth, self->inline_size);

				Float paddingsum = padding.near.h + padding.far.h;

				contenth += paddingsum;

				OnAlign<Y, INVERT, CENTER, false, AUTOFIT_X, AUTOFIT_Y, PAD, MAX_SCALED_ASPECT>(object, false, contenth);
			}
		}
		else
		{
			if (size.w > 0.0f && size.h > 0.0f)
			{
				AlignRenderer(object, renderercontenth);

				auto itr = object.GetFirst();

				while (itr)
				{
					auto & item = *itr;

					auto alignfn = GetPositioningFn<Y, INVERT>(item);

					(*alignfn)(inner, inlinedata, item);

					itr = item.GetNext();
				}

				if constexpr (CENTER)
				{
					if (nflex && inlinedata.flex)
					{
						auto shift = inlinedata.flex * 0.5f;

						for (auto & i : object)
						{
							if (Detail::GetPositioning(i).a == kPositioningInline)
							{
								AXIS::IncPoint(RemoveConst(i.GetRect().origin), shift);
							}
						}
					}
				}
			}

			return;	//! dont do responsive
		}
	}


	//complete responsive

	REFLEX_ASSERT(isresponsive);

	auto & minmax = cstyle->GetMinMax();

	auto minh = minmax.a.h;

	if constexpr (MAX_SCALED_ASPECT)
	{
		Float height = size.w * cstyle->GetHeightRatio();

		contenth = Max(height, Max(contenth, minh));

		if constexpr (AUTOFIT_Y) contenth = Max(contenth, renderercontenth);
	}
	else if constexpr (AUTOFIT_Y)
	{
		contenth = Max(Max(contenth, minh), renderercontenth);
	}
	else
	{
		contenth = minh;
	}

	if constexpr (MAX_SCALED_ASPECT)
	{
		auto maxh = minmax.b.h;

		contenth = RoundNearest<true>(Min(contenth, maxh) * cstyle->GetScale());
	}
}

REFLEX_END_INTERNAL

Reflex::GLX::Detail::LayoutModel & Reflex::GLX::Detail::LayoutModel::null = Reflex::GLX::Detail::g_null_layout;

Reflex::Pair <Reflex::GLX::Core::Object::AccommodateFn,Reflex::GLX::Core::Object::AlignFn> Reflex::GLX::Detail::StandardLayout::OnRebuild(GLX::Object & object, UInt8 layout_flags)
{
	constexpr UInt8 kMask = Bits<true, true, true, false, true, true, true, true>::value;

	layout_flags &= kMask;

	return { OnAccommodateBinder::Bind(layout_flags), OnAlignBinder::Bind(layout_flags) };
}

const Reflex::GLX::Detail::LayoutModelCtr Reflex::GLX::kStandardLayout = [](Object & object) -> TRef <Detail::LayoutModel>
{
	return New<Detail::StandardLayout>();
};

const Reflex::GLX::Detail::LayoutModelCtr Reflex::GLX::Detail::kStandardLayoutRealigningContent = [](Object & object) -> TRef <LayoutModel>
{
	struct StandardLayoutEx : public StandardLayout
	{
		Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override
		{
			auto base = StandardLayout::OnRebuild(object, layout_flags);

			std_align = base.b;
			
			base.b = [](GLX::Object & object, bool isresponsive, Float & contenth)
			{
				Core::RealignBranch(object);

				auto layout = Cast<StandardLayoutEx>(object.GetLayoutModel());

				layout->std_align(object, isresponsive, contenth);
			};

			return base;
		}

		AlignFn std_align;
	};

	return New<StandardLayoutEx>();
};

const Reflex::GLX::Detail::LayoutModelCtr Reflex::GLX::kStandardLayoutWrapped = [](Object & object) -> TRef <Detail::LayoutModel>
{
	struct StandardLayoutWrapped : public Detail::StandardLayout
	{
		Pair <AccommodateFn, AlignFn> OnRebuild(GLX::Object & object, UInt8 layout_flags) override
		{
			layout_flags |= MakeBit(Detail::kStandardLayoutWrap);

			layout_flags = BitClear(layout_flags, Detail::kStandardLayoutY);

			return { Detail::OnAccommodateBinder::Bind(layout_flags), Detail::OnAlignBinder::Bind(layout_flags) };
		}
	};

	return New<StandardLayoutWrapped>();
};
