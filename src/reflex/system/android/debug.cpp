#include <iostream>
#include <android/log.h>
#include "jni/[require].h"

//
//functions

#if REFLEX_DEBUG
void Reflex::System::DebugLog(bool brk, const char *text) {
	__android_log_write(brk ? ANDROID_LOG_ERROR : ANDROID_LOG_DEBUG, "REFLEX", text);

	if (brk) g_on_debug_break(text);
}
#endif

void Reflex::System::Log(const char *ptr) {
	__android_log_write(ANDROID_LOG_DEBUG, "REFLEX", ptr);
}
