#include "../../../../include/reflex/core/allocator/allocators.h"

REFLEX_INSTANTIATE_DEFAULT_ALLOCATOR;	//needed as allocating globals are used here

#include "../common/module.cpp"
#include "../common/null.cpp"
#include "../common/debug.cpp"
#include "../common/thread.cpp"
#include "../common/criticalsection.cpp"
#include "../common/http.cpp"
#include "../common/notification.cpp"
#include "../common/core.cpp"
#include "../common/utf8.cpp"

#include "../common/stubs/disk_iterator.cpp"
#include "../common/stubs/dynamic_library.cpp"
#include "../common/stubs/process.cpp"
#include "../common/stubs/audio_device.cpp"

#include "../common/posix/directory_iterator.cpp"
#include "../common/posix/file.cpp"
#include "../common/posix/functions.cpp"

#include "library.cpp"
#include "debug.cpp"
#include "file.cpp"
#include "http.cpp"

#include "dialog.cpp"

#if REFLEX_INCLUDE_UI
#include "window.cpp"
#include "graphics/android_opengl.cpp"
#endif

#include "../common/buildinfo.cpp"
