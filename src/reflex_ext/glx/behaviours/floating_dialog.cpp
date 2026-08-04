#include "../../../../include/reflex_ext/glx/behaviours/floating_dialog.h"




//
//FloatingDialogBehaviour

Reflex::GLX::FloatingDialogBehaviour::FloatingDialogBehaviour()
	: m_resize(GLX::ResizeBehaviour::Create()),
	m_move(GLX::MoveBehaviour::Create())
{
}

void Reflex::GLX::FloatingDialogBehaviour::Attach(GLX::Object & parent)
{
	if (parent != object->GetParent())
	{
		m_attach_debounce_clock = CreateAnimationClock([this, parent = Make<Reflex::Detail::WeakRef<GLX::Object>>(parent)](Float)
		{
			object->SetParent(parent->Load());

			m_attach_debounce_clock.Clear();
		});
	}
}

void Reflex::GLX::FloatingDialogBehaviour::Detach()
{
	m_attach_debounce_clock.Clear();

	object->Detach();
}

void Reflex::GLX::FloatingDialogBehaviour::Toggle(GLX::Object & parent)
{
	if (object->GetParent() || m_attach_debounce_clock)
	{
		Detach();
	}
	else
	{
		Attach(parent);
	}
}

void Reflex::GLX::FloatingDialogBehaviour::OnAttachObject()
{
	object->SetDelegate(ResizeBehaviour::kClassID, m_resize);

	object->SetDelegate(MoveBehaviour::kClassID, m_move);

	object->EnableOnAttachDetachWindow();

	EnableAbsolute(object);
}

void Reflex::GLX::FloatingDialogBehaviour::OnDetachObject()
{
	m_resize->Detach();

	m_move->Detach();
}

void Reflex::GLX::FloatingDialogBehaviour::OnAttachWindow()
{
	AnimationScope animate(false);

	object->GetWindow()->GLX::Core::WindowClient::GetContent()->ComputeLayout();

	auto parent_size = object->GetParent()->GetRect().size;

	auto position = Detail::Align(parent_size, object->contentsize, kAlignmentCenter);

	object->SetPosition(Detail::SnapToPixels(position));
}