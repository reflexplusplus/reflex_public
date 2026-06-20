#include "../../../include/reflex_ext/core/date.h"



	
//
//date

REFLEX_BEGIN_INTERNAL(Reflex)

bool IsLeapYear(UInt y)
{
	return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

constexpr UInt16 kBaseYear = 1970;

constexpr UInt32 kSecondsPerDay = 3600 * 24;

constexpr UInt8 kDaysInMonth[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

constexpr CString::View kMonths[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

REFLEX_END_INTERNAL

Reflex::Tuple <Reflex::UInt16,Reflex::UInt8,Reflex::UInt8> Reflex::UnixTimestampToDate(UInt64 timestamp)
{
	UInt days = UInt(timestamp / kSecondsPerDay);

	UInt year = kBaseYear;

	while (true)
	{
		UInt days_in_year = 365 + UInt(IsLeapYear(year));

		if (days < days_in_year) break;

		days -= days_in_year;

		++year;
	}

	UInt month = 0;

	while (true)
	{
		UInt days_in_month = kDaysInMonth[month];

		if (month == 1 && IsLeapYear(year)) ++days_in_month;

		if (days < days_in_month) break;

		days -= days_in_month;

		++month;
	}

	return { UInt16(year), UInt8(month + 1), UInt8(days + 1) };
}

Reflex::UInt64 Reflex::DateToUnixTimestamp(UInt year, UInt month, UInt day)
{
	UInt days = day - 1;

	for (auto y = kBaseYear; y < year; ++y)
	{
		days += 365 + UInt(IsLeapYear(y));
	}

	for (UInt8 m = 1; m < month; ++m)
	{
		days += kDaysInMonth[m - 1];

		if (m == 2 && IsLeapYear(year)) days += 1;
	}

	return days * kSecondsPerDay;
}

Reflex::UInt8 Reflex::UnixTimestampToWeekday(UInt64 timestamp)
{
	UInt64 days_since_epoch = timestamp / kSecondsPerDay;

	return UInt8((days_since_epoch + 4) % 7); // 0 = Sunday ... 6 = Saturday
}

Reflex::WString Reflex::UnixTimestampToWString(UInt64 timestamp)
{
	auto [year, month, day] = UnixTimestampToDate(timestamp);

	return Join(ToWString(day), L' ', ToWString(kMonths[month - 1]), L' ', ToWString(year));
}

Reflex::UInt64 Reflex::WStringToUnixTimestamp(const WString & string, UInt64 fallback)
{
	constexpr CString::View kDoubleSpace = "  ";

	constexpr char alts[] = { ' ', '\\', '-' };

	CString temp = ToCString(string);

	temp = Trim(ToView(temp));

	UInt len;

	do
	{
		len = temp.GetSize();

		temp = Replace(temp, kDoubleSpace, kSpace);
	} 
	while (len != temp.GetSize());

	REFLEX_LOOP(idx, GetArraySize(alts))
	{
		temp = Replace(temp, alts[idx], '/');
	}

	auto split = Split(temp, '/');

	if (split.GetSize() == 3)
	{
		UInt day = (ToUInt32(split[0]) - 1);

		UInt month = ToUInt32(split[1]);

		if (month)
		{
			month = (month - 1) % 12;
		}
		else if (auto result = Search<CaseInsensitive>(ToView(kMonths), split[1]))
		{
			month = result.value;
		}

		UInt year = ToUInt32(split[2]);

		return DateToUnixTimestamp(year, month, day);
	}

	return fallback;
}
