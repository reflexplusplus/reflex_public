#include "core.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

REFLEX_INLINE UInt32 Read(const UInt8 * & ptr, const UInt8 * end, UInt n)
{
	if (n > UInt(end - ptr))
	{
		ptr = end;

		return 0;
	}
	else
	{
		UInt32 word = 0;

		REFLEX_LOOP(idx, n) Reinterpret<UInt8>(&word)[idx] = *ptr++;

		return word;
	}
}

template <class CHARACTER> REFLEX_INLINE ArrayView <CHARACTER> ReadLineImpl(ArrayView <CHARACTER> & stream)
{
	ArrayView <CHARACTER> line = { stream.data, 0 };

	CHARACTER character;

	while (stream)
	{
		character = *stream.data;

		stream = Nudge(stream);

		if (character == 10)
		{
			return line;
		}
		else if (character == 13)
		{
			if (stream && *stream.data == 10)
			{
				stream = Nudge(stream);
			}

			return line;
		}

		line.size++;
	}

	return line;
}

REFLEX_END_INTERNAL

Reflex::CString::View Reflex::Data::Detail::ReadLine(CString::View & stream)
{
	return ReadLineImpl(stream);
}

Reflex::WString::View Reflex::Data::Detail::ReadLine(WString::View & stream)
{
	return ReadLineImpl(stream);
}

void Reflex::Data::EncodeUTF8(Array <UInt8> & value, const WString::View & wstring)
{
	UInt length = wstring.size;

	value.Allocate(value.GetSize() + ((length + 1) * 2));

	REFLEX_LOOP_PTR(wstring.data, ptr, wstring.size)
	{
		WChar character = *ptr;

		if (character >= 0xd800 && character <= 0xdbff)
		{
			//codepoint = ((character - 0xd800) << 10) + 0x10000;
		}
		else
		{
			UInt codepoint = 0;

			if (character >= 0xdc00 && character <= 0xdfff)
			{
				codepoint |= character - 0xdc00;
			}
			else
			{
				codepoint = character;
			}

			if (codepoint <= 0x7f)
			{
				value.Push(static_cast<char>(codepoint));
			}
			else if (codepoint <= 0x7ff)
			{
				auto ptr = Extend(value, 2);

				ptr[0] = (static_cast<char>(0xc0 | ((codepoint >> 6) & 0x1f)));
				ptr[1] = (static_cast<char>(0x80 | (codepoint & 0x3f)));
			}
			else if (codepoint <= 0xffff)
			{
				auto ptr = Extend(value, 3);

				ptr[0] = (static_cast<char>(0xe0 | ((codepoint >> 12) & 0x0f)));
				ptr[1] = (static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
				ptr[2] = (static_cast<char>(0x80 | (codepoint & 0x3f)));
			}
			else
			{
				auto ptr = Extend(value, 4);

				ptr[0] = (static_cast<char>(0xf0 | ((codepoint >> 18) & 0x07)));
				ptr[1] = (static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
				ptr[2] = (static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
				ptr[3] = (static_cast<char>(0x80 | (codepoint & 0x3f)));
			}
		}
	}
}

void Reflex::Data::DecodeUTF8(WString & output, const ArrayView <UInt8> & input)
{
	REFLEX_ASSERT(!output.GetSize());

	output.Clear();

	UInt8 u248 = 248;	//MakeBits(5);

	UInt8 u240 = 240;	//MakeBits(4);

	UInt8 u224 = 224;	//MakeBits(3);

	UInt8 u192 = 192;	//MakeBits(2);

	const UInt8 * in = input.data;

	const UInt8 * end = in + input.size;

	while (in < end)
	{
		const UInt8 & byte = *in;

		WChar c = 0;

		if ((byte & u248) == u240)
		{
			//todo support this block, for WString32

			//UInt word = Read(in, end, 4);

			return;
		}
		else if ((byte & u240) == u224)
		{
			UInt word = Read(in, end, 3);

			auto bytes = Reinterpret<UInt8>(&word);

			UInt32 first = bytes[0] & (~u240);

			UInt32 second = bytes[1] & (~u192);

			UInt32 third = bytes[2] & (~u192);

			c = WChar((first << 12) | (second << 6) | third);

			output.Push(c);
		}
		else if ((byte & u224) == u192)
		{
			UInt word = Read(in, end, 2);

			auto bytes = Reinterpret<UInt8>(&word);

			UInt32 first = bytes[0] & (~u224);

			UInt32 second = bytes[1] & (~u192);

			c = WChar((first << 6) | second);

			output.Push(c);
		}
		else
		{
			c = *in++;

			output.Push(c);
		}
	}
}

Reflex::Pair <Reflex::WString::View> Reflex::File::SplitExtension(const WString::View & path)
{
	if (auto idx = ReverseSearch(path, L'.'))
	{
		auto pair = Splice(path, idx.value + 1);

		pair.a.size--;

		return pair;
	}

	return { path, {} };
}

void Reflex::File::Detail::CorrectStrokes(WString & path)
{
	Reflex::Detail::ReplaceElement<StandardCompare>(path, WChar(92), System::kPathDelimiter);
}

void Reflex::File::Detail::CorrectTrailingStroke(WString & path)
{
	if (path && path.GetLast() != System::kPathDelimiter)
	{
		path.Push(System::kPathDelimiter);
	}
}

void Reflex::File::Detail::RemoveTrailingStroke(WString & path)
{
	if (path && path.GetLast() == System::kPathDelimiter)
	{
		path.Pop();
	}
}
