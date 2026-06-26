#include "utf8.h"




//
//impl

REFLEX_BEGIN_INTERNAL(Reflex::System::Common)

typedef Array <UInt8> ArchiveT;

struct ArrayAccessor
{
	void * m_allocator;

	void * m_ptr;

	UInt32 m_capacity;

	UInt32 m_size;
};

ArchiveT & CastToArchive(CString & cstring)
{
	auto & a = Reinterpret<ArrayAccessor>(cstring);

	a.m_capacity++;

	a.m_size++;

	return Reinterpret<ArchiveT>(a);
}

void RestoreCString(ArchiveT & archive)
{
	REFLEX_ASSERT(archive && archive.GetLast() == 0);

	auto & string = Reinterpret<ArrayAccessor>(archive);

	string.m_capacity--;

	string.m_size--;
}

REFLEX_END_INTERNAL

Reflex::System::Common::UTF8 Reflex::System::Common::ToUTF8(const WString::View & string)
{
	UTF8 output;

	auto & buffer = CastToArchive(output);

	buffer.Clear();
	
	Data::EncodeUTF8(buffer, string);

	buffer.Push(char(0));

	RestoreCString(buffer);

	return output;
}
