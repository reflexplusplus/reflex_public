#include "reflex/core/string/convert.h"




//
//internal

REFLEX_BEGIN_INTERNAL(Reflex)

constexpr Pair <Float64, UInt> kMaxFloat[24] =
{
	{ 0, 0 },
	{ 0, 0 },
	{ 9, 0 },
	{ 9, 0 },

	{ 9, 1 },
	{ 99, 1 },
	{ 99, 2 },
	{ 999, 2 },

	{ 999, 3 },
	{ 9999, 3 },
	{ 9999, 4 },
	{ 99999, 4 },

	{ 99999, 5 },
	{ 999999, 5 },
	{ 999999, 6 },
	{ 9999999, 6 },

	{ 9999999, 7 },
	{ 99999999, 7 },
	{ 99999999, 8 },
	{ 999999999, 8 },

	{ 9999999999, 8 },
	{ 99999999999, 8 },
	{ 999999999999, 8 },
	{ 9999999999999, 8 },
};

constexpr Float64 kRound[17] =
{
	1.0,
	0.1,
	0.01,
	0.001,
	0.0001,
	0.00001,
	0.000001,
	0.0000001,
	0.00000001,
	0.000000001,
	0.0000000001,
	0.00000000001,
	0.000000000001,
	0.0000000000001,
	0.00000000000001,
	0.000000000000001,
	0.0000000000000001
};

constexpr UInt kMaxPrecision = (GetArraySize(kRound) - 1);

constexpr UInt kMinus = UInt('-' - '0');

constexpr char kDiscardChar[2] = { 0, '0' };


template <class INT> struct IntInfo {};

template <> struct IntInfo <UInt32>
{
	static const bool kSigned = false;

	static const UInt kMaxWidth = 10;
};

template <> struct IntInfo <UInt64>
{
	static const bool kSigned = false;

	static const UInt kMaxWidth = 20;
};

template <> struct IntInfo <Int>
{
	static const bool kSigned = true;

	static const UInt kMaxWidth = 11;
};

template <> struct IntInfo <Int64>
{
	static const bool kSigned = true;

	static const UInt kMaxWidth = 21;
};


template <class CHARACTER, class INT> REFLEX_INLINE ArrayView <CHARACTER> IntToString(INT value, const ArrayRegion <CHARACTER> & buffer)
{
	//NOT ZERO_TERMINATED !

	typedef String <CHARACTER> STRING;

	typedef IntInfo <INT> Traits;

	if (buffer.size < Traits::kMaxWidth)
	{
		return {};
	}
	else
	{
		CHARACTER * pbuffer = buffer.data + Traits::kMaxWidth;

		UInt length = 0;

		if (Traits::kSigned)
		{
			if (value < 0)
			{
				value *= -1;

				do
				{
					(*--pbuffer) = '0' + (value % 10);

					value /= 10;

					length++;
				} while (value);

				*--pbuffer = '-';

				length++;

				return { pbuffer, length };
			}
		}

		do
		{
			(*--pbuffer) = '0' + (value % 10);

			value /= 10;

			length++;
		} while (value);

		return { pbuffer, length };
	}
}

template <class CHARACTER> inline ArrayView <CHARACTER> FloatToString(Float64 value, UInt precision, bool discard_zeros, CHARACTER * buffer, UInt maxlen)
{
	typedef String <CHARACTER> STRING;

	if (maxlen < 23)
	{
		return {};
	}
	else
	{
		maxlen = Min(maxlen, GetArraySize(kMaxFloat));

		precision = Min(precision, kMaxPrecision);

		--maxlen;

		auto & max = kMaxFloat[maxlen];

		buffer[maxlen] = 0;

		CHARACTER * pend = buffer + maxlen;

		CHARACTER * pbuffer = pend - 1;



		//prep

		value = Quantise(value, kRound[precision]);

		Float64 abs = Min<Float64>(Abs(value), max.a);



		//fraction

		if (precision)
		{
			precision = Min<UInt>(precision, max.b);

			pbuffer -= precision;


			CHARACTER * ptr = pbuffer;

			(*ptr++) = '.';

			CHARACTER * end = ptr + precision;

			auto start = ptr + 1;


			Float64 mult = 10.0;

			while (ptr != end)
			{
				(*ptr++) = '0' + char(Int64(abs * mult) % 10);

				mult *= 10.0;
			}


			pbuffer--;

			auto discard = kDiscardChar[discard_zeros];

			while (pend > start && pend[-1] == discard)
			{
				pend--;
			}
		}



		//whole

		do
		{
			(*pbuffer--) = '0' + char(Int64(abs) % 10);

			abs *= 0.1;
		}
		while (abs >= 1.0);



		//sign

		if (value < 0.0f)
		{
			(*pbuffer) = '-';
		}
		else
		{
			pbuffer++;
		}

		return { pbuffer, UInt(pend - pbuffer) };
	}
}

template <class INT, class CHARACTER> REFLEX_INLINE INT StringToInt(const ArrayView <CHARACTER> & string)
{
	const CHARACTER * buffer = string.data;

	INT rtn = 0;

	if (UInt length = string.size)
	{
		INT mult = 1;

		//if constexpr (IntInfo<INT>::kSigned)
		//{
		//	if (buffer[0] == '-')
		//	{
		//		buffer++;

		//		length--;

		//		mult = -1;
		//	}
		//}

		const CHARACTER * ptr = buffer + length;

		while (ptr-- != buffer)
		{
			UInt digit = *ptr - '0';

			if (digit < 10)
			{
				rtn += (digit * mult);

				mult *= 10;
			}
			else if (digit == kMinus)
			{
				rtn *= -1;
			}
			else if (digit == 4294967294)	//(UInt('.') - UInt('0')))
			{
				break;
			}
		}
	}

	return rtn;
}

template <class CHARACTER> REFLEX_INLINE Float64 StringToFloat(const ArrayView <CHARACTER> & string)
{
	const CHARACTER * buffer = string.data;

	if (UInt length = string.size)
	{
		//sign

		Float64 rtn = 0.0;

		//Float64 sign = 1.0;

		Float64 mult = 1.0;

		//if (*buffer == '-')
		//{
		//	sign = -1.0;

		//	buffer++;

		//	length--;
		//}

		//frac

		if (auto dot = Search(ArrayView<CHARACTER>(buffer, length), CHARACTER('.')))
		{
			UInt pos = (dot.value) + 1;

			UInt n = length - pos;

			const CHARACTER * start = buffer + pos;

			const CHARACTER * ptr = start + n;

			while (ptr-- != start)
			{
				UInt digit = (*ptr) - CHARACTER('0');

				if (digit < 10)
				{
					rtn += Float32(digit) * mult;

					mult *= 10.0;
				}
			}

			rtn /= mult;

			length -= n;

			length--;

			mult = 1.0;
		}

		//whole

		const CHARACTER * ptr = buffer + length;

		while (ptr-- != buffer)
		{
			UInt digit = (*ptr) - CHARACTER('0');

			if (digit < 10)
			{
				rtn += Float32(digit) * mult;

				mult *= 10.0;
			}
			else if (digit == kMinus)
			{
				rtn *= -1.0;
			}
		}

		return rtn;// *sign;
	}

	return 0.0f;
}

template <UInt BUFFERSIZE> UInt ToCString(const WString::View & wstring, char(&buffer)[BUFFERSIZE])
{
	REFLEX_ASSERT(wstring.size < BUFFERSIZE);

	auto pbuffer = buffer;

	auto len = Min<UInt>(wstring.size, BUFFERSIZE - 1);

	buffer[len] = 0;

	REFLEX_LOOP_PTR(wstring.data, w, len)
	{
		*pbuffer++ = char(*w);
	}

	return len;
}

REFLEX_END_INTERNAL

Reflex::CString Reflex::ToCString(const WString::View & ref)
{
	CString rtn;

	auto pwrite = Extend(rtn, ref.size).data;

	REFLEX_LOOP_PTR(ref.data, w, ref.size)
	{
		if (*w < 256) *pwrite++ = char(*w);
	}

	return rtn;
}

Reflex::CString::View Reflex::Detail::ToCString(UInt64 value, const CString::Region & output)
{
	return IntToString<char>(value, output);
}

Reflex::CString::View Reflex::Detail::ToCString(Int64 value, const CString::Region & output)
{
	return IntToString<char>(value, output);
}

Reflex::CString::View Reflex::Detail::ToCString(Float64 value, UInt precision, bool discard_zeros, const CString::Region & output)
{
	return FloatToString(value, precision, discard_zeros, output.data, output.size);
}

Reflex::CString Reflex::ToCString(Float64 value, UInt precision, bool discard_zeros)
{
	char buffer[23];

	return FloatToString(value, precision, discard_zeros, buffer, GetArraySize(buffer));
}

Reflex::WString Reflex::ToWString(const CString::View & ref)
{
	WString rtn;

	rtn.SetSize(ref.size);

	auto pb = ref.data;

	REFLEX_LOOP_PTR(rtn.GetData(), ptr, ref.size) *ptr = UInt8(*pb++);

	return rtn;
}

Reflex::WString Reflex::ToWString(Float64 value, UInt precision, bool discard_zeros)
{
	WChar buffer[23];

	return FloatToString(value, precision, discard_zeros, buffer, GetArraySize(buffer));
}

Reflex::UInt64 Reflex::ToUInt64(const CString::View & string)
{
	return StringToInt<UInt64>(string);
}

Reflex::Int64 Reflex::ToInt64(const CString::View & string)
{
	return StringToInt<Int64>(string);
}

Reflex::Float64 Reflex::ToFloat64(const CString::View & string)
{
	return StringToFloat(string);
}

Reflex::UInt64 Reflex::ToUInt64(const WString::View & string)
{
	char buffer[11];

	auto len = ToCString(string, buffer);

	return ToUInt64({ buffer, len });
}

Reflex::Int64 Reflex::ToInt64(const WString::View & string)
{
	char buffer[11];

	auto len = ToCString(string, buffer);

	return ToInt64({ buffer, len });
}

Reflex::Float64 Reflex::ToFloat64(const WString::View & string)
{
	return StringToFloat(string);
}
