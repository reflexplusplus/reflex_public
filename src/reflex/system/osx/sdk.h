#pragma once

#include "../common/module.h"
#include "../common/debug.h"
#include "../common/apple_objcref.h"
#include "../common/apple_utils.hpp"

#include <Cocoa/Cocoa.h>
#include <AvailabilityMacros.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreMidi/MidiServices.h>
#include <ApplicationServices/ApplicationServices.h>
#include <mach/mach_time.h>
#include <DiskArbitration/dadisk.h>

#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUCocoaUIView.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include <mach-o/dyld.h>
#include <mach-o/ldsyms.h>

#include <new>
#include <stdio.h>
#include <sys/types.h>
#include <sys/user.h>

#if (REFLEX_MACOS_TARGET == REFLEX_TARGET_OPENGL)
#define GL_SILENCE_DEPRECATION 1
#include <OpenGL/gl.h>
#endif




//
//generate classnames, need to be unique between plugin targets due to objective c dynamic lookup fiasco

#ifndef REFLEX_MACOS_TARGET
#error REFLEX_MACOS_TARGET Not Defined
#endif

#define REFLEX_NSCLASSID REFLEX_CONCATENATE(ReflexJun2026,REFLEX_MACOS_TARGET)

#define NS_HttpDelegate REFLEX_CONCATENATE(REFLEX_NSCLASSID,_HttpDelegate)
#define NS_View REFLEX_CONCATENATE(REFLEX_NSCLASSID,_View)
#define NS_Window REFLEX_CONCATENATE(REFLEX_NSCLASSID,_Window)
#define NS_PluginView REFLEX_CONCATENATE(REFLEX_NSCLASSID,_PluginView)
#define NS_OpenGLView REFLEX_CONCATENATE(REFLEX_NSCLASSID,_OpenGLView)




//
//functions

REFLEX_NS(Reflex::System::OSX)

Common::UTF8 ResolveBundlePath(const WString & path);


inline ObjCRef <NSString*> MakeNSStringRef(const WString::View & wstring)
{
	return MakeOwnedObjCRef([[NSString alloc] initWithBytes:wstring.data length:(wstring.size * sizeof(WChar)) encoding:0x9c000100]);
}

inline UInt32 NSStringToWChar(const NSString * string, WChar * buffer, UInt bufferlen)
{
	UInt n = Min(UInt([string length]), bufferlen - 2);

	buffer[n] = 0;

	REFLEX_LOOP(idx, n) buffer[idx] = [string characterAtIndex:idx];

	return n;
}

extern bool g_enableretina;

REFLEX_END

inline bool operator==(const CGPoint & a, const CGPoint & b)
{
	return Reflex::MemCompare(&a, &b, sizeof(CGPoint));
}
