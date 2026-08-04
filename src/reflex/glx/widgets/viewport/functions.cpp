#include "implementation.h"
#include "animation_polyfill.h"





//
//

void Reflex::GLX::SyncViewports(AbstractViewPort & a, AbstractViewPort & b, bool x, bool y)
{
	auto master = Make<ViewPortWeakRef>(a);

	b.SetProperty(kmaster, master);

	SetAbstractProperty(b, kmaster, a.CreateListener([master, &self = b, x, y]()
	{
		auto from = master->Load();

		auto range_from = Detail::ComputeContentSize(from->GetContent());

		auto content = self.GetContent();

		auto range_to = Detail::ComputeContentSize(content);

		auto [vo_from, vr_from] = from->GetView();

		auto [vo_to, vr_to] = self.GetView();

		Size range = { x ? range_from.w : range_to.w, y ? range_from.h : range_to.h };

		SetBounds(content, K32("self"), range, kLarge);

		Point vo = { x ? vo_from.x : vo_to.x, y ? vo_from.y : vo_to.y };

		Size vr = { x ? vr_from.w : vr_to.w, y ? vr_from.h : vr_to.h };

		self.SetView({ vo, vr });
	}));
}

void Reflex::GLX::UnsyncViewports(AbstractViewPort & b)
{
	b.UnsetProperty<ViewPortWeakRef>(kmaster);

	UnsetAbstractProperty(b, kmaster);
}

void Reflex::GLX::Zoom(ZoomArea & viewport, bool y, Float time, Float vo, Float vr, Float pixel_padding)
{
	auto [from_vo, from_vr] = viewport.GetView();

	auto size_pixels = Reinterpret<Size>(from_vr / viewport.GetPixelsPerUnit());

	Float size = Detail::GetSize(y, size_pixels);

	Float range = Detail::GetSize(y, viewport.GetExtent());

	Float pad = (pixel_padding / size) * vr;

	vo -= pad;

	vr += pad;

	vr += pad;

	vr = Min(range, vr);

	Pair <Float> from = { Detail::GetPoint(y, from_vo), Detail::GetSize(y, from_vr) };

	vo = Min(range - vr, vo);

	Pair <Float> to = { vo, vr };

	RunAnimation(viewport, kXY[y], time, g_create_interpolated_animation([y, from, to](Object & object, Float x)
	{
		if (auto viewport = DynamicCast<AbstractViewPort>(object))
		{
			auto [vo, vr] = viewport->GetView();

			Detail::SetPoint(y, vo, LinearInterpolate(x, from.a, to.a));

			Detail::SetSize(y, vr, LinearInterpolate(x, from.b, to.b));

			viewport->SetView({ vo, vr });
		}
	}));
}

void Reflex::GLX::Reveal(ZoomArea & viewport, bool y, Float vo_new, Float vr_new, Float pixel_padding)
{
	Float range = Detail::GetSize(y, viewport.GetExtent());

	auto [vo, vr] = viewport.GetView();

	Float axis_vo = Detail::GetPoint(y, vo);

	Float axis_vr = Detail::GetSize(y, vr);

	auto size_pixels = Reinterpret<Size>(vr / viewport.GetPixelsPerUnit());

	Float size = Detail::GetSize(y, size_pixels);

	if (vr_new <= axis_vr)
	{
		Float end = vo_new + vr_new;

		if (Reflex::Inside(vo_new, axis_vo, axis_vr) && Reflex::Inside(end, axis_vo, axis_vr)) return;

		Float pad = (pixel_padding / size) * axis_vr;

		Float pos = ViewPortImpl::CalculateReveal(range, axis_vo, axis_vr, vo_new, vr_new, pad);

		Zoom(viewport, y, 0.25f, pos, axis_vr, 0.0f);
	}
	else
	{
		Zoom(viewport, y, 0.25f, vo_new, vr_new, pixel_padding);
	}
}


void Reflex::GLX::Detail::ClearZoomToggle(ZoomArea & viewport, AbstractViewBar & viewbar)
{
	viewbar.UnsetProperty<RangeProperty>(K32("zoom_toggle"));
}

void Reflex::GLX::Detail::ToggleZoom(ZoomArea & viewport, AbstractViewBar & viewbar, bool y)
{
	auto extent = viewbar.extent;

	auto toggleref = Data::Detail::AcquireProperty<RangeProperty>(viewbar, K32("zoom_toggle"), extent * 0.25f, extent * 0.5f);

	auto & toggle = toggleref->value;

	if (viewbar.region.length == extent)
	{
		Zoom(viewport, y, 0.25f, toggle.start, toggle.length);
	}
	else
	{
		toggle = viewbar.region;

		Zoom(viewport, y, 0.25f, 0.0f, extent);
	}
}
