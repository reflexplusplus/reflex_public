#pragma once

#include "../[require].h"




//
//Detail

REFLEX_NS(Reflex::File::Detail)

struct MemoryFile : public Reflex::System::FileHandle
{
	MemoryFile(bool writeable) : writeable(writeable) {}

	bool IsWriteable() const override { return writeable; }

	bool Flush(bool commit) override { return true; }

	bool Truncate() override { return true; }

	const bool writeable;
};

struct MemoryReader : public MemoryFile
{
	MemoryReader(const Data::Archive::View & data = {});

	UInt64 GetSize() const override;

	void SetPosition(UInt64 position) override;

	UInt64 GetPosition() const override;

	UInt Read(void * dest, UInt size) override;

	UInt32 Write(const void * data, UInt size) override;


	const UInt8 * m_start;

	const UInt8 * m_end;

	const UInt8 * m_ptr;
};

struct MemoryWriter : public MemoryFile
{
	MemoryWriter(Data::Archive & data);

	UInt64 GetSize() const override;

	void SetPosition(UInt64 position) override;

	UInt64 GetPosition() const override;

	UInt32 Read(void * ptr, UInt32 size) override;

	UInt32 Write(const void * bytes, UInt num_bytes) override;

	bool Truncate() override;


	Data::Archive & m_dataref;

	UInt m_position;
};

REFLEX_END
