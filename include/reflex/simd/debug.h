#include "[require].h"




//
//Debug overloads

template <> struct Reflex::Detail::Stringizer <Reflex::SIMD::FloatV4>
{
	static UInt Call(ArrayRegion <char> & buffer, const SIMD::FloatV4 & value);
};

template <> struct Reflex::Detail::Stringizer <Reflex::SIMD::IntV4>
{
	static UInt Call(ArrayRegion <char> & buffer, const SIMD::IntV4 & value);
};

template <> struct Reflex::Detail::Stringizer <Reflex::SIMD::BoolV4>
{
	static UInt Call(ArrayRegion <char> & buffer, const SIMD::BoolV4 & value);
};
