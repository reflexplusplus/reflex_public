#include "../../../../include/reflex_ext/glx/functions/overlay.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::GLX)

struct Overlay : public Object
{
	bool OnEvent(Object & src, Event & e) override
	{
		if (e.id == kMouseDown)
		{
			if (block_input)
			{
				return true;
			}
			else
			{
				Detail::DiscardOverlay(*this);

				return false;
			}
		}
		else if (e.id == kDragDropTender)
		{
			return block_dragdrop;
		}
		else if (e.id == kKeyDown)
		{
			auto keycode = GetKeyCode(e);

			if (keycode == kKeyCodeEscape && !block_input)
			{
				Detail::DiscardOverlay(*this);

				return true;
			}

			return block_input;
		}
		else if (e.id == kRequestClose)
		{
			Core::Context evnt;

			Object::id = kMaxUInt32;

			EnableMouse(*this, false, false);

			Exit(*this, true);

			return true;
		}

		return false;
	}

	bool block_input;

	bool block_dragdrop;
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::Acquire(Object & parent, Key32 id, UInt8 enterflags, const Function<TRef<Object>()> & ctr)
{
	if (auto pobject = QueryChildById(parent, id))
	{
		return *pobject;
	}

	auto object = ctr();

	object->id = id;

	object->SetParent(parent);	//BEFORE ENTER

	Enter(object, enterflags);

	return object;
}

void Reflex::GLX::Discard(Object & parent, Key32 id)
{
	if (auto pobject = QueryChildById(parent, id))
	{
		pobject->id = {};

		Exit(*pobject, true);
	}
}

bool Reflex::GLX::Detail::DiscardOverlay(Object & overlay)
{
	Event e(kRequestClose);

	return overlay.ProcessEvent(overlay, e);
}

Reflex::TRef <Reflex::GLX::Object> Reflex::GLX::AcquireOverlay(Object & parent, Key32 id, bool block_input, bool block_dragdrop, Key32 style_id, const Function<void(Object&)> & oninit)
{
	auto overlay = Cast<Overlay>(Acquire(parent, id, kEnterAnimationFade, [&parent, style_id, oninit]()
	{
		TRef <Object> overlay = REFLEX_CREATE(Overlay);

		Reflex::Detail::SilentReference <Object> retain(overlay);

		EnableFloat(overlay, kOrientationFit, kOrientationFit);

		overlay->SetMod(kMaxUInt32 - 1, Detail::ComputedStyle::Create(1.0f, 1.0f, Detail::ComputedStyle::kRenderFalse));

		overlay->SetProperty(kDragDropTender, Null<Animation>());	//NAMESPACE HACK for Freestyle::DragManager

		Data::SetBool(overlay, kDragDropTender, false);					//NAMESPACE HACK for Freestyle::DragManager

		oninit(overlay);

		overlay->SetStyle(FindStyle(parent, style_id));

		return overlay;
	}));

	overlay->SendTop();

	overlay->block_input = block_input;

	overlay->block_dragdrop = block_dragdrop;

	return overlay;
}

bool Reflex::GLX::DiscardOverlay(Object & parent, Key32 id)
{
	if (auto overlay = QueryChildById(parent, id))
	{
		return Detail::DiscardOverlay(*overlay);
	}

	return false;
}
