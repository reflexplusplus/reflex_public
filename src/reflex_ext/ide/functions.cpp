#include "../../../include/reflex_ext/ide.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::IDE)

REFLEX_END_INTERNAL

void Reflex::IDE::AddStyleSheet(ResourceGroup & monitor, const GLX::StyleSheet & sheet)
{
	REFLEX_LOCAL(void, AddLocked)(ResourceGroup & monitor, const GLX::StyleSheet & sheet)
	{
		monitor.AddItem(MakeAddress<GLX::StyleSheet>(sheet.path), sheet);

		REFLEX_LOOP(idx, 2)
		{
			for (auto & i : sheet.GetIncludes(True(idx)))
			{
				Call(monitor, i);
			}
		}
	}
	REFLEX_END
	
	AddLocked::Call(monitor, sheet);
}
