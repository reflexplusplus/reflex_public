#pragma once

#include "sdk.h"
#include "../common/debug.h"
#include "../common/notification.h"




//
//iOS::Library

@interface ReflexDisplayLinkReceiver : NSObject
- (void)updateFrame;
@end

REFLEX_NS(Reflex::System::iOS)

struct Library {
	Library();
	~Library();

	static void OnTimer(CFRunLoopTimerRef timer, void* info);

	Common::Signal m_signals[kNumNotification + 2];


private:

	CFRunLoopTimerRef m_timer = 0;
	ObjCRef <ReflexDisplayLinkReceiver*> m_displaylinkreceiver;
};

extern Reflex::Detail::Module::Member <Library> globals;

REFLEX_END
