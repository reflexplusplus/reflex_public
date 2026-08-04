#include "../../../../include/reflex_ext/glx/functions/style.h"




void Reflex::GLX::SetOnStyle(Object & object, Key32 delegate_id, const Function <void(const Style&)> & callback)
{
	struct Delegate : public GLX::Object::Delegate
	{
		Delegate(const Function <void(const Style & style)> & callback) : m_callback(callback) {}

		void OnAttachObject() override
		{
			if (auto current = object->GetStyle())
			{
				OnSetStyle(current);
			}
		}

		void OnSetStyle(const Style & style) override
		{
			m_callback(style);
		}

		const Function <void(const Style & style)> m_callback;
	};

	object.SetDelegate(delegate_id, New<Delegate>(callback));
}

void Reflex::GLX::UnsetOnStyle(Object & object, Key32 delegate_id)
{
	object.ClearDelegate(delegate_id);
}

void Reflex::GLX::SelectChildren(Object & object, bool select)
{
	if (select)
	{
		for (auto & i : object) i.SetState(kSelectedState);
	}
	else
	{
		for (auto & i : object) i.UnsetState(kSelectedState);
	}
}

void Reflex::GLX::ActivateBranch(Object & object, bool enable)
{
	if (enable)
	{ 
		EnableMouse(object, true, false);

		ClearBranchState(object, kInactiveState);
	}
	else
	{
		EnableMouse(object, false, true);

		SetBranchState(object, kInactiveState);
	}
}

void Reflex::GLX::SetBranchState(Object & object, Key32 state)
{
	object.SetState(state);

	for (auto & i : object)
	{
		SetBranchState(i, state);
	}
}

void Reflex::GLX::ClearBranchState(Object & object, Key32 state)
{
	object.UnsetState(state);

	for (auto & i : object)
	{
		ClearBranchState(i, state);
	}
}
