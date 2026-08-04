#include "../../../../include/reflex/glx/widgets/rangebar.h"
#include "../../../../include/reflex/glx/behaviours/resizer.h"
#include "common.h"




//
//layout & style

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

const Alignment kJumpOrigin[4] = { kAlignmentLeft, kAlignmentTop, kAlignmentRight, kAlignmentBottom };

REFLEX_END_INTERNAL

struct Reflex::GLX::RangeBar::Layout : public Detail::LayoutModel
{
	using LayoutModel::LayoutModel;

	Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & base, UInt8 layout_flags) override
	{
		auto self = Cast<RangeBar>(base);

		auto y = True(layout_flags & GLX::kFlowY);

		auto x = !y;

		TRef bar = self->m_bar;

		if (self->m_enable_resize)
		{
			auto resizeable = ResizeBehaviour::Create();

			bar->SetDelegate(ResizeBehaviour::kClassID, resizeable);

			resizeable->EnableAxis(x, y);
		}
		else
		{
			bar->ClearDelegate(ResizeBehaviour::kClassID);
		}

		auto moveable = MoveBehaviour::Create();

		moveable->EnableAxis(x, y);

		bar->SetDelegate(MoveBehaviour::kClassID, moveable);

		return CastLayoutFns<RangeBar>(&Layout::OnAccommodate, OnAlignBinder::Bind(layout_flags | self->m_transactionflag));
	}

	static void OnAccommodate(RangeBar & self, bool & isresponsive, Size & contentsize)
	{
		contentsize = Max(self.GetComputedStyle()->GetMinMax().a, Detail::MakeSize(GetAxis(self), self.m_minsize));// m_bar_display.GetComputedStyle()->GetMinMax().a);

		Detail::AccommodateRenderer(self, isresponsive, contentsize);
	};

	template <bool Y, bool INVERT, bool TRANSACTION> static void OnAlign(RangeBar & self, bool isresponsive, Float & contenth)
	{
		typedef ConditionalType <Y, Detail::YAxis, Detail::XAxis> AXIS;

		typedef typename AXIS::Ortho Ortho;

		auto & size = self.Object::GetRect().size;

		Float & pixsize = self.m_pixsize;

		pixsize = AXIS::GetSize(size) / self.AbstractViewBar::extent;

		Rect rect = { AXIS::MakePoint(self.AbstractViewBar::region.start * pixsize), AXIS::MakeSize(Max(self.AbstractViewBar::region.length * pixsize, self.m_minsize), Ortho::GetSize(size)) };

		if constexpr (INVERT)
		{
			rect.origin = Detail::Align(size, rect, Y ? kAlignmentBottom : kAlignmentRight);
		}

		if constexpr (!TRANSACTION)
		{
			self.m_bar.SetRect(rect);

			self.m_bar.Object::SetSize(rect.size);
		}

		Detail::AlignRenderer(self, contenth);
	}

	REFLEX_TBINDER_3P(OnAlign);
};

Reflex::GLX::RangeBar::RangeBar()
	: AbstractViewBar(New<Layout>())
	, m_enable_resize(false)
	, m_transactionflag(0)
	, m_snap(0.0f)
	, m_minsize(16.0f)
	, m_range(New<RangeProperty>(kNormalRange))
	, m_region(New<RangeProperty>(kNormalRange))
{
	SetProperty(krange, m_range);

	SetProperty(K32("region"), m_region);

	EnableDraw(m_bar, kNullKey, false);

	AddAbsolute(*this, m_bar);
}

void Reflex::GLX::RangeBar::EnableResize(bool enable)
{
	m_enable_resize = enable;

	RebuildLayout();
}

void Reflex::GLX::RangeBar::SetSnap(Float snap)
{
	m_snap = snap;
}

Reflex::Float Reflex::GLX::RangeBar::GetSnap() const
{
	return m_snap;
}

bool Reflex::GLX::RangeBar::OnEvent(Object & src, Event & e)
{
	if (e.id == kTransaction && src == m_bar)
	{
		auto stage = GetTransactionStage(e);

		switch (stage)
		{
		case kTransactionStageBegin:
			m_transactionflag = 4;
			RebuildLayout();
			break;

		case kTransactionStageEnd:
		case kTransactionStageCancel:
			m_transactionflag = 0;
			RebuildLayout();
			break;
		};

		Float pixsize_rcp = Reciprocal(m_pixsize);

		bool y = GetAxis(*this);

		auto vo = Detail::GetPoint(y, m_bar.GetRect().origin) * pixsize_rcp;

		auto vr = Detail::GetSize(y, Detail::ComputeContentSize(m_bar)) * pixsize_rcp;

		if (IsInverted(*this)) vo = Detail::Align(MakeSize(AbstractViewBar::extent, 0.0f), Rect {{ vo, 0.0f }, { AbstractViewBar::region.length, 0.0f }}, kAlignmentRight).x;

		if (Float snap = m_snap)
		{
			Float mod = Modulo(vo, snap);

			Float pix = AbstractViewBar::region.length / Detail::GetSize(y, Object::GetRect().size);

			pix *= 8.0f;

			if (mod < pix || mod > (snap - pix))
			{
				vo = Quantise(vo, snap);
			}
		}

		SetRange(AbstractViewBar::extent, { vo, vr });

		Pair <Orientation> orientation = Detail::kAlignmentToOrientation[GetIndex(e)];

		EmitTransaction(stage, { vo, vr }, (&orientation.a)[y]);

		return true;
	}
	else if (e.id == kMouseWheel)
	{
		auto delta = GetDelta(e);

		Detail::PerformStandardScroll(*this, AbstractViewBar::region.start, delta.y, GetAxis(*this), IsInverted(*this), GetModifierKeys(e) & kModifierKeyPrimary);

		return true;
	}
	else if (e.id == kMouseDown)
	{
		bool y = GetAxis(*this);

		bool inverted = IsInverted(*this);

		Float pixsize_rcp = Reciprocal(m_pixsize);

		Rect mousepos = { GetPointerPosition(*this, e), {} };

		Float jump = Detail::GetPoint(y, Detail::Align(Object::GetRect().size, mousepos, kJumpOrigin[MakeBits(y, inverted)]) * MakeSize(pixsize_rcp)) - (AbstractViewBar::region.length * 0.5f);

		jump = Clip(jump, 0.0f, AbstractViewBar::extent - AbstractViewBar::region.length);

		EmitJump(jump);

		return true;
	}

	return Object::OnEvent(src, e);
}

void Reflex::GLX::RangeBar::OnSetRange()
{
	m_range->value.length = AbstractViewBar::extent;

	m_region->value = AbstractViewBar::region;
}

void Reflex::GLX::RangeBar::OnSetStyle(const Style & style)
{
	AbstractViewBar::OnSetStyle(style);

	m_minsize = Detail::GetNumber(style, K32("min_size"), 16.0f);
}

void Reflex::GLX::AbstractViewBar::SetRange(Float extent, Range region)
{
	REFLEX_STATIC_ASSERT(sizeof(Tuple<Float,Range>) == 12);

	extent = Max(extent, Core::kRoundingTolerance);

	region.length = Min(region.length, extent);

	region.start = Clip(region.start, 0.0f, extent - region.length);

	if (SetFiltered(*Reinterpret<Tuple<Float,Range>>(&RemoveConst(AbstractViewBar::extent)), MakeTuple(extent, region)))
	{
		OnSetRange();

		Realign();

		Update();
	}
}

bool Reflex::GLX::AbstractViewBar::EmitTransaction(TransactionStage stage, Range region, Orientation edge)
{
	auto e = Make<Event>(kTransaction);

	Data::SetUInt8(e, kstage, UInt8(stage));
	Data::SetFloat32(e, koffset, region.start);
	Data::SetFloat32(e, krange, region.length);
	e->SetProperty(K32("edge"), New<ObjectOf<Orientation>>(edge));

	return Emit(e);
}

bool Reflex::GLX::AbstractViewBar::EmitJump(Float vo)
{
	auto e = Make<Event>(kJump);

	Data::SetFloat32(e, koffset, vo);

	return Emit(e);
}
