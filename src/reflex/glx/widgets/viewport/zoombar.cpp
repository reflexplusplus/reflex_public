#include "zoombar.h"




//
//zoombar implementation

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

Reflex::TRef <ZoomArea> GetContainingZoomable(Object & object)
{
	return QueryParentByType<ZoomArea>(object, &ZoomArea::null);
}

struct ZoomState
{
	void Init(ZoomArea & viewport, ZoomBar & zoombar, const Event & e)
	{
		bool y = GetAxis(zoombar);

		min_vr = Detail::GetSize(y, viewport.GetMinView());

		Detail::ClearZoomToggle(viewport, zoombar);

		extent = zoombar.extent;
		region = zoombar.region;

		zoombar.EmitTransaction(kTransactionStageBegin, region);

		Float size = Detail::GetSize(y, zoombar.GetRect().size);

		zoompos = Clip(Detail::GetPoint(y, GetPointerPosition(zoombar, e)), 0.0f, size) / size;

		zoompos = region.start + (zoompos * region.length);

		drag_z = {};
	}

	Float zoompos;

	Point drag_z;

	Float extent;
	Range region;
	Float min_vr;
};

ZoomState g_zoom_state;

ZoomBar::ZoomBar()
	: AbstractViewBar([](GLX::Object & self) -> TRef <Detail::LayoutModel>
	{
		struct LayoutModel : public Detail::LayoutModel
		{
			Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & self, UInt8 layout_flags) override
			{
				return CastLayoutFns<ZoomBar>(&ZoomBar::OnAccommodate, (layout_flags & GLX::kFlowY) ? &ZoomBar::OnAlign<Detail::YAxis> : &ZoomBar::OnAlign<Detail::XAxis>);	
			}
		};

		return REFLEX_CREATE(LayoutModel);
	})
	, m_range(New<RangeProperty>(kNormalRange))
	, m_region(New<RangeProperty>(kNormalRange))
{
	SetProperty(krange, m_range);
	
	SetProperty(K32("region"), m_region);
		
	SetMouseCursor(kMouseCursorZoom);
}

void ZoomBar::OnSetRange()
{
	m_range->value.length = AbstractViewBar::extent;

	m_region->value = AbstractViewBar::region;

	if (SetFiltered(m_extent_z, AbstractViewBar::extent))
	{
		if (auto viewport = GetContainingZoomable(*this)) Detail::ClearZoomToggle(viewport, *this);
	}
}

void ZoomBar::OnSetStyle(const Style & style)
{
	AbstractViewBar::OnSetStyle(style);

	m_min_size = Detail::GetNumber(style, K32("min_size"), 16.0f);
}

bool ZoomBar::OnEvent(Object & src, Event & e)
{
	switch (e.id.value)
	{
	case kMouseDown:
		if (auto viewport = GetContainingZoomable(*this))
		{
			if (GLX::GetClickFlags(e) & GLX::kClickFlagRmb)
			{
				Detail::ToggleZoom(viewport, *this, GetAxis(*this));

				GLX::EnablePointerCapture(e, false);
			}
			else
			{
				g_zoom_state.Init(viewport, *this, e);

				GLX::EnablePointerCapture(e, true, true);
			}
		}
		return true;

	case kMouseDrag:
		{
			auto drag_inc = SetDelta(g_zoom_state.drag_z, GetDelta(e));

			if (Reinterpret<UInt64>(drag_inc))
			{
				Increment(-drag_inc, GetModifierKeys(e));
			}
		}
		return true;

	case kMouseUp:
		EmitTransaction(kTransactionStageEnd, AbstractViewBar::region);
		return true;

	case kMouseWheel:
		if (auto viewport = GetContainingZoomable(*this))
		{
			g_zoom_state.Init(viewport, *this, e);

			auto delta = GetDelta(e);

			Increment(delta * MakeSize(Data::GetBool(e, kinverted) ? -1.0f : 1.0f), GetModifierKeys(e));

			EmitTransaction(kTransactionStageEnd, AbstractViewBar::region);
		}
		return true;
	}

	return Object::OnEvent(src, e);
}

void ZoomBar::SetFlow(UInt8 flowflags)
{
	GLX::SetFlow(*this, flowflags);
}

void ZoomBar::OnAccommodate(ZoomBar & self, bool & isresponsive, Size & contentsize)
{
	contentsize = self.GetComputedStyle()->GetMinMax().a;

	Detail::AccommodateRenderer(self, isresponsive, contentsize);
}

template <class AXIS> void ZoomBar::OnAlign(ZoomBar & self, bool isresponsive, Float & contenth)
{
	Detail::AlignRenderer(self, contenth);

	auto & size = self.GetRect().size;

	Float pixsize = AXIS::GetSize(size) / self.AbstractViewBar::extent;

	Float x1 = RoundNearest(self.AbstractViewBar::region.start * pixsize);

	Float x2 = RoundNearest(x1 + (self.AbstractViewBar::region.length * pixsize));

	Rect rect = { AXIS::MakePoint(x1), AXIS::MakeSize(x2 - x1, AXIS::Ortho::GetSize(size)) };

	AXIS::SetSize(rect.size, Reflex::Max(AXIS::GetSize(rect.size), self.m_min_size));

	for (auto & i : self) i.SetSize(size);
}

void ZoomBar::Increment(Point drag_inc, UInt8 modifiers)
{
	bool y = GetAxis(*this);


	auto & [vo,vr] = g_zoom_state.region;


	auto inc_x = Detail::GetPoint(!y, drag_inc);

	auto inc_y = Detail::GetPoint(y, drag_inc);

	Float ratio = (Abs(inc_y) + 0.00001f) / (Abs(inc_x) + 0.00001f);

	bool scroll = Abs(ratio) > 2.0f;

	Float size = Detail::GetSize(y, GetRect().size);


	bool from_origin = True(modifiers & GLX::kModifierKeyPrimary);// GLX::CheckModifier(GLX::kCtrl);

	bool lock_scroll = Or(modifiers & GLX::kModifierKeyShift, from_origin);

	bool alt = modifiers & GLX::kModifierKeyAlt;

	bool lock_zoom = alt || And(!lock_scroll, scroll);

	Float zoom_delta = lock_zoom ? 0.0f : inc_x;

	Float zoom_mult = size / Reflex::Max(size + (8.0f * zoom_delta), 1.0f);

	Float vr_delta = Reflex::Max(vr * zoom_mult, g_zoom_state.min_vr) - vr;


	Float zoompos = from_origin ? vo : g_zoom_state.zoompos;

	Float shift = Clip((zoompos - vo) / vr, 0.0f, 1.0f) * vr_delta;


	vr += vr_delta;

	Float scroll_delta = lock_scroll ? 0.0f : ((inc_y / size) * (alt ? g_zoom_state.extent : vr));

	vo = (vo - shift) - scroll_delta;

	g_zoom_state.zoompos = Clip(g_zoom_state.zoompos - scroll_delta, vo, vo + vr);

	EmitTransaction(kTransactionStagePerform, g_zoom_state.region);
}

REFLEX_END_INTERNAL
