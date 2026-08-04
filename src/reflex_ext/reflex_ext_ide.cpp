#include "reflex/reflex.h"

#ifndef REFLEX_INCLUDE_IDE
#define REFLEX_INCLUDE_IDE REFLEX_DEBUG
#endif

#if REFLEX_INCLUDE_IDE

#include "ide/defines.cpp"
#include "ide/global.cpp"
#include "ide/proxypath.cpp"
#include "ide/functions.cpp"

#include "ide/view/info_item.cpp"
#include "ide/view/console_panel.cpp"
#include "ide/view/console.cpp"
#include "ide/view/texteditor.cpp"
#include "ide/view/propertyeditor.cpp"
#include "ide/view/functions.cpp"
#include "ide/view/builder.cpp"
#include "ide/view/ui.cpp"
#include "ide/view/debug.cpp"
#include "ide/resources.cpp"

#else

#include "ide/defines.cpp"
#include "ide/placeholder.cpp"
#include "ide/functions.cpp"

#include "ide/view/info_item.cpp"
#include "ide/view/console_panel.cpp"
#include "ide/view/texteditor.cpp"
#include "ide/view/functions.cpp"

Reflex::TRef <Reflex::Object> Reflex::IDE::AcquireConsole(TRef <GLX::Object> root, const Function <void()> & onclose)
{
	return {};
}

#endif