#pragma once

#include "defines.h"




//
//Secondary API

namespace Reflex
{

	class Output;

}




//
//Output

class Reflex::Output :
	public Detail::StaticItem <Output>,
	public StateMt
{
public:

	//types

	struct Buffer;

	using Queue = Reflex::Queue < Tuple<Key32, LogType, Buffer>, 512 >;

	class Profiler;



	//setup

	static void SetOutputFile(TRef <System::FileHandle> logfile);

	static void Disable();	//disables capture of logs to queue



	//lifetime

	Output(const char * name, UInt8 flags = kMaxUInt8);

	void SetFlags(UInt8 flags);



	//profilers

	void EnumerateProfilers(const Function <void(Profiler&)> & callback);



	//log

	template <typename... VARGS> void Log(VARGS &&... args);

	template <typename... VARGS> void Warn(VARGS &&... args);

	template <typename... VARGS> void Error(VARGS &&... args);


	template <typename... VARGS> void LogEx(LogType type, const CString::View & delimiter, VARGS && ... args);



	//info

	const CString::View name;

	const Key32 id;



	//queue (for IDE)

	static Queue queue;

	static const StateMt & state;	//flags or profilers changed



protected:

	friend Profiler;

	struct Item : public Reflex::Item <Item, false, NullType>
	{
		using Base = Reflex::Item <Item, false, NullType>;
		using Base::Attach;
		using Base::Detach;
		void OnDetach(List & list);
	};


	Item::List m_profilers;

	UInt8 m_output_mask;

	FunctionPointer <void(const Output&,LogType,const CString::View &)> m_output;

};




//
//Output::Buffer

struct Reflex::Output::Buffer
{
	static constexpr UInt kCapacity = 256;

	Buffer();

	Buffer(const CString::View & string);

	CString::View Extract() const { return { data, size }; }

	UInt8 size;

	char data[kCapacity];
};




//
//impl

REFLEX_NS(Reflex)

[[deprecated("use Output")]] typedef Output DebugOutput;

REFLEX_END

REFLEX_NS(Reflex::Detail)

extern FunctionPointer <Float64()> GetElapsedTime;

extern const CString::View kFalseTrue[2];

constexpr CString::View kCommaSpace = ", ";

//Stringizer: in-place string writer (can specialise for additional types)

template <class TYPE> struct Stringizer
{
	static UInt Call(ArrayRegion <char> & buffer, const TYPE & value)
	{
		if (buffer)	//check if buffer has remaining space for output
		{
			buffer[0] = '?';

			return 1;	//return number of characters written
		}

		return 0;
	}
};

template <class TYPE> UInt WriteString(ArrayRegion <char> & buffer, const TYPE & arg);

template <> struct Stringizer <const void*>
{
	static UInt Call(ArrayRegion <char> & buffer, const void * value)
	{
		constexpr UInt kSize = sizeof(void*) * 2;

		constexpr UInt kShiftStart = (kSize - 1) * 4;

		if (buffer.size < kSize)
		{
			return 0;
		}
		else
		{
			auto v = ToUIntNative(value);

			UInt shift = kShiftStart;

			REFLEX_LOOP_PTR(buffer.data, ptr, kSize)
			{
				UInt nibble = UInt((v >> shift) & 0xF);

				*ptr = char(nibble < 10 ? ('0' + nibble) : ('a' + (nibble - 10)));

				shift -= 4;
			}

			return kSize;	//return number of characters written
		}
	}
};

template <> struct Stringizer <char>
{
	static UInt Call(ArrayRegion <char> & buffer, char value)
	{
		if (buffer)
		{
			buffer[0] = value;

			return 1;
		}

		return 0;
	}
};

template <class TYPE>
struct Stringizer < ArrayView <TYPE> >
{
	static UInt Call(ArrayRegion <char> & buffer, const ArrayView <TYPE> & values)
	{
		auto itr = buffer;

		WriteString(itr, '[');

		if (values)
		{
			auto first_last = ReverseSplice(values, 1);

			for (auto & i : first_last.a)
			{
				WriteString(itr, i);

				WriteString(itr, kCommaSpace);
			}

			WriteString(itr, first_last.b.GetFirst());
		}

		WriteString(itr, ']');

		return buffer.size - itr.size;
	}
};

template <class ... VARGS> struct Stringizer < Tuple <VARGS...> >
{
	using Tuple = Tuple<VARGS...>;

	template <UInt IDX> static void Traverse(ArrayRegion <char> & buffer, const Tuple & t)
	{
		if constexpr (IDX < sizeof...(VARGS))
		{
			WriteString(buffer, TupleElement<const Tuple,IDX>::Get(t));

			if constexpr (IDX + 1 < sizeof...(VARGS)) WriteString(buffer, kCommaSpace);

			Traverse<IDX + 1>(buffer, t);
		}
	}

	static UInt Call(ArrayRegion <char> & buffer, const Tuple & values)
	{
		auto itr = buffer;

		WriteString(itr, '<');

		Traverse<0>(itr, values);

		WriteString(itr, '>');

		return buffer.size - itr.size;
	}
};

template <> struct Stringizer <CString::View>
{
	static UInt Call(ArrayRegion <char> & buffer, const CString::View & value);
};

template <> struct Stringizer <WString::View>
{
	static UInt Call(ArrayRegion <char> & buffer, const WString::View & value);
};

template <> struct Stringizer <UInt64>
{
	static UInt Call(ArrayRegion <char> & buffer, UInt64 value);
};

template <> struct Stringizer <Int64>
{
	static UInt Call(ArrayRegion <char> & buffer, Int64 value);
};

template <> struct Stringizer <Float64>
{
	static UInt Call(ArrayRegion <char> & buffer, Float64 value);
};

//Convert: reduce inputs to limited set, so we dont need many Stringizers

inline CString::View Convert(bool value) { return kFalseTrue[value]; }

inline CString::View Convert(const CString & value) { return value; }
inline CString::View Convert(const char * value) { return value; }

inline WString::View Convert(const WString & value) { return value; }
inline WString::View Convert(const WChar * value) { return value; }

inline UInt64 Convert(UInt32 value) { return value; }
inline UInt64 Convert(UInt16 value) { return value; }
inline UInt64 Convert(UInt8 value) { return value; }

inline Int64 Convert(Int32 value) { return value; }
inline Int64 Convert(Int16 value) { return value; }
inline Int64 Convert(Int8 value) { return value; }

inline Float64 Convert(Float32 value) { return value; }

template <class TYPE> inline ArrayView <TYPE> Convert(const Array <TYPE> & value) { return value; }

template <class TYPE> inline const void * Convert(TYPE * value) { return value; }

template <class TYPE> inline decltype (auto) Convert(const TYPE & value)
{
	if constexpr (kIsObject<TYPE>)
	{
		return MakeTuple(value.object_t->tname, &value);
	}
	else
	{
		return value;
	}
}


//actual handler

extern char g_debug_scratch[8][256];

extern UInt g_debug_scratch_idx;

template <class TYPE> inline UInt WriteString(ArrayRegion <char> & buffer, const TYPE & arg)
{
	using Type = decltype(Convert(arg));

	auto n = Stringizer<NonConstT<NonRefT<Type>>>::Call(buffer, Convert(arg));

	buffer = Nudge(buffer, n);

	return n;
}

template <class TYPE> inline void WriteStringDelimited(ArrayRegion <char> & buffer, const TYPE & arg, const CString::View & delim)
{
	WriteString(buffer, arg);

	WriteString(buffer, delim);
}

//varadic handler

inline CString::View DebugJoin(const CString::View & delim, const char * arg)
{
	return arg;
}

inline CString::View DebugJoin(const CString::View & delim, const CString::View & arg)
{
	return arg;
}

template <typename... VARGS> inline CString::View DebugJoin(const CString::View & delim, const VARGS & ... args)
{
	ArrayRegion <char> buffer = { g_debug_scratch[g_debug_scratch_idx++ & 7], sizeof(g_debug_scratch[0]) - 1 };

	auto itr = buffer;

	(WriteStringDelimited(itr, args, delim), ...);

	CString::View output = { buffer.data, buffer.size - itr.size };

	output.size -= Min(output.size, delim.size);

	if constexpr (REFLEX_DEBUG)
	{
		//prevent out of bounds assertion (which is invalid because we reserved the last character for null)

		buffer.size = sizeof(g_debug_scratch[0]);
	}

	buffer[output.size] = 0;	//null terminate

	return output;
}

REFLEX_END

REFLEX_INLINE Reflex::Output::Buffer::Buffer()
	: size(0)
{
	data[0] = 0;
}

REFLEX_INLINE Reflex::Output::Buffer::Buffer(const CString::View & ref)
	: size(UInt8(Min<UInt>(GetArraySize(data), ref.size)))
{
	MemCopy(ref.data, data, size);
}

template <typename... VARGS> inline void Reflex::Output::LogEx(LogType type, const CString::View & delim, VARGS && ... args)
{
	(*m_output)(*this, type, Detail::DebugJoin(delim, std::forward<VARGS>(args)...));
}

template <typename... VARGS> inline void Reflex::Output::Log(VARGS &&... args)
{
	LogEx(kLogNormal, kSpace, std::forward<VARGS>(args)...);
}

template <typename... VARGS> inline void Reflex::Output::Warn(VARGS &&... args)
{
	LogEx(kLogWarning, kSpace, std::forward<VARGS>(args)...);
}

template <typename... VARGS> inline void Reflex::Output::Error(VARGS &&... args)
{
	LogEx(kLogError, kSpace, std::forward<VARGS>(args)...);
}
