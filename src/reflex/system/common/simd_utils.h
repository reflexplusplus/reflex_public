#pragma once

#include "./[require].h"

REFLEX_NS(Reflex::System::Common)

template <class INT> struct MaxInt {};

template <> struct MaxInt <Int16> { static inline const Int16 value = kMaxInt16; };

template <> struct MaxInt <Int32> { static inline const Int32 value = kMaxInt32; };

template <class INT> struct FromIntStream
{
	static inline const Float32 kMult = Float32(1.0 / Float64(MaxInt<INT>::value));

	static inline const Float32x4 kMultV4 = REFLEX_SET1_F32_X4(kMult);

	static void Convert(UInt nblock, UInt remainder, const void * buffer, Float * channel);
};

inline constexpr Float32 kRangeInt16 = 32767.0f;
inline constexpr Float32x4 kRangeInt16_V4 = { kRangeInt16, kRangeInt16, kRangeInt16, kRangeInt16 };
inline constexpr Float32x4 kMinusOne_V4 = { -1.0f, -1.0f, -1.0f, -1.0f };
inline constexpr Float32x4 kOne_V4 = { 1.0f, 1.0f, 1.0f, 1.0f };

REFLEX_END




//
//Implementation

template <class INT> REFLEX_INLINE void Reflex::System::Common::FromIntStream<INT>::Convert(UInt nblock, UInt remainder, const void * buffer, Float * channel)
{
	const INT * source = Reinterpret<INT>(buffer);

	auto dest = Reinterpret<Float32x4>(channel);

	REFLEX_LOOP(block, nblock)
	{
		Int32x4 value;

		auto ptr = Reinterpret<Int32>(&value);

		ptr[0] = *source++;
		ptr[1] = *source++;
		ptr[2] = *source++;
		ptr[3] = *source++;

		*dest++ = REFLEX_MUL_F32_X4(REFLEX_I32_X4_TO_F32_X4(value), kMultV4);
	}

	REFLEX_LOOP(sample, remainder) Reinterpret<Float32>(dest)[sample] = ToFloat32(*source++) * kMult;
}
