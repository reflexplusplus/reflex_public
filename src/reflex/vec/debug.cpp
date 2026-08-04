#include "../../../include/reflex/core/debug/output.h"




//
//impl

REFLEX_NS(Reflex::Detail)
extern UInt Shift(ArrayRegion <char> & buffer, const ArrayView <char> & written);
extern UInt AutoPrecision(Float64 f);
REFLEX_END

Reflex::UInt Reflex::Detail::Stringizer<Reflex::SIMD::FloatV4>::Call(ArrayRegion <char> & buffer, const SIMD::FloatV4 & value)
{
	auto itr = buffer;

	if (itr)
	{
		itr[0] = '[';

		itr = Nudge(itr);
	}

	UInt commas = 0;

	REFLEX_LOOP(idx, 4)
	{
		auto a = Shift(itr, Reflex::Detail::ToCString(value[idx], AutoPrecision(value[idx]), true, itr));

		itr = Nudge(itr, a);

		commas += WriteString(itr, kCommaSpace);
	}

	if (commas)
	{
		itr.data[-2] = ']';

		itr.size += 1;
	}

	return buffer.size - itr.size;
}

Reflex::UInt Reflex::Detail::Stringizer<Reflex::SIMD::IntV4>::Call(ArrayRegion <char> & buffer, const SIMD::IntV4 & value)
{
	auto itr = buffer;

	if (itr)
	{
		itr[0] = '[';

		itr = Nudge(itr);
	}

	UInt commas = 0;

	REFLEX_LOOP(idx, 4)
	{
		auto a = Shift(itr, Reflex::Detail::ToCString(Int64(value[idx]), itr));

		itr = Nudge(itr, a);

		commas += WriteString(itr, kCommaSpace);
	}

	if (commas)
	{
		itr.data[-2] = ']';

		itr.size += 1;
	}

	return buffer.size - itr.size;
}

Reflex::UInt Reflex::Detail::Stringizer<Reflex::SIMD::BoolV4>::Call(ArrayRegion <char> & buffer, const SIMD::BoolV4 & value)
{
	auto itr = buffer;

	if (itr)
	{
		itr[0] = '[';

		itr = Nudge(itr);
	}

	UInt commas = 0;

	REFLEX_LOOP(idx, 4)
	{
		WriteString(itr, kFalseTrue[value[idx] == SIMD::kBooleanTrue]);

		commas += WriteString(itr, kCommaSpace);
	}

	if (commas)
	{
		itr.data[-1] = ']';

		itr.size += 1;
	}

	return buffer.size - itr.size;
}
