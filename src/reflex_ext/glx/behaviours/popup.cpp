#include "common.h"
#include "../../../../include/reflex_ext/glx/behaviours/popup.h"
#include "../../../../include/reflex_ext/glx/functions/overlay.h"




//
//cstyle

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

REFLEX_DECLARE_KEY32(popup);

using WeakRef = Reflex::Detail::WeakRef <GLX::Object>;

struct PopupBehaviourImpl : public PopupBehaviour
{
	PopupBehaviourImpl()
		: m_content_ctr(&Detail::CreateMenuContent)
		, m_forward_event(Menu::kMenuOpen)
		, m_content_style(kmenu)
	{
	}

	void SetConfig(FunctionPointer <TRef<GLX::Object>()> create_content, Key32 forward_event, Key32 content_style) override
	{
		m_content_ctr = create_content;
		m_forward_event = forward_event;
		m_content_style = content_style;
	}

	GLX::Object * Open() override
	{
		if (auto existing = Detail::QueryPopup(object))
		{
			return existing;
		}
		else
		{
			auto popup = m_content_ctr();

			SetEventDelegate(popup, kpopup, [target = Core::WeakReference(object), forward_event = m_forward_event](GLX::Object & src, Event & e)
			{
				if (e.id == forward_event)
				{
					target->Emit(e);
					
					return true;
				}

				return false;
			});

			
			auto style = object->GetStyle();

			Detail::ApplySubStyle(popup, style, m_content_style);

			if (!style->QuerySubStyle(m_content_style) && style->QuerySubStyle(kcontent))
			{
				output.Warn("Popup 'content' sub-style is no longer the default");
			}


			auto window = object->GetWindow();

			auto target_rect = CalculateAbsoluteRect(object);

			target_rect = Indent(target_rect, object->GetComputedStyle()->GetPadding());


			auto align = Detail::ParseAlignment(Data::GetKey32(style, kalign), kAlignmentBottom);
			
			auto justify = Detail::ParseOrientation(Data::GetKey32(style, K32("justify")), kOrientationFit);

			Detail::PlacePopup(object, window->GetForeground(), popup, target_rect, align, justify);

			object->SetState(kActiveState);

			struct DetachDelegate : public GLX::Object::Delegate
			{
				void OnDetachWindow() override
				{
					if (auto popup = object->QueryProperty<WeakRef>(kowner))
					{
						popup->Load()->ClearState(kActiveState);
					}
				}
			};

			popup->SetDelegate(kpopup, New<DetachDelegate>());

			return popup.Adr();
		}
	}

	void OnAttachObject() override
	{
		object->SetMouseCursor(kMouseCursorPointer);
	}

	bool OnEvent(GLX::Object & src, Event & e) override
	{
		if (e.id == kMouseDown)
		{
			if (Not(GetClickFlags(e) & kClickFlagRmb))
			{
				bool is_open = Detail::QueryPopup(object);

				EnableMouseCapture(object, !is_open);
			}

			return true;
		}
		else if (e.id == kMouseDrag)
		{
			if (ExceedsDragThreshold(GetMouseDelta(e)) && Data::GetBool(object, K32("allow_drag_open"), true))
			{
				if (auto content = Open())
				{
					if (auto menu = DynamicCast<Menu>(*content))
					{
						StartDragDrop(REFLEX_CREATE(ObjectOf<Menu*>, menu), kMouseCursorArrow, kMouseCursorArrow);
					}
				}
			}

			return true;
		}
		else if (e.id == kMouseUp)
		{
			Open();

			EnableMouseCapture(object, false);
		}
		else if (e.id == kKeyDown)
		{
			auto keycode = GetKeyCode(e);

			if (keycode == kKeyCodeEnter || keycode == kKeyCodeDown)
			{
				Open();

				return true;
			}
		}

		return false;
	}


	decltype (&Detail::CreateMenuContent) m_content_ctr;

	Key32 m_forward_event, m_content_style;
};

Float PlacePopupJustify(Float target_origin, Float target_size, Float child_size, Orientation justify)
{
	switch (justify)
	{
	case kOrientationNear:
		return target_origin;

	case kOrientationCenter:
	case kOrientationFit:
		return target_origin + ((target_size - child_size) * 0.5f);

	case kOrientationFar:
		return target_origin + target_size - child_size;
	}

	return target_origin;
}

REFLEX_INLINE bool PlacePopupFits(const Rect & bounds, const Rect & rect, UInt constrain_flags)
{
	auto far0 = bounds.origin + Reinterpret<Point>(bounds.size);
	auto far1 = rect.origin + Reinterpret<Point>(rect.size);

	bool fits_x = rect.origin.x >= bounds.origin.x && far1.x <= far0.x;
	bool fits_y = rect.origin.y >= bounds.origin.y && far1.y <= far0.y;

	switch (constrain_flags)
	{
	case 1:  
		return fits_x;

	case 2:  
		return fits_y;

	case 3:  
		return fits_x && fits_y;

	case 0:  
		return true;
	}

	return false;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::PopupBehaviour> Reflex::GLX::PopupBehaviour::Create()
{
	return New<PopupBehaviourImpl>();
}

void Reflex::GLX::Detail::PlacePopup(Object & owner, Object & fg, TRef <Object> popup, const Rect & target_rect, Alignment alignment, Orientation justify)
{
	struct DetachDelegate : public GLX::Object::Delegate
	{
		DetachDelegate(GLX::Object & owner)
			: m_owner(New<WeakRef>(owner))
			, m_previous_focus(Core::desktop->GetFocus())
		{
		}

		void OnAttachWindow() override
		{
			m_onfocus = Core::desktop->CreateListener(Core::Desktop::kNotificationFocus, [this]()
			{
				if (!PopupHasFocus(object, Core::desktop->GetFocus())) object->Detach();
			});
		}

		void OnDetachWindow() override
		{
			m_onfocus = CreateAnimationClock([this](Float32)
			{
				auto & owner = *m_owner->Load();

				m_onfocus.Clear();

				owner.UnsetProperty<GLX::Object>(kpopup);
			});

			auto & previous_focus = *m_previous_focus.Load();

			m_previous_focus.Clear();

			previous_focus.Focus();
		}

		
		Reference <WeakRef> m_owner;
		
		WeakRef m_previous_focus;

		Reference <AbstractProperty> m_onfocus;
	};

	auto attach_delegate = New<DetachDelegate>(owner);

	popup->SetDelegate("place_popup", attach_delegate);

	popup->EnableOnAttachDetachWindow();

	owner.SetProperty(kpopup, popup);

	popup->SetProperty(kowner, attach_delegate->m_owner);

	popup->Focus();

	AddAbsolute(fg, popup);

	auto content_size = Detail::ComputeContentSize(popup);

	popup->SetRect({ kOrigin, content_size });

	content_size = Detail::ComputeContentSize(popup);

	auto cstyle = popup->GetComputedStyle();

	auto & parent_padding = fg.GetComputedStyle()->GetPadding();

	auto & margin = cstyle->GetMargin();

	auto bounds = Indent(fg.GetRect().size, { Max(parent_padding.near, margin.near), Max(parent_padding.far, margin.far) });

	auto min_size = kNormal;

	if (justify == kOrientationFit)
	{
		auto [min, max] = cstyle->GetMinMax();

		auto fit = Min(Max(content_size, target_rect.size), max);

		switch (alignment)
		{
		case kAlignmentTop:
		case kAlignmentBottom:
			content_size.w = fit.w;
			break;

		case kAlignmentLeft:
		case kAlignmentRight:
			content_size.h = fit.h;
			break;

		case kAlignmentCenter:
			content_size.w = fit.w;
			content_size.h = fit.h;
			break;
		}

		min_size = content_size;
	}

	SetBounds(popup, "place_popup", min_size, bounds.size);

	Rect rect = { {}, content_size };

	UInt8 constrain_flags = 3;

	REFLEX_LOOP(idx, 3)
	{
		switch (alignment)
		{
		case kAlignmentTopLeft:
			rect.origin = { target_rect.origin.x - rect.size.w - margin.far.w, target_rect.origin.y - rect.size.h - margin.far.h };
			break;

		case kAlignmentTop:
			rect.origin = { PlacePopupJustify(target_rect.origin.x, target_rect.size.w, rect.size.w, justify), target_rect.origin.y - rect.size.h - margin.far.h };
			constrain_flags = 2;
			break;

		case kAlignmentTopRight:
			rect.origin = { target_rect.origin.x + target_rect.size.w + margin.near.w, target_rect.origin.y - rect.size.h - margin.far.h };
			break;

		case kAlignmentLeft:
			rect.origin = { target_rect.origin.x - rect.size.w - margin.far.w, PlacePopupJustify(target_rect.origin.y, target_rect.size.h, rect.size.h, justify) };
			constrain_flags = 1;
			break;

		case kAlignmentCenter:
			rect.origin = { PlacePopupJustify(target_rect.origin.x, target_rect.size.w, rect.size.w, kOrientationCenter), PlacePopupJustify(target_rect.origin.y, target_rect.size.h, rect.size.h, kOrientationCenter) };
			constrain_flags = 0;
			break;

		case kAlignmentRight:
			rect.origin = { target_rect.origin.x + target_rect.size.w + margin.near.w, PlacePopupJustify(target_rect.origin.y, target_rect.size.h, rect.size.h, justify) };
			constrain_flags = 1;
			break;

		case kAlignmentBottomLeft:
			rect.origin = { target_rect.origin.x - rect.size.w - margin.far.w, target_rect.origin.y + target_rect.size.h + margin.near.h };
			break;

		case kAlignmentBottom:
			rect.origin = { PlacePopupJustify(target_rect.origin.x, target_rect.size.w, rect.size.w, justify), target_rect.origin.y + target_rect.size.h + margin.near.h };
			constrain_flags = 2;
			break;

		case kAlignmentBottomRight:
			rect.origin = { target_rect.origin.x + target_rect.size.w + margin.near.w, target_rect.origin.y + target_rect.size.h + margin.near.h };
			break;
		}

		if (PlacePopupFits(bounds, rect, constrain_flags)) break;

		alignment = Alignment((kNumAlignment - 1) - alignment);	//flip
	}

	popup->SetRect(SnapToPixels(ConstrainRect(bounds, rect)));

	if (Detail::GetBool(popup->GetStyle(), kanimate))
	{
		SetOpacity(popup, {}, 0.0f);

		Run(popup, {}, 0.25f, CreateOpacityAnimation({}, 0.0f, 1.0f, Detail::ComputedStyle::kRenderAuto));
	}
}

Reflex::GLX::Object * Reflex::GLX::Detail::QueryPopup(Object & owner)
{
	return owner.QueryProperty<GLX::Object>(kpopup);
}
