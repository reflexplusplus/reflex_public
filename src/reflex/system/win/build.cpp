#pragma warning (disable : 4091)	//windows bug in shjobj

#include "../common/module.cpp"
#include "../common/null.cpp"
#include "../common/debug.cpp"
#include "../common/criticalsection.cpp"
#include "../common/http.cpp"
#include "../common/notification.cpp"
#include "../common/core.cpp"
#include "../common/graphics/functions.cpp"

#include "library.cpp"
#include "thread.cpp"
#include "debug.cpp"
#include "directory_iterator.cpp"
#include "file_handle.cpp"
#include "http.cpp"

#ifndef REFLEX_MINIMAL
#include "dynamic_library.cpp"
#include "process.cpp"
#include "audio/wasapi.cpp"
#endif

#include "dialog.cpp"

#if REFLEX_INCLUDE_UI
#include "resources.cpp"
#include "ui/functions.cpp"
//#include "ui/wgl.cpp"	//compiled as seperate optional lib, not included by default
#include "ui/renderer.cpp"
#include "ui/d3d.cpp"
#include "ui/window.cpp"
#else
#include "../common/stubs/ui.cpp"
#endif

#include "../common/buildinfo.cpp"

#if (defined(_M_AMD64) || defined(_M_X64))
#else
REFLEX_STATIC_ASSERT(_M_IX86_FP >= 2)
#endif

#pragma warning (default : 4091)

