#pragma once

#include "../array.h"




//
//Primary API

namespace Reflex
{

	template <AllocatePolicy POLICY = kAllocateOver, class TYPE> ArrayRegion <TYPE> Extend(Array <TYPE> & data, UInt n);


	template <class ARRAY> void Reverse(ARRAY && data);


	template <class ARRAY> void Reorder(ARRAY && data, UInt from, UInt to);


	template <class ARRAY> void Rotate(ARRAY && data, Int32 shift);


	template <class TYPE> void Swap(Array <TYPE> & a, Array <TYPE> & b);	//swap overload

}




//
//impl

REFLEX_NS(Reflex::Detail)

template <typename TYPE> inline void Reverse(ArrayRegion <TYPE> data)
{
	REFLEX_LOOP(idx, data.size / 2)
	{
		Swap(data[idx], data[data.size - idx - 1]);
	}
}

template <class TYPE> inline void Reorder(ArrayRegion <TYPE> region, UInt from, UInt to)
{
	REFLEX_ASSERT(from < region.size);
	REFLEX_ASSERT(to < region.size);

	if (from != to)
	{
		if constexpr (kIsRawCopyable<TYPE>)
		{
			Detail::Initialiser <TYPE> temp;

			MemCopy(region.data + from, temp.Adr(), sizeof(TYPE));

			if (from < to)
			{
				MemMove(region.data + from + 1, region.data + from, UIntNative(to - from) * sizeof(TYPE));
			}
			else
			{
				MemMove(region.data + to, region.data + to + 1, UIntNative(from - to) * sizeof(TYPE));
			}

			MemCopy(temp.Adr(), region.data + to, sizeof(TYPE));
		}
		else
		{
			TYPE temp = std::move(region[from]);

			if (from < to)
			{
				for (UInt i = from; i < to; ++i)
				{
					region[i] = std::move(region[i + 1]);
				}
			}
			else
			{
				for (UInt i = from; i > to; --i)
				{
					region[i] = std::move(region[i - 1]);
				}
			}

			region[to] = std::move(temp);
		}
	}
}

template <class TYPE> inline void Rotate(ArrayRegion <TYPE> region, Int shift)
{
	if (region.size)
	{
		shift %= Int(region.size);

		if (shift < 0) shift += region.size;

		if (shift)
		{
			UInt right = UInt(shift);
			UInt left = region.size - right;

			if constexpr (kIsRawCopyable<TYPE>)
			{
				UInt scratch_count = Min(left, right);
				UIntNative scratch_size = UIntNative(scratch_count) * sizeof(TYPE);

				if (scratch_size <= 1024)
				{
					auto temp = static_cast<TYPE*>(REFLEX_STACKALLOC(scratch_size));

					if (right <= left)
					{
						MemCopy(region.data + left, temp, scratch_size);
						MemMove(region.data, region.data + right, UIntNative(left) * sizeof(TYPE));
						MemCopy(temp, region.data, scratch_size);
					}
					else
					{
						MemCopy(region.data, temp, scratch_size);
						MemMove(region.data + left, region.data, UIntNative(right) * sizeof(TYPE));
						MemCopy(temp, region.data + right, scratch_size);
					}

					return;
				}
			}

			UInt cycles = region.size;
			UInt divisor = left;

			while (divisor)
			{
				UInt remainder = cycles % divisor;

				cycles = divisor;
				divisor = remainder;
			}

			if constexpr (kIsRawCopyable<TYPE>)
			{
				Detail::Initialiser <TYPE> temp;

				REFLEX_LOOP(start, cycles)
				{
					MemCopy(region.data + start, temp.Adr(), sizeof(TYPE));

					UInt current = start;

					while (true)
					{
						UInt next = current < right ? current + left : current - right;

						if (next == start) break;

						MemCopy(region.data + next, region.data + current, sizeof(TYPE));

						current = next;
					}

					MemCopy(temp.Adr(), region.data + current, sizeof(TYPE));
				}
			}
			else
			{
				REFLEX_LOOP(start, cycles)
				{
					TYPE temp = std::move(region[start]);
					UInt current = start;

					while (true)
					{
						UInt next = current < right ? current + left : current - right;

						if (next == start) break;

						region[current] = std::move(region[next]);

						current = next;
					}

					region[current] = std::move(temp);
				}
			}
		}
	}
}

REFLEX_END

template <Reflex::AllocatePolicy POLICY, class TYPE> REFLEX_INLINE Reflex::ArrayRegion <TYPE> Reflex::Extend(Array <TYPE> & data, UInt n)
{
	UInt size = data.GetSize();

	data.template Expand<POLICY>(n);

	return { data.GetData() + size, n };
}

template <typename ARRAY> inline void Reflex::Reverse(ARRAY && data)
{
	Detail::Reverse(ToRegion(data));
}

template <class ARRAY> inline void Reflex::Reorder(ARRAY && data, UInt from, UInt to)
{
	Detail::Reorder(ToRegion(data), from, to);
}

template <class ARRAY> REFLEX_INLINE void Reflex::Rotate(ARRAY && data, Int32 shift)
{
	Detail::Rotate(ToRegion(data), shift);
}

template <class TYPE> REFLEX_INLINE void Reflex::Swap(Array <TYPE> & a, Array <TYPE> & b)
{
	a.Swap(b);
}
