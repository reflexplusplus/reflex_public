#pragma once

#include <jni.h>
#include <utility>
#include "reflex/core.h"
#include "../android_utils.hpp"
#include "jni-wrapper.h"
#include "java-classes.h"

#define REFLEX_JNIEXPORT(rtn_type) extern "C" JNIEXPORT rtn_type JNICALL
