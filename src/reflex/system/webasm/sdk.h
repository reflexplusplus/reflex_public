#pragma once

#include <emscripten/webaudio.h>
#include <emscripten/fetch.h>

#define REFLEX_WASM_IMPORT extern "C"

#define REFLEX_WASM_EXPORT extern "C" __attribute__((used)) __attribute__((visibility("default")))

#include "../../../../include/reflex/system.h"
#include "../../../../include/reflex/file/functions.h"

#include "../common/utf8.h"
#include "../../file/memorystream.h"


typedef Reflex::UIntNative em_ptr_t;

typedef Reflex::UInt32 em_val32;





//
//Javascript functions (defined in webasm/resources/reflex.js)

REFLEX_WASM_IMPORT void jsConsoleLog(const char * ptr);

REFLEX_WASM_IMPORT void jsConsoleError(const char * ptr);

REFLEX_WASM_IMPORT void jsInitSystem();

REFLEX_WASM_IMPORT void jsDeinitSystem();


REFLEX_WASM_IMPORT Reflex::Float64 jsGetPerformanceTimer();


REFLEX_WASM_IMPORT void jsDeleteLocalStorage(const char * ptr);

REFLEX_WASM_IMPORT void jsWriteLocalStorage(const char * keyPtr, const Reflex::UInt8 * data, Reflex::UInt32 length);

REFLEX_WASM_IMPORT Reflex::Int32 jsQueryLocalStorage(const char * ptr);

REFLEX_WASM_IMPORT void jsReadLocalStorage(const char * keyPtr, const Reflex::UInt8 * & data, Reflex::UInt & length);


REFLEX_WASM_IMPORT void jsStartWebAudio();

REFLEX_WASM_IMPORT void jsStopWebAudio();

REFLEX_WASM_IMPORT Reflex::Float64 jsWebAudioGetSampleRate();



//
//functions exported to be visible to js

REFLEX_WASM_EXPORT void reflexOnUnload();

REFLEX_WASM_EXPORT void reflexOnAnimationClock();
