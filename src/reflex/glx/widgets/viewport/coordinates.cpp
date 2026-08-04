#include "coordinates.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct AbstractCoordinatesImpl::NotificationScope
{
	REFLEX_INLINE NotificationScope(AbstractCoordinatesImpl & coordinates)
		: coordinates(coordinates),
		ref(coordinates)
	{
		REFLEX_ASSERT(coordinates.GetRetainCount() > 1);

		coordinates.m_notificationscopes++;

		coordinates.m_notifyfn = &AbstractCoordinatesImpl::DoNotify<true>;
	}

	REFLEX_INLINE ~NotificationScope()
	{
		if (!--coordinates.m_notificationscopes)
		{
			if (coordinates.m_notify) AbstractCoordinatesImpl::DoNotify<false>(coordinates);

			coordinates.m_notifyfn = &AbstractCoordinatesImpl::DoNotify<false>;
		}
	}

	AbstractCoordinatesImpl & coordinates;

	Reference <ViewState> ref;
};

AbstractCoordinatesImpl::AbstractCoordinatesImpl(AbstractViewPort & owner)
	: ViewState(owner),
	m_range(kNormal),
	m_min({ Core::kRoundingTolerance, Core::kRoundingTolerance }),
	m_view({ kOrigin, kNormal }),
	m_size(kNormal),
	m_pixelsize(MakeSize(Detail::kPixelSize)),
	m_pixelscale(kNormal),
	m_notifyfn(&AbstractCoordinatesImpl::DoNotify<false>),
	m_notify(false)
{
}

const Rect & AbstractCoordinatesImpl::GetView() const
{
	return m_view;
}

Size AbstractCoordinatesImpl::GetMinView() const
{
	return m_min;
}

//const Size & AbstractCoordinatesImpl::GetViewSize() const
//{
//	return m_size;
//}

const Size & AbstractCoordinatesImpl::GetPixelsPerUnit() const
{
	return m_pixelsize;
}

template <bool DEFER> REFLEX_INLINE void AbstractCoordinatesImpl::DoNotify(AbstractCoordinatesImpl & self)
{
	if constexpr (DEFER)
	{
		self.m_notify = true;
	}
	else
	{
		auto & content = *self.m_content;

		content.GetParent()->Realign();	//why this doesnt work?

		self.m_notify = false;

		self.State::Notify();

		self.Signal::Notify();

		self.m_notify = false;
	}
}

template <bool ZOOM> void CoordinatesImpl<ZOOM>::SetMinView(const Size & size)
{
	REFLEX_ASSERT(ZOOM);

	if constexpr (ZOOM)
	{
		REFLEX_ASSERT(size.w * size.h);

		m_min = Max(MakeSize(Core::kRoundingTolerance), size);
	}
}

template <bool ZOOM> void CoordinatesImpl<ZOOM>::SetViewOffset(const Point & vo)
{
	NotificationScope scope(*this);

	UpdateRange();

	auto & [m_vo,m_vr] = m_view;

	if (SetFiltered(m_vo, Clip(ZOOM ? vo : Detail::Snap(vo, m_pixelsize), kOrigin, Reinterpret<Point>(m_range - m_vr))))
	{
		m_notifyfn(*this);
	}
}

template <bool ZOOM> void CoordinatesImpl<ZOOM>::SetView(const Rect & view)
{
	NotificationScope scope(*this);

	SetViewRange(view.size);

	SetViewOffset(view.origin);
}

template <bool ZOOM> void CoordinatesImpl<ZOOM>::SetViewSize(const Size & size)
{
	Size max = { size.w ? size.w : m_size.w, size.h ? size.h : m_size.h };

	if (SetFiltered(m_size, max))
	{
		UpdatePixelSize();

		m_notifyfn(*this);
	}
}

template <bool ZOOM> void CoordinatesImpl<ZOOM>::SetViewRange(const Size & vr)
{
	auto & [m_vo, m_vr] = m_view;

	NotificationScope scope(*this);

	UpdateRange();

	if constexpr (ZOOM)
	{
		if (SetFiltered(m_vr, Clip(vr, m_min, m_range)))
		{
			UpdatePixelSize();

			SetViewOffset(m_vo);

			m_notifyfn(*this);
		}
	}
	else
	{
		if (SetFiltered(m_vr, Clip(vr, kNormal, m_range)))
		{
			UpdatePixelSize();

			SetViewOffset(m_vo);

			m_notifyfn(*this);
		}
	}

	REFLEX_ASSERT(m_vr.w * m_vr.h);
}

template <bool ZOOM> REFLEX_INLINE void CoordinatesImpl<ZOOM>::UpdatePixelSize()
{
	if constexpr (ZOOM)
	{
		auto & [m_vo, m_vr] = m_view;

		m_pixelscale = m_size / m_vr;

		m_pixelsize = Reinterpret<Size>(Reciprocal(m_pixelscale));
	}
}

template <bool ZOOM> REFLEX_INLINE void CoordinatesImpl<ZOOM>::UpdateRange()
{
	auto & [m_vo, m_vr] = m_view;

	auto & content = *m_content;

	auto prev = m_range;

	if (SetFiltered(m_range, Max(m_min, Detail::ComputeContentSize(content))))
	{
		if constexpr (ZOOM)
		{
			auto adjust = (m_vr / prev) * m_range;

			SetViewRange(adjust);
		}
		else
		{
			SetViewRange(m_vr);
		}

		m_notifyfn(*this);
	}
}

REFLEX_END_INTERNAL

Reflex::GLX::AbstractViewPort::ViewState::ViewState(AbstractViewPort & owner)
	: owner(owner)
{
}
