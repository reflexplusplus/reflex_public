#include "apple_utils.hpp"




//
//common functions ios/macos

Reflex::UInt32 Reflex::System::GetNumProcessor()
{
	return UInt([[NSProcessInfo processInfo] activeProcessorCount]);
}

Reflex::Float64 Reflex::System::GetElapsedTime()
{
	uint64_t nanoseconds = clock_gettime_nsec_np(CLOCK_UPTIME_RAW);	//changed from CLOCK_MONOTONIC 15/07/2025

	return nanoseconds / 1'000'000'000.0;
}

Reflex::CString Reflex::System::GetLanguage()
{
	// Fallback to preferred languages list
	NSArray<NSString *> * langs = [NSLocale preferredLanguages];
	NSString * first = langs.firstObject; // e.g. "en-US"

	if (first.length > 0)
	{
		auto locale = [NSLocale localeWithLocaleIdentifier:first];
		NSString * code = [locale objectForKey:NSLocaleLanguageCode];

		return ToCStringView(code);
	}

	return {};
}
