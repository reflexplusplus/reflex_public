#include "../../../include/reflex/data/serialisation/string.h"




//
//

void Reflex::Data::EncodeUCS2(Archive & output, const WString::View & text)
{
	if constexpr (sizeof(WChar) == 2)
	{
		output.Append({ Reinterpret<UInt8>(text.data), UInt(text.size * sizeof(WChar)) });
	}
	else
	{
		auto wr = Reinterpret<UInt16>(Extend(output, text.size * 2).data);

		for (auto & i : text)
		{
			*wr++ = UInt16(i);
		}
	}
}

void Reflex::Data::DecodeUCS2(WString & text, const Archive::View & ucs16)
{
	REFLEX_ASSERT(!(ucs16.size & 1));

	auto length = ucs16.size / 2;

	if constexpr (sizeof(WChar) == 2)
	{
		text.Append({ Reinterpret<WChar>(ucs16.data), length });
	}
	else
	{
		auto wr = Extend(text, length).data;

		REFLEX_LOOP_PTR(Reinterpret<UInt16>(ucs16.data), rd, length)
		{
			*wr++ = WChar(*rd);
		}
	}
}

void Reflex::Data::SerializeUTF8(Archive & stream, const WString::View & string)
{
	Marker <UInt16> marker(stream);

	EncodeUTF8(stream, string);

	REFLEX_ASSERT(marker.GetDelta() <= kMaxUInt16);

	marker.Set(UInt16(marker.GetDelta()));
}

void Reflex::Data::DeserializeUTF8(Archive::View & stream, WString & string)
{
	auto bytes = ReadBytes(stream, Deserialize<UInt16>(stream));

	string.Clear();

	DecodeUTF8(string, bytes);
}

Reflex::WString Reflex::Data::DeserializeUTF8(Archive::View & stream)
{
	WString rtn;

	DeserializeUTF8(stream, rtn);

	return rtn;
}

void Reflex::Data::SerializeUCS2(Archive & stream, const WString::View & string)
{
	Serialize(stream, UInt16(string.size));

	EncodeUCS2(stream, string);
}

void Reflex::Data::DeserializeUCS2(Archive::View & stream, WString & string)
{
	auto bytes = ReadBytes(stream, Deserialize<UInt16>(stream) * 2);

	string.Clear();

	DecodeUCS2(string, bytes);
}

Reflex::WString Reflex::Data::DeserializeUCS2(Archive::View & stream)
{
	WString rtn;

	DeserializeUCS2(stream, rtn);

	return rtn;
}
