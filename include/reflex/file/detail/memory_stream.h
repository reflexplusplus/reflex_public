#pragma once

#include "../[require].h"




//
//Detail

REFLEX_NS(Reflex::File::Detail)

struct MemoryFile : public Reflex::System::FileHandle
{
	MemoryFile(bool writeable) : writeable(writeable) {}

	virtual bool Status() const override { return true; }

	virtual bool IsWriteable() const override { return writeable; }

	virtual void Flush() override {}

	virtual bool Truncate() override { return true; } //{ REFLEX_ASSERT(false); }

	const bool writeable;
};

struct MemoryReader : public MemoryFile
{
	MemoryReader(const Data::Archive::View & data = {});

	virtual UInt64 GetSize() const override;

	virtual void SetPosition(UInt64 position) override;

	virtual UInt64 GetPosition() const override;

	virtual UInt Read(void * dest, UInt size) override;

	virtual UInt32 Write(const void * data, UInt size) override;


	const UInt8 * m_start;

	const UInt8 * m_end;

	const UInt8 * m_ptr;
};

struct MemoryWriter : public MemoryFile
{
	MemoryWriter(Data::Archive & data);

	virtual UInt64 GetSize() const override;

	virtual void SetPosition(UInt64 position) override;

	virtual UInt64 GetPosition() const override;

	virtual UInt32 Read(void * ptr, UInt32 size) override;

	virtual UInt32 Write(const void * bytes, UInt num_bytes) override;

	virtual bool Truncate() override;


	Data::Archive & m_dataref;

	UInt m_position;
};

REFLEX_END
