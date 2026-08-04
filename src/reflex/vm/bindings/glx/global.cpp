#include "global.h"




Reflex::GLXVM::Library::Library()
	: m_allocator(CreateAllocator(kRecycleAllocID, REFLEX_NULL(Object))),
	m_callbacks(REFLEX_NULL(VM::Context))
{
	REFLEX_ASSERT(!st_self);

	st_self = this;

	g_allocator = m_allocator.Adr();

	//constexpr Key32 ids[] = { K32("DragStart"), K32("DragEnd") };

	//REFLEX_LOOP(idx, 2)
	//{
	//	m_ondragstart_end[idx] = GLX::Core::desktop->CreateListener(GLX::Core::Desktop::Notification(GLX::Core::Desktop::kNotificationOnDragStart + idx), [id = ids[idx]](Reflex::Object & src)
	//	{
	//		auto e = AutoRelease(New<GLX::Event>(id));

	//		e->SetProperty(K32("source"), src);

	//		REFLEX_NULL(GLX::Object).Emit(e);
	//	});
	//}
}

