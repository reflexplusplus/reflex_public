#include "[require].h"




REFLEX_BEGIN_INTERNAL(Reflex)

UInt Copy(ArrayRegion <char> & buffer, const CString::View & value)
{
	auto pdest = buffer.data;

	auto length = Min(value.size, buffer.size);

	REFLEX_LOOP_PTR(value.data, psrc, length) *pdest++ = *psrc;

	return length;
}

REFLEX_END_INTERNAL

REFLEX_NS(Reflex::Detail)

UInt Shift(ArrayRegion <char> & buffer, const ArrayView <char> & written)
{
	//this is bcecause Reflex::Detail in-place number converters write to end of buffer, not the start

	auto dest = buffer.data;

	for (auto & i : written) *dest++ = i;

	return written.size;
}

UInt AutoPrecision(Float64 f)
{
	if (f > 100.0)
	{
		return 1;
	}
	else if (f > 10.0)
	{
		return 2;
	}
	else if (f > 1.0)
	{
		return 3;
	}
	else
	{
		return 4;
	}
}

REFLEX_END

char Reflex::Detail::g_debug_scratch[8][256];

Reflex::UInt Reflex::Detail::g_debug_scratch_idx = 0;

Reflex::UInt Reflex::Detail::Stringizer<Reflex::CString::View>::Call(ArrayRegion <char> & buffer, const CString::View & value)
{
	return Copy(buffer, value);
}

Reflex::UInt Reflex::Detail::Stringizer<Reflex::WString::View>::Call(ArrayRegion <char> & buffer, const WString::View & value)
{
	auto pdest = buffer.data;

	auto length = Min(value.size, buffer.size);

	REFLEX_LOOP_PTR(value.data, psrc, length) *pdest++ = char(*psrc);

	return length;
}

Reflex::UInt Reflex::Detail::Stringizer<Reflex::UInt64>::Call(ArrayRegion <char> & buffer, UInt64 value)
{
	return Shift(buffer, Reflex::Detail::ToCString(value, buffer));
}

Reflex::UInt Reflex::Detail::Stringizer<Reflex::Int64>::Call(ArrayRegion <char> & buffer, Int64 value)
{
	return Shift(buffer, Reflex::Detail::ToCString(value, buffer));
}

Reflex::UInt Reflex::Detail::Stringizer<Reflex::Float64>::Call(ArrayRegion <char> & buffer, Float64 value)
{
	return Shift(buffer, Reflex::Detail::ToCString(value, AutoPrecision(value), true, buffer));
}
