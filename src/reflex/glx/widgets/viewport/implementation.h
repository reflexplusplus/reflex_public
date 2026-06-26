#pragma once

#include "[require].h"
#include "../../standard_layout.h"




//
//Internal

#define VIEWPORT_IMPL(pthis) auto & impl = reinterpret_cast<ViewPortImpl&>(RemoveConst((pthis)->m_impl));

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct ViewPortImpl
{
	struct ComputedStyle;

	struct Layout;

	struct Body : public Object
	{
		struct Layout : public Detail::StandardLayout
		{
			Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 flags) override;
		};

	
		REFLEX_OBJECT(ViewPortImpl::Body, Object);

		Body(AbstractViewPort & viewport, ViewPortImpl & impl);

		void ApplyZoomInversion(bool x, bool y);


		template <bool VIEWPORT_Y, bool ZOOM, bool ENABLE_VIEWBAR_X, bool ENABLE_VIEWBAR_Y> static void OnAlign(Body & body, bool responsive, Float & contenth);

		REFLEX_TBINDER_4P(OnAlign);


		AbstractViewPort & viewport;

		Scale m_invert_shift_mult, m_invert_mult;
	};

	struct AutoScroll;


	static AbstractViewPort::ViewState * CreateView(AbstractViewPort & self, bool zoom);


	template <class AXIS> static Float GetInlineSize(Object & item)
	{
		item.ComputeLayout();

		return AXIS::GetSize(item.contentsize) + Detail::AxisSum<AXIS>(item.GetComputedStyle()->GetMargin());
	}


	static TRef <AbstractViewBar> CreateNullBar(AbstractViewPort & self)
	{
		return AbstractViewBar::null;
	}


	static GLX::AbstractViewBar & ShowViewBar(AbstractViewPort & self, GLX::Object & parent, UInt8 y);

	static void DiscardViewBar(AbstractViewPort & self, UInt8 y);


	static Float CalculateReveal(Float range, Float vo, Float vr, Float itemposition, Float itemsize, Float padding);

	static Float CalculateReveal(AbstractViewPort::ViewState & view, bool y, Float itemposition, Float itemsize, Float padding);


	ViewPortImpl(AbstractViewPort & viewport, bool zoom);

	UInt8 GetPrimaryAxis(const Event & e) const
	{
		UInt8 shift = GetModifierKeys(e) & kModifierKeyShift;

		return UInt8(GetAxis(m_content) + m_flipaxis + shift) & 1;
	}


	ConstReference <ComputedStyle> m_cstyle;


	UInt8 m_zoomflags2;

	UInt8 m_flipaxis;

	UInt8 m_enablebarflags;

	UInt8 m_forcebarflags;

	Key32 m_content_style_id;


	Body::Layout m_body_layout;

	Body m_body;

	Reference <GLX::Object> m_content;


	AbstractViewPort::ViewBarCtr m_barctrs[2];	//TODO single ctr with axis param

	Reference <AbstractViewBar> m_viewbars[2];

};

struct ViewPortImpl::ComputedStyle : public Reflex::Object
{
	ComputedStyle(const Style & style);

	const Style & body;

	const Style * bars[2];
};

struct ViewPortImpl::AutoScroll : public Reflex::Object
{
	AutoScroll(AbstractViewPort * scroller, bool scoped, Float amount);

	void OnClock(Float delta);

	void SetEdge(Alignment edge);


	AbstractViewPort & scroller;

	Reference <Reflex::Object> m_onclock;

	Float m_amount;

	Alignment m_edge;

	Float m_remainder;

	bool m_scoped;


	static const Float kSensitivity;

	static const Float kInterval;
};

using ViewPortWeakRef = Reflex::Detail::WeakRef<AbstractViewPort>;

REFLEX_DECLARE_KEY32(master);

REFLEX_END_INTERNAL
