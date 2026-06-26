#include "core/stream.h"

REFLEX_BEGIN_INTERNAL(Reflex::VM)

constexpr UInt32 kDataNamespace = K32("Data");

REFLEX_END_INTERNAL

#include "data/propertyset.cpp"
#include "data/format.cpp"
#include "data/serialize.cpp"
#include "data/misc.cpp"




//
//TODO

Reflex::TRef <Reflex::VM::ValueArray> Reflex::VM::CreateByteArray(const Data::Archive::View & bytes)
{
	TRef bindings = REFLEX_NULL(Bindings);

	auto rtn = REFLEX_CREATE(ValueArray, bindings->archive_t, bindings->uint8_t);

	MemCopy(bytes.data, rtn->Extend<UInt8>(bytes.size), bytes.size);

	return rtn;
}

void Reflex::VM::Append(ValueArray & archive, const Data::Archive::View & bytes)
{
	MemCopy(bytes.data, archive.Extend<UInt8>(bytes.size), bytes.size);
}
