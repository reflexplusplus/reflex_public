#pragma once

#include "system/[require].h"

#ifndef REFLEX_INCLUDE_UI
#define REFLEX_INCLUDE_UI 1
#endif

#include "system/preprocessor.h"

#include "system/module.h"

#include "system/types.h"
#include "system/simd.h"
#include "system/defines.h"

#include "system/dynamic_library.h"
#include "system/thread.h"
#include "system/process.h"
#include "system/file_handle.h"
#include "system/external_resource.h"
#include "system/http.h"
#include "system/disk_iterator.h"
#include "system/directory_iterator.h"
#include "system/functions.h"

#include "system/dialog.h"
#include "system/window.h"
#include "system/renderer.h"

#include "system/audio_device.h"		//experimental/transitional api

#include "system/entry/console.h"		//framework: console/cmd line app
#include "system/entry/instance.h"		//framework: ui app
#include "system/entry/audioplugin.h"	//framework: vst/au plugin
