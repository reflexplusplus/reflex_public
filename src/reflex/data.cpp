#include "../../include/reflex/data/[require].h"

REFLEX_NS(Reflex::File)

extern Output output;

REFLEX_END

#include "data/library.cpp"

#include "data/serialisation.cpp"

#include "data/hash.cpp"
#include "data/hex.cpp"

#include "data/propertyset.cpp"
#include "data/format.cpp"
#include "data/formats.cpp"

#include "data/sha1.cpp"
#include "data/sha256.cpp"

#include "data/compress.cpp"

#include "data/interfaces/streamable.cpp"

#ifndef REFLEX_MINIMAL
#include "data/tokeniser.cpp"
#include "data/interfaces/history.cpp"
#endif

#include "data/functions.cpp"
#include "data/string.cpp"

void Reflex::Data::Detail::RestoreLegacyWString(Archive::View & stream, WString & out)
{
	UInt32 length = Data::Deserialize<UInt32>(stream);

	auto chars = Data::ReadRawArray<UInt16>(stream, length + 1);

	chars.size--;

	out.Allocate(chars.size);

	out.Clear();

	for (auto & i : chars) out.Push(WChar(i));
}
