#pragma once

#include "common_utils.hpp"
#include "../common/utf8.h"

REFLEX_NS(Reflex::System)

REFLEX_INLINE NSString* ToNSString(const CString::View& utf8String) {
	return [NSString stringWithUTF8String:utf8String.data];
}

REFLEX_INLINE NSString* ToNSString(const WString::View& str) {
	return ToNSString(System::Common::ToUTF8(str));
}

REFLEX_INLINE CString::View ToCStringView(const NSString* str)
{
	if (str)
	{
		return [str UTF8String];
	}
	else
	{
		return {};
	}
}

REFLEX_INLINE WString ToWString(const NSString* str)
{
	if (str)
	{
		CString::View utf8Chars = [str UTF8String];

		return System::Common::DecodeUTF8(Reinterpret<ArrayView<UInt8>>(utf8Chars));
	}
	else
	{
		return {};
	}
}

extern CString Apple_GetLanguage();


// ⚠️ Avoid using, it doesn't work well as is on iOS. Unless you can fix it, avoid modal dialogs on all platforms.
//void RunSystemEventLoop(std::function<bool()> continueCondition) {
//	NSRunLoop* myRunLoop = [NSRunLoop currentRunLoop];
//	do {
//		[myRunLoop runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
//	} while (continueCondition());
//}

REFLEX_END
