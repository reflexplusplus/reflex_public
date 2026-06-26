#include "object.h"
#include "global.h"




//
//

Reflex::GLXVM::Object::Object(VM::Context & context)
	: WidgetOf(context),
	context(context),
	m_callbacks(&Library::st_self->m_callbacks)
{
}

void Reflex::GLXVM::Object::OnClock(Float32 delta)
{
	context->Call(*m_callbacks->onclock, delta);
}

void Reflex::GLXVM::Object::OnReleaseData()
{
	EnableOnAttachDetachWindow(false);

	EnableOnClock(false);

	GLX::Object::OnReleaseData();
}

void Reflex::GLXVM::Object::OnSetProperty(Address address, Reflex::Object & object)
{
	if (address == Address( { kExtension, REFLEX_TYPEID(Callbacks) }))
	{
		m_callbacks = Cast<Callbacks>(&object);
	}

	WidgetOf::OnSetProperty(address, object);
}

void Reflex::GLXVM::Object::OnUnsetProperty(Address address)
{
	if (address == Address({ kExtension, REFLEX_TYPEID(Callbacks) }))
	{
		m_callbacks = &Library::st_self->m_callbacks;
	}

	GLX::Object::OnUnsetProperty(address);
}
