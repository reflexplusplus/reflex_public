#include "[require].h"

#include "reflex/glx/widgets/rangebar.h"
#include "reflex/glx/behaviours/gestures.h"
#include "reflex/glx/functions.h"

#include "implementation.h"
#include "coordinates.h"
#include "zoombar.h"
#include "animation_polyfill.h"

#include "../../vm.h"




//
//implementation

REFLEX_NS(Reflex::GLX::Core)
extern void RealignBranch(Object & object);
REFLEX_END

Reflex::FunctionPointer <Reflex::TRef <Reflex::GLX::Animation>(Reflex::Float, Reflex::Float, const Reflex::Function <void(Reflex::GLX::Object&, Reflex::Float)> &, Reflex::Float)> Reflex::GLX::g_create_logarithmic_animation = [](Float from, Float to, const Function <void(Object&, Float)> & callback, Float decay_factor) -> TRef <Animation>
{
	return New<AnimationPolyfill>(callback);
};

Reflex::FunctionPointer <Reflex::TRef <Reflex::GLX::Animation>(const Reflex::Function <void(Reflex::GLX::Object &, Reflex::Float)> &)> Reflex::GLX::g_create_interpolated_animation = [](const Function <void(Object & target, Float x)> & callback) -> TRef <Animation>
{
	return New<AnimationPolyfill>(callback);
};

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct ViewPortImpl::Layout : public GLX::Detail::StandardLayout
{
	Pair <AccommodateFn,AlignFn> OnRebuild(GLX::Object & object, UInt8 flags) override;

	template <bool VIEWPORT_Y, bool ZOOM, bool ENABLE_VIEWBAR_X, bool ENABLE_VIEWBAR_Y> static void OnAlign(AbstractViewPort & object, bool isresponsive, Float & contenth);

	REFLEX_TBINDER_4P(OnAlign);

	AlignFn std_align;
};

template <bool ORIENTATIONX_A, bool ORIENTATIONX_B, bool ORIENTATIONY_A, bool ORIENTATIONY_B> static void FloatBar(const Rect & inner, GLX::Object & item)
{
	Detail::FloatItem<Orientation(Bits<ORIENTATIONX_A, ORIENTATIONX_B>::value), Orientation(Bits<ORIENTATIONY_A, ORIENTATIONY_B>::value)>(inner, item);
}

void StopAnimation(Object & object, Key32 id)
{
	object.UnsetProperty<Animation>(id);
}

REFLEX_TBINDER_4P(FloatBar);

REFLEX_END_INTERNAL

REFLEX_STATIC_ASSERT(sizeof(Reflex::GLX::ViewPortImpl) <= sizeof(Reflex::GLX::AbstractViewPort::m_impl))

const Reflex::GLX::AbstractViewPort::ViewBarCtr Reflex::GLX::AbstractViewPort::kNullBar = [](Reflex::GLX::AbstractViewPort&)
{
	return Null<AbstractViewBar>();
};

const Reflex::GLX::AbstractViewPort::ViewBarCtr Reflex::GLX::AbstractViewPort::kScrollBar = [](Reflex::GLX::AbstractViewPort &) -> Reflex::TRef <Reflex::GLX::AbstractViewBar>
{
	auto range = New<RangeBar>();

	EnableAutoFit(range, true, true);

	return range;
};

const Reflex::GLX::AbstractViewPort::ViewBarCtr Reflex::GLX::AbstractViewPort::kZoomBar = [](Reflex::GLX::AbstractViewPort &) -> Reflex::TRef <Reflex::GLX::AbstractViewBar>
{
	return REFLEX_CREATE(ZoomBar);
};

Reflex::GLX::AbstractViewPort::AbstractViewPort(bool zoom)
	: GLX::Object(New<ViewPortImpl::Layout>())
	, zoomable(zoom)
	, view_state(ViewPortImpl::CreateView(*this, zoom))
{
	VIEWPORT_IMPL(this);

	Reflex::Detail::Constructor<ViewPortImpl>::Construct(m_impl.bytes, *this, zoom);

	Retain(view_state);

	//impl.m_body.ApplyZoomInversion(false, false);

	AddInlineFlex(*this, impl.m_body);

	SetFlow(*this, kFlowY);

	if (kIsMobile)
	{
		SetDelegate(kZeroKey, CreatePanGestureRecognizer(true));
	}
}

Reflex::GLX::AbstractViewPort::~AbstractViewPort()
{
	RemoveConst(view_state->owner) = {};

	Release(view_state);

	VIEWPORT_IMPL(this);

	Reflex::Detail::Constructor<ViewPortImpl>::Destruct(&impl);
}

void Reflex::GLX::AbstractViewPort::InvertAxis(bool x, bool y)
{
	VIEWPORT_IMPL(this);

	impl.m_zoomflags2 = (impl.m_zoomflags2 & 3) | UInt8(MakeBit(2,x) | MakeBit(3, y));

	impl.m_body.ApplyZoomInversion(x, y);

	RemoveConst(inverted) = { x, y };

	RebuildLayout();
}

void Reflex::GLX::AbstractViewPort::SetViewBarCtr(bool y, ViewBarCtr ctr)
{
	REFLEX_ASSERT(ctr);

	VIEWPORT_IMPL(this);

	UInt8 yidx = y;
		
	ViewPortImpl::DiscardViewBar(*this, yidx);

	impl.m_barctrs[yidx] = ctr;

	impl.m_enablebarflags = BitSet(impl.m_enablebarflags, yidx, ctr != kNullBar);

	RebuildLayout();
}

void Reflex::GLX::AbstractViewPort::InvertScrollAxis(bool enable)
{
	VIEWPORT_IMPL(this);

	impl.m_flipaxis = enable;
}

void Reflex::GLX::AbstractViewPort::SetContent(TRef <Object> content, Key32 style_id)
{
	VIEWPORT_IMPL(this);

	auto t = AutoRelease(content);	//could be same content

	impl.m_content->Detach();


	Cast<AbstractCoordinatesImpl>(view_state)->m_content = content;


	impl.m_content = content;

	impl.m_content_style_id = style_id;

	Detail::ApplySubStyle(content, GetStyle(), style_id);


	AddAbsolute(impl.m_body, content);

	content->SendBottom();


	BeginEventForwarding(*this, kKeyDown, content);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::AbstractViewPort::GetContent()
{
	VIEWPORT_IMPL(this);

	return impl.m_content;
}

Reflex::TRef <Reflex::Object> Reflex::GLX::AbstractViewPort::CreateListener(const Function <void()> & callback)
{
	return Cast<AbstractCoordinatesImpl>(view_state)->CreateListener(callback);
}

void Reflex::GLX::AbstractViewPort::StartScroll(bool axis, Float vo)
{
	auto view = GetView();

	Float axis_start = Detail::GetPoint(axis, view.origin);

	Float max_start = Detail::GetSize(axis, GetExtent()) - Detail::GetSize(axis, view.size);

	auto axis_target = Detail::SnapToPixels(Clip(vo, 0.0f, max_start));
	
	RunAnimation(*this, kXY[axis], 0.6f, g_create_logarithmic_animation(axis_start, axis_target, [axis, ortho_axis = !axis](Object & object, Float axis_position)
	{
		REFLEX_ASSERT(IsValid(object));//as attached to self, below check should not be needed

		if (IsValid(object))
		{
			auto viewport = Cast<AbstractViewPort>(object);

			auto view = viewport->GetView();

			Float ortho_position = Detail::GetPoint(ortho_axis, view.origin);

			viewport->SetView({ Detail::MakePoint(axis, axis_position, ortho_position), view.size });
		}
	}, 0.005f));
}

void Reflex::GLX::AbstractViewPort::StopScroll(bool yaxis)
{
	StopAnimation(*this, kXY[yaxis]);
}

void Reflex::GLX::AbstractViewPort::Reveal(bool y, Float offset, Float range, Float padding, bool animate)
{
	Float jump = ViewPortImpl::CalculateReveal(view_state, y, offset, range, padding);

	if (animate)
	{
		StartScroll(y, jump);
	}
	else
	{
		auto view = GetView();

		Detail::SetPoint(y, view.origin, jump);

		SetView(view);
	}
}

void Reflex::GLX::AbstractViewPort::EnableAutoScroll(Float amount, bool scoped)
{
	SetProperty(kNullKey, New<ViewPortImpl::AutoScroll>(this, scoped, amount));
}

void Reflex::GLX::AbstractViewPort::DisableAutoScroll()
{
	UnsetProperty<ViewPortImpl::AutoScroll>(kNullKey);
}

Reflex::ConstTRef <Reflex::GLX::Object> Reflex::GLX::AbstractViewPort::GetBody() const
{
	VIEWPORT_IMPL(this);

	return impl.m_body;
}

Reflex::TRef <Reflex::GLX::AbstractViewBar> Reflex::GLX::AbstractViewPort::GetViewBar(bool y)
{
	VIEWPORT_IMPL(this);

	if (BitCheck(impl.m_enablebarflags, UInt8(y)) /*&& impl.m_barctrs[y] != AbstractViewPort::kNullBar*/)
	{
		ViewPortImpl::ShowViewBar(*this, *this, y);
	}

	REFLEX_ASSERT(impl.m_viewbars[y]);

	return impl.m_viewbars[y];
}

void Reflex::GLX::AbstractViewPort::OnSetStyle(const Style & style)
{
	VIEWPORT_IMPL(this);

	auto cstyle = Detail::Compile<ViewPortImpl::ComputedStyle>(style);

	impl.m_cstyle = cstyle;

	impl.m_body.SetStyle(cstyle->body);

	REFLEX_LOOP(idx, 2)
	{
		if (impl.m_viewbars[idx]) impl.m_viewbars[idx]->SetStyle(*cstyle->bars[idx]);
	}

	Detail::ApplySubStyle(impl.m_content, style, impl.m_content_style_id);
}

bool Reflex::GLX::AbstractViewPort::OnEvent(Object & src, Event & e)
{
	VIEWPORT_IMPL(this);

	if (e.id == kFocus)
	{
		if (&src == this)
		{
			impl.m_content->Focus();

			return true;
		}
	}
	else if (e.id == kMouseWheel)
	{
		auto pdelta = Data::Detail::AcquireProperty<PointProperty>(e, kdelta);

		auto pbars = impl.m_viewbars;

		if (auto weakref = QueryProperty<ViewPortWeakRef>(kmaster))
		{
			auto pmaster = weakref->Load().Adr();

			VIEWPORT_IMPL(pmaster);

			pbars = impl.m_viewbars;
		}

		auto primaryaxis = impl.GetPrimaryAxis(e);

		Point store = pdelta->value;

		pdelta->value.x = 0.0f;

		bool trapped = false;

		REFLEX_LOOP(y, 2)
		{
			auto ty = (y + primaryaxis) & 1;

			pdelta->value.y = Detail::GetPoint(!y, store);

			auto & viewbar = *pbars[ty];

			trapped = Or(trapped, viewbar.ProcessEvent(viewbar, e));

			//if (trapped) break;
		}

		pdelta->value = store;

		Focus();

		return trapped;
	}
	else if (e.id == kPanGesture)
	{
		bool zoomable = BitCheck(impl.m_zoomflags2, 1);

		if (!zoomable)
		{
			REFLEX_LOOP(y, 2) StopAnimation(*this, kXY[y]);

			auto inc = GetDelta(e);

			auto view = GetView();
			
			view.origin -= inc;

			SetView(view);
		}

		return true;
	}
	else if (e.id == kTransaction)
	{
		if (auto viewbar = DynamicCast<AbstractViewBar>(src))
		{
			auto & view = GetView();

			bool y = GetAxis(src);

			if (GetTransactionStage(e) == kTransactionStagePerform)
			{
				bool zoomable = BitCheck(impl.m_zoomflags2, 1);

				StopAnimation(*this, kXY[y]);

				auto prange = e.QueryProperty<Data::Float32Property>(krange);

				auto requested = view;

				if (zoomable && prange)
				{
					Detail::SetSize(y, requested.size, prange->value);
				}

				Detail::SetPoint(y, requested.origin, Data::GetFloat32(e, koffset));

				SetView(requested);
			}

			//FIXED viewbar could have requested invalid vo/vr here, so needs resetting because nothing changed on viewport, so will not be updated in OnAlign

			viewbar->SetRange(Detail::GetSize(y, GetExtent()), { Detail::GetPoint(y, view.origin), Detail::GetSize(y, view.size) });

			return true;
		}
	}
	else if (e.id == RangeBar::kJump)
	{
		bool y = GetAxis(src);

		StartScroll(y, RoundNearest(Data::GetFloat32(e, koffset)));

		return true;
	}
	else if (e.id == kKeyDown)
	{
		bool zoomable = BitCheck(impl.m_zoomflags2, 1);

		bool y = impl.GetPrimaryAxis(e);

		auto viewport_rect = GetView();

		Float vo = Detail::GetPoint(y, viewport_rect.origin);

		Float vr = Detail::GetSize(y, viewport_rect.size);

		Float page = zoomable ? vr * 0.5f : vr + 1.0f;

		Float range = Detail::GetSize(y, view_state->GetExtent());

		Float dir = (BitCheck(impl.m_zoomflags2, 1) && BitCheck(impl.m_zoomflags2, 2 + y)) ? -1.0f : 1.0f;

		switch (GetKeyCode(e))
		{
		case kKeyCodePageUp:
			StartScroll(y, vo - (page * dir));
			return true;

		case kKeyCodePageDown:
			StartScroll(y, vo + (page * dir));
			return true;

		case kKeyCodeHome:
			StartScroll(y, vo - (range * dir));
			return true;

		case kKeyCodeEnd:
			StartScroll(y, vo + (range * dir));
			return true;
		}
	}

	return GLX::Object::OnEvent(src, e);
}

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

Pair <Core::Object::AccommodateFn,Core::Object::AlignFn> ViewPortImpl::Layout::OnRebuild(GLX::Object & object, UInt8 layout_flags)
{
	auto viewport = Cast<AbstractViewPort>(object);

	VIEWPORT_IMPL(viewport.Adr());

	impl.m_body.RebuildLayout();

	//auto & layoutflags = RemoveConst(viewport->GetLayoutFlags());

	layout_flags &= UInt8(~((1 << Detail::kStandardLayoutCenter) | (1 << Detail::kStandardLayoutWrap)));

	auto base = StandardLayout::OnRebuild(object, layout_flags);

	UInt8 flags = (layout_flags & 1) | (impl.m_zoomflags2 & 3) | (impl.m_enablebarflags << 2);

	std_align = base.b;
	
	base.b = CastAlignFn<AbstractViewPort>(OnAlignBinder::Bind(flags));

	return base;
}

Pair <Core::Object::AccommodateFn,Core::Object::AlignFn> ViewPortImpl::Body::Layout::OnRebuild(GLX::Object & object, UInt8 layout_flags)
{
	auto viewport = Cast<AbstractViewPort>(object.GetParent());

	VIEWPORT_IMPL(viewport);

	auto base = StandardLayout::OnRebuild(object, layout_flags);

	auto viewport_layout_flags = viewport->GetLayoutFlags();

	UInt8 flags = (viewport_layout_flags & 1) | (impl.m_zoomflags2 & 3) | (impl.m_enablebarflags << 2);

	base.b = CastAlignFn<Body>(OnAlignBinder::Bind(flags));

	return base;
}

ViewPortImpl::ViewPortImpl(AbstractViewPort & viewport, bool zoom)
	: m_cstyle(Detail::Compile<ComputedStyle>(Style::null))
	, m_zoomflags2(MakeBits(0, zoom, false, false))
	, m_flipaxis(0)
	, m_enablebarflags(Bits<true,true>::value)
	, m_forcebarflags(0)
	, m_body(viewport, *this)
{
	EnableAutoFit(viewport, false, false);

	EnableAutoFit(m_body, !zoom, !zoom);

	EnableMouse(m_body, false);
}

REFLEX_INLINE AbstractViewPort::ViewState * ViewPortImpl::CreateView(AbstractViewPort & self, bool zoom)
{
	VIEWPORT_IMPL(&self);

	if (zoom)
	{
		REFLEX_LOOP(idx, 2) impl.m_barctrs[idx] = AbstractViewPort::kZoomBar;

		return REFLEX_CREATE(CoordinatesImpl<true>, self);
	}
	else
	{
		REFLEX_LOOP(idx, 2) impl.m_barctrs[idx] = AbstractViewPort::kScrollBar;

		return REFLEX_CREATE(CoordinatesImpl<false>, self);
	}
}

GLX::AbstractViewBar & ViewPortImpl::ShowViewBar(AbstractViewPort & self, GLX::Object & parent, UInt8 y)
{
	VIEWPORT_IMPL(&self);

	auto & ref = impl.m_viewbars[y];

	if (!ref)
	{
		auto ctr = impl.m_barctrs[y];

		auto bar = ctr(self);

		ref = bar;

		bar->id = kXY[y];

		auto & cstyle = *impl.m_cstyle;

		bar->SetStyle(*cstyle.bars[y]);

		bar->SetParent(parent);

		EnableInline<false>(bar, kOrientationFit);	//this only affects inline ortho bar

		bool invert = BitCheck(impl.m_zoomflags2, 2 + y);

		bar->SetFlow(FlowFlags(MakeBits(y, invert)));
	}

	return ref;
}

void ViewPortImpl::DiscardViewBar(AbstractViewPort & self, UInt8 y)
{
	VIEWPORT_IMPL(&self);

	auto & ref = impl.m_viewbars[y];

	if (ref)
	{
		ref->Detach();

		ref.Clear();
	}
}

template <bool VIEWPORT_Y, bool ZOOM, bool ENABLE_VIEWBAR_X, bool ENABLE_VIEWBAR_Y> void ViewPortImpl::Layout::OnAlign(AbstractViewPort & self, bool isresponsive, Float & contenth)
{
	typedef ConditionalType <VIEWPORT_Y,Detail::YAxis,Detail::XAxis> Axis;

	typedef Bits<VIEWPORT_Y ? ENABLE_VIEWBAR_Y : ENABLE_VIEWBAR_X> EnableAxisBar;
	typedef Bits<VIEWPORT_Y ? ENABLE_VIEWBAR_X : ENABLE_VIEWBAR_Y> EnableOrthoBar;
	
	typedef typename Axis::Ortho Ortho;

	VIEWPORT_IMPL(&self);

	auto layout = Cast<Layout>(self.GetLayoutModel().RemoveConst());

	auto & size = self.GetRect().size;

	auto & body = impl.m_body;

	auto & content = *impl.m_content;

	body.ComputeLayout();

	auto cstyle = self.GetComputedStyle();

	auto body_cstyle = body.GetComputedStyle();

	auto body_margin = Sum(body_cstyle->GetMargin());

	auto body_padding = Sum(body_cstyle->GetPadding());

	auto content_margin = Sum(content.GetComputedStyle()->GetMargin());


	//recalc for flex (because bar may be added/removed)

	layout->inline_size = Axis::GetSize(content_margin) + Axis::GetSize(body_padding) + Axis::GetSize(body_margin);



	if constexpr (ZOOM)
	{
		if constexpr (EnableOrthoBar::value)	//ortho is priority because affects inner size
		{
			auto & bar = ShowViewBar(self, self, Ortho::kY);
			
			auto barsize = GetInlineSize<Axis>(bar);

			layout->inline_size += barsize;
		}
		else
		{
			DiscardViewBar(self, Ortho::kY);
		}

		if constexpr (EnableAxisBar::value)
		{
			auto & bar = ShowViewBar(self, impl.m_body, Axis::kY);

			EnableFloat(bar, Axis::kY ? kOrientationFar : kOrientationFit, Axis::kY ? kOrientationFit : kOrientationFar);
		}
		else
		{
			DiscardViewBar(self, Axis::kY);
		}
	}
	else
	{
		auto & contentsize = content.contentsize;

		layout->inline_size += Axis::GetSize(contentsize);
		
		auto padding = Sum(cstyle->GetPadding());

		auto outdent = padding + body_margin + body_padding + content_margin;

		auto viewable = size - outdent;

		if constexpr (EnableOrthoBar::value)
		{
			if ((Ortho::GetSize(contentsize) - Ortho::GetSize(viewable)) > 0.999f)
			{
				auto & bar = ShowViewBar(self, self, Ortho::kY);

				EnableMouse(bar, true);

				auto barsize = GetInlineSize<Axis>(bar);

				layout->inline_size += barsize;

				Axis::IncSize(viewable, -barsize);
			}
			else
			{
				DiscardViewBar(self, Ortho::kY);
			}
		}
		else
		{
			DiscardViewBar(self, Ortho::kY);
		}

		if constexpr (EnableAxisBar::value)
		{
			if ((Axis::GetSize(contentsize) - Axis::GetSize(viewable)) > 0.999f)
			{
				auto & bar = ShowViewBar(self, body, Axis::kY);

				EnableFloat(bar, Axis::kY ? kOrientationFar : kOrientationFit, Axis::kY ? kOrientationFit : kOrientationFar);
			}
			else
			{
				DiscardViewBar(self, Axis::kY);
			}
		}
		else
		{
			DiscardViewBar(self, Axis::kY);
		}

		auto data = self.view_state;

		data->SetView({ data->GetView().origin, viewable });
	}

	layout->std_align(self, isresponsive, contenth);
}

ViewPortImpl::Body::Body(AbstractViewPort & viewport, ViewPortImpl & impl)
	: GLX::Object(impl.m_body_layout)
	, viewport(viewport)
	, m_invert_shift_mult({0.0f,0.0f})
	, m_invert_mult(kNormal)
{
}

void ViewPortImpl::Body::ApplyZoomInversion(bool x, bool y)
{
	constexpr Scale shift_mult[4] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };

	constexpr Scale mult[4] = { kNormal, { -1.0f, 1.0f }, { 1.0f, -1.0f }, { -1.0f, -1.0f } };

	auto flags = MakeBits(x, y);

	m_invert_shift_mult = shift_mult[flags];

	m_invert_mult = mult[flags];
}

template <bool VIEWPORT_Y, bool ZOOM, bool ENABLE_VIEWBAR_X, bool ENABLE_VIEWBAR_Y> void ViewPortImpl::Body::OnAlign(Body & body, bool isresponsive, Float & contenth)
{
	typedef Bits<VIEWPORT_Y ? ENABLE_VIEWBAR_Y : ENABLE_VIEWBAR_X> EnableAxisBar;


	//auto layout = Cast<Layout>(body.GetLayoutModel().RemoveConst());

	auto & viewport = body.viewport;

	VIEWPORT_IMPL(&viewport);


	auto content = viewport.GetContent();

	auto data = Cast<AbstractCoordinatesImpl>(viewport.view_state);

	auto range = data->GetExtent();

	auto [vo,vr] = data->GetView();


	auto pbars = impl.m_viewbars;

	if constexpr (ENABLE_VIEWBAR_X) pbars[0]->SetRange(range.w, { vo.x, vr.w });

	if constexpr (ENABLE_VIEWBAR_Y) pbars[1]->SetRange(range.h, { vo.y, vr.h });


	//mode specific

	alignas(16) auto padding = body.GetComputedStyle()->GetPadding();
	
	alignas(16) auto margin = content->GetComputedStyle()->GetMargin();

	auto outdent = Reinterpret<Margin>(Reinterpret<SIMD::FloatV4>(padding) + Reinterpret<SIMD::FloatV4>(margin));

	const auto inner_rect = Indent(body.GetRect().size, outdent);

	data->SetViewSize(inner_rect.size);

	if constexpr (ZOOM)
	{
		Point offset = inner_rect.origin;

		Scale scale = data->m_pixelscale;

		offset += Reinterpret<Point>(inner_rect.size * body.m_invert_shift_mult);

		scale *= body.m_invert_mult;

		Rect rect = { (-vo * scale) + offset, range };

		content->SetScale(scale);

		content->SetRect(rect);
	}
	else
	{
		Rect rect = { inner_rect.origin - vo, Max(range, inner_rect.size) };

		content->SetRect(rect);
	}

	content->Realign();

	if constexpr (EnableAxisBar::value)
	{
		auto & axisbar = *pbars[VIEWPORT_Y];

		if (IsValid(axisbar))	///without -> recursive crash
		{
			auto floatfn = FloatBarBinder::Bind(axisbar.GetPositioningFlags() >> 2);

			floatfn(inner_rect, axisbar);
		}
	}


	//standard

	Detail::AlignRenderer(body, contenth);
}

Float ViewPortImpl::CalculateReveal(Float range, Float vo, Float vr, Float itemposition, Float itemsize, Float padding)
{
	bool t = itemposition > vo;

	if (t && ((itemposition + itemsize) < (vo + vr)))
	{
		return vo;
	}
	else if (t)
	{
		itemposition -= vr;

		itemposition += Reflex::Min(itemsize, vr - padding);

		itemposition += padding;
	}
	else
	{
		itemposition -= padding;
	}

	itemposition = Clip(itemposition, 0.0f, range - vr);

	return itemposition;
}

Float ViewPortImpl::CalculateReveal(AbstractViewPort::ViewState & view, bool y, Float itemposition, Float itemsize, Float padding)
{
	Float range = Detail::GetSize(y, view.GetExtent());

	auto [vo, vr] = view.GetView();

	Float axisvr = Detail::GetSize(y, vr);

	Float axisvo = Detail::GetPoint(y, vo);

	return CalculateReveal(range, axisvo, axisvr, itemposition, itemsize, padding);
}

REFLEX_END_INTERNAL
