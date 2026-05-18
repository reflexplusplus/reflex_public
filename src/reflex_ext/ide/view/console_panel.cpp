#include "../globalimpl.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

REFLEX_END_INTERNAL

Reflex::IDE::Detail::ConsolePanel::ConsolePanel(Key32 id, UInt16 version)
	: Data::iStreamable(version)
{
	Data::iStreamable::Publish(*this);

	GLX::Object::id = id;
}

void Reflex::IDE::Detail::ConsolePanel::SetIcon(const GLX::Style & style)
{
	auto stylesheet = RetrieveStyleSheet();

	auto header = stylesheet[K32("Dialog")][K32("header")].RemoveConst();

	RemoveConst(style.id) = id;

	RemoveConst(style).Attach(header);
}

void Reflex::IDE::Detail::ConsolePanel::Reset()
{
	iStreamable::Reset();

	Update();
}

void Reflex::IDE::Detail::ConsolePanel::Deserialize(Data::Archive::View & stream)
{
	iStreamable::Deserialize(stream);

	Update();
}

Reflex::Array < Reflex::Tuple <Reflex::WString, Reflex::IDE::Detail::ConsolePanel&> > Reflex::IDE::Detail::CreatePanels(TRef <GLX::Object> root)
{
	Sequence < Int, Tuple <WString,ConsolePanel&> > panels;

	for (auto & panelctr : ConsolePanel::Ctr::range)
	{
		auto panel = panelctr.ctr();

		if (IsValid(*panel))	//allow AudioDialog panel to be bypassed in plugin
		{
			panel->id = Key32(panelctr.name);

			panels.Insert(panelctr.order, { panelctr.name, panel });

			if (auto vi = DynamicCast<ViewInspectorPanel>(panel)) vi->SetRoot(root);
		}
	}

	Array < Tuple <WString, ConsolePanel&> > rtn;

	rtn.Allocate(panels.GetSize());

	for (auto & i : panels) rtn.Push<kAllocateNone>(i.value);

	return rtn;
}

const Reflex::Key32 Reflex::IDE::Detail::kItemStates[3] = { K32(""), K32("Warning"), K32("Error") };
