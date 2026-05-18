#pragma once

#include "[require].h"



	
//
//Primary API

namespace Reflex
{

	Tuple <UInt16,UInt8,UInt8> UnixTimestampToDate(UInt64 timestamp);

	UInt64 DateToUnixTimestamp(UInt year, UInt month, UInt day);

	UInt8 UnixTimestampToWeekday(UInt64 timestamp);


	WString UnixTimestampToWString(UInt64 timestamp);

	UInt64 WStringToUnixTimestamp(const WString & string, UInt64 fallback = 0);

}
