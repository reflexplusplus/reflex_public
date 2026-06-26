#pragma once

#include "implementation.h"




//
//declarations

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct NullViewPortCoordinates : public AbstractViewPort::ViewState
{
	using ViewState::ViewState;
	
	virtual void SetMinView(const Size & size) override {}
	Size GetMinView() const override { return kNormal; }
	virtual Size GetExtent() const override { return kLarge; }
	virtual void SetView(const Rect &) override {}
	const Rect & GetView() const override { return m_view; };
	virtual const Size & GetPixelsPerUnit() const override { return kNormal; }

	const Rect m_view = { kOrigin, kLarge };
};

struct AbstractCoordinatesImpl : public AbstractViewPort::ViewState
{
	struct NotificationScope;

	AbstractCoordinatesImpl(AbstractViewPort & owner);


	using ViewState::CreateListener;


	virtual Size GetMinView() const override;
	
	const Rect & GetView() const override;
	
	virtual void SetViewSize(const Size & size) = 0;	//internal
	
	virtual const Size & GetPixelsPerUnit() const override;


	template <bool DEFER> static void DoNotify(AbstractCoordinatesImpl & self);


	Core::WeakReference m_content;

	Point m_origin;
	Size m_range;
	Size m_min;
	Rect m_view;
	Size m_size;
	Size m_pixelsize;
	Scale m_pixelscale;

	FunctionPointer <void(AbstractCoordinatesImpl&)> m_notifyfn;

	UInt8 m_notificationscopes = 0;

	bool m_notify;
};

template <bool ZOOM>
struct CoordinatesImpl : public AbstractCoordinatesImpl
{
	using AbstractCoordinatesImpl::AbstractCoordinatesImpl;

	virtual Size GetExtent() const override
	{
		RemoveConst(this)->UpdateRange();

		return m_range;
	}

	virtual void SetMinView(const Size & size) override;

	virtual void SetView(const Rect & view) override;

	virtual void SetViewSize(const Size & size) override;


	void SetViewOffset(const Point & vo);

	void SetViewRange(const Size & vr);

	void UpdatePixelSize();

	void UpdateRange();
};

REFLEX_END_INTERNAL
