#include "../../../include/reflex/file/detail/memory_stream.h"
#include "../../../include/reflex/file/functions.h"




//
//instream

REFLEX_BEGIN_INTERNAL(Reflex::File)

struct MemoryObjectReader : public Detail::MemoryReader
{
	MemoryObjectReader(const Data::ArchiveObject & data)
		: MemoryReader(data.value),
		m_data(data)
	{
	}

	ConstReference <Data::ArchiveObject> m_data;
};

struct MemoryObjectWriter : public Detail::MemoryWriter
{
	MemoryObjectWriter(Data::ArchiveObject & data)
		: MemoryWriter(data.value),
		m_data(data)
	{
	}

	Reference <Data::ArchiveObject> m_data;
};

REFLEX_END_INTERNAL

Reflex::File::Detail::MemoryReader::MemoryReader(const Data::Archive::View & data)
	: MemoryFile(false),
	m_start(data.data),
	m_end(m_start + data.size),
	m_ptr(m_start)
{
}

Reflex::UInt64 Reflex::File::Detail::MemoryReader::GetSize() const
{
	return m_end - m_start;
}

void Reflex::File::Detail::MemoryReader::SetPosition(UInt64 position)
{
	m_ptr = m_start + position;

	REFLEX_ASSERT(m_ptr <= m_end);
}

Reflex::UInt64 Reflex::File::Detail::MemoryReader::GetPosition() const
{
	return m_ptr - m_start;
}

Reflex::UInt Reflex::File::Detail::MemoryReader::Read(void * dest, UInt size)
{
	size = Min(size, UInt32(m_end - m_ptr));

	MemCopy(m_ptr, Cast<UInt8>(dest), size);

	m_ptr += size;

	return size;
}

Reflex::UInt32 Reflex::File::Detail::MemoryReader::Write(const void * data, UInt size)
{
	REFLEX_ASSERT(false);

	return 0;
}

Reflex::File::Detail::MemoryWriter::MemoryWriter(Data::Archive & data)
	: MemoryFile(true),
	m_dataref(data),
	m_position(data.GetSize())
{
}

Reflex::UInt64 Reflex::File::Detail::MemoryWriter::GetSize() const
{
	return m_dataref.GetSize();
}

void Reflex::File::Detail::MemoryWriter::SetPosition(UInt64 position)
{
	m_position = Min(UInt32(position), m_dataref.GetSize());
}

Reflex::UInt64 Reflex::File::Detail::MemoryWriter::GetPosition() const 
{ 
	return m_position; 
}

Reflex::UInt32 Reflex::File::Detail::MemoryWriter::Read(void * ptr, UInt32 size)
{
	size = Min(size, m_dataref.GetSize() - m_position);

	MemCopy(m_dataref.GetData() + m_position, ptr, size);

	m_position += size;

	return size;
}

Reflex::UInt32 Reflex::File::Detail::MemoryWriter::Write(const void * bytes, UInt num_bytes)
{
	Data::Archive::View chunk = { Cast<UInt8>(bytes), num_bytes };

	if (m_position < m_dataref.GetSize())
	{
		auto ninside = m_dataref.GetSize() - m_position;

		auto overwrite = Min(chunk.size, ninside);

		MemCopy(chunk.data, m_dataref.GetData() + m_position, overwrite);

		chunk.data += overwrite;

		chunk.size -= overwrite;
	}

	m_dataref.Append(Reinterpret<Data::Archive::View>(chunk));

	m_position += num_bytes;

	return num_bytes;
}

bool Reflex::File::Detail::MemoryWriter::Truncate()
{
	m_dataref.SetSize(m_position);

	return true;
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::Detail::CreateMemoryReader(const Data::Archive::View & data)
{
	return REFLEX_CREATE(Detail::MemoryReader, data);
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::CreateMemoryReader(ConstTRef <Data::ArchiveObject> data)
{
	return REFLEX_CREATE(MemoryObjectReader, data);
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::CreateMemoryWriter(TRef <Data::ArchiveObject> data)
{
	return REFLEX_CREATE(MemoryObjectWriter, data);
}
