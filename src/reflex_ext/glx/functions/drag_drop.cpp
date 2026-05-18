#include "../../../../include/reflex_ext/glx/functions/drag_drop.h"





void Reflex::GLX::EnableDragDropAutoScroll(AbstractViewPort & viewport, bool enable)
{
	REFLEX_DECLARE_KEY32(DragStart);
	REFLEX_DECLARE_KEY32(DragEnd);

	if (enable)
	{
		SetAbstractProperty(viewport, kDragStart, Core::desktop->CreateListener(Core::Desktop::kNotificationDragDropBegin, [&viewport]()
		{
			if (viewport.GetWindow())
			{
				auto scoped = BranchContains(viewport, Core::desktop->GetMouseOver());

				viewport.EnableAutoScroll(0.5f, scoped);
			}
		}));

		SetAbstractProperty(viewport, kDragEnd, Core::desktop->CreateListener(Core::Desktop::kNotificationDragDropEnd, [&viewport]()
		{
			viewport.DisableAutoScroll();
		}));
	}
	else
	{
		UnsetAbstractProperty(viewport, kDragStart);

		UnsetAbstractProperty(viewport, kDragEnd);
	}
}
