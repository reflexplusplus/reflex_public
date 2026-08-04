#include "../../../include/reflex/glx/functions.h"




//
//impl

void Reflex::GLX::ClearText(Object & object, Key32 id)
{
	GetProperty<Text>(object, id).RemoveConst()->ClearValue();

	object.ClearState(kUsedState);

	object.Accommodate();
}

void Reflex::GLX::SetText(Object & object, WString && label, Key32 id)
{
	auto text = Data::Detail::AcquireProperty<Text>(object, id, false);

	SetState(object, kUsedState, True(label));

	text->SetValue(std::move(label));

	object.Accommodate();
}

Reflex::WString::View Reflex::GLX::GetText(const Object & object, Key32 id)
{
	return GetProperty<Text>(object, id)->GetView();
}

void Reflex::GLX::SetViewPortGridStringifier(Object & object, Key32 id, const Function <WString(Float32 position)> & stringifier)
{
	typedef ObjectOf < Function <WString(Float32 position)> > Property;
	
	object.SetProperty(id, New<Property>(stringifier));
}

bool Reflex::GLX::ExceedsDragThreshold(Point drag, Float sens)
{
	return (Abs(drag.x) + Abs(drag.y)) > sens;
}
