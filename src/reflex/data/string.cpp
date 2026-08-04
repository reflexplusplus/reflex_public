#include "../../../include/reflex/data/functions/string.h"
#include "../../../include/reflex/data/serialisation/pack.h"




//
//implementation

void Reflex::Data::WriteLine(Archive & archive, const CString::View & line)
{
	archive.Allocate(archive.GetSize() + line.size + 1);

	archive.Append<kAllocateNone>(Pack(line));

	archive.Push<kAllocateNone>(10);
}

void Reflex::Data::WriteLine(Archive & output, const WString::View & line)
{
	output.Append(EncodeUTF8(line));

	output.Push(10);
}
