#pragma once

#include <cfloat>
#include "debug.h"

REFLEX_NS(Reflex::System)

template<class T>
struct always_false : std::false_type {};

template<class T>
inline constexpr bool always_false_v = always_false<T>::value;

constexpr Reflex::Float64 operator ""_secs_in_ms(long double seconds) {
	return Float64(seconds) * 1000;
}

template<class VALUE>
static VALUE FindClosest(const Array<VALUE>& array, VALUE value_to_find) {
	if (array.Empty()) return value_to_find;

	Float64 min_diff = Abs(Float64(value_to_find - array[0]));
	UInt min_index = 0;

	for (UInt i = 1, end = array.GetSize(); i < end; i++) {
		auto diff = Abs(Float64(value_to_find - array[i]));
		if (diff < min_diff) {
			min_diff = diff;
			min_index = i;
		}
	}
	return array[min_index];
}

inline bool IsWithinDistance(float distX, float distY, float allowedDist) {
	return distX * distX + distY * distY <= allowedDist * allowedDist;
}

template <int PeriodInMs = 1000, bool TriggerOnFirstCall = true>
struct DebouncedAction {
	static constexpr Float64 kPeriodInSecs = PeriodInMs / 1000.0;

	DebouncedAction()
		: lastActionTime(TriggerOnFirstCall ? 0 : GetElapsedTime())
	{}

	inline void Perform(const std::function<void()>& action) {
		auto now = GetElapsedTime();
		m_occurrences += 1;
		if (now >= lastActionTime + kPeriodInSecs) {
			lastActionTime = now;
			RemoveConst(NumOccurrences) = m_occurrences;
			m_occurrences = 0;
			action();
		}
	}

	const unsigned NumOccurrences = 0;

private:
	Float64 lastActionTime;
	unsigned m_occurrences = 0;
};

template<class FN, class ... VARGS>
inline void InvokeAndClear(Function <FN> & callback, VARGS&& ... result) {
	auto fn = callback;
	if (fn) {
		callback.Clear();
		fn(std::forward<VARGS>(result)...);
	}
}

inline bool StartsWith(const WString::View& haystack, const WString::View& needle) {
	return Left<true>(haystack, needle.size) == needle;
}

inline bool IsAlmostEqual(float a, float b) {
	return Abs(b - a) <= FLT_EPSILON;
}

inline bool IsAlmostEqual(double a, double b) {
	return Abs(b - a) <= DBL_EPSILON;
}

// TODO: [Florian] check with James, why doesn't it work to declare ArrayView<TYPE>& b?
template<class TYPE>
inline void Intersect(Array<TYPE>& a, const Array<TYPE>& b) {
	REFLEX_RLOOP(i, a.GetSize()) {
		if (!Search(b, a[i])) a.Remove(i);
	}
}

REFLEX_END
