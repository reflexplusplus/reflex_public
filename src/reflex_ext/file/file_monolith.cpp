#include "../../../include/reflex_ext/file/monolith.h"
#include "../../../include/reflex_ext/file/functions.h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::File)

UInt16 g_ignore_count = 0;

struct FileRegion : public System::FileHandle
{
	FileRegion(const Object & owner, UInt16 & count, System::FileHandle & file, UInt start, UInt length)
		: m_owner(owner)
		, count(count)
		, m_file(file)
		, m_offset(start)
		, m_size(length)
		, m_position(0)
	{
		REFLEX_ASSERT((start + length) <= m_file->GetSize());

		count++;

		m_file->SetPosition(start);
	}

	~FileRegion()
	{
		count--;
	}

	bool IsWriteable() const override { return false; }

	UInt64 GetSize() const override { return m_size; }

	void SetPosition(UInt64 position) override
	{
		REFLEX_ASSERT(position <= m_size);

		m_position = UInt32(position);

		return m_file->SetPosition(m_offset + position);
	}

	UInt64 GetPosition() const override
	{
		RemoveConst(m_position) = UInt32(m_file->GetPosition()) - m_offset;

		return m_position;
	}

	UInt32 Read(void * ptr, UInt32 size) override
	{
		REFLEX_ASSERT(size <= m_size);

		size = Min(size, m_size - m_position);

		auto read = m_file->Read(ptr, size);

		m_position += read;

		return read;
	}

	UInt32 Write(const void * data, UInt size) override
	{
		REFLEX_ASSERT(false);

		return 0;
	}

	bool Flush(bool commit) override { return m_file->Flush(commit); }

	bool Truncate() override { return true; }


	ConstReference <Object> m_owner;

	UInt16 & count;

	TRef <System::FileHandle> m_file;

	UInt m_offset, m_size, m_position;
};

struct FileRegionWriter : public FileRegion
{
	using FileRegion::FileRegion;

	bool IsWriteable() const override { return m_file->IsWriteable(); }

	UInt32 Write(const void * pdata, UInt size) override
	{
		size = Min(size, m_size - m_position);

		m_file->Write(pdata, size);

		m_position += size;

		return size;
	}
};

struct MonolithImpl : public Monolith
{
	struct Header
	{
		UInt32 monolithid;
		UInt32 clientheader;
		UInt32 version_reserved;
	};

	struct PartitionInfo
	{
		UInt32 position;
		UInt32 size;
	};


	typedef const PartitionInfo * PartitionRef;


	MonolithImpl(System::FileHandle & file, UInt32 clientheader);


	bool IsWriteable() const override;

	bool Status() const override;

	void Enumerate(const Function <void(Key64,UInt32)> & callback) const override;

	void Clear() override;

	bool Remove(Key64 partitionid) override;

	TRef <System::FileHandle> Write(Key64 partitionid, UInt size) override;

	TRef <System::FileHandle> Read(Key64 partitionid) const override;

	void Commit() override;


	Sequence <UInt, Pair <UInt8, UInt> > Analyse() const;

	bool Validate() const;


	TRef <System::FileHandle> Read_ReadPartition(const PartitionInfo & partitionref) const;


	PartitionRef Write_CreatePartition(UInt64 id, UInt size);

	PartitionRef Write_ExpandPartition(Idx idx, UInt size);

	void Write_Shrink(PartitionInfo & partition, UInt size);

	PartitionRef Write_RetrievePartition(UInt64 partitionid, UInt size);


	UInt GetSize();



	Reference <System::FileHandle> m_stream;

	const UInt32 m_clientheader;

	bool m_writeable;

	bool m_status;

	mutable UInt16 m_nstreams;

	PartitionInfo m_index;

	Sequence <UInt64,PartitionInfo> m_partitions;

	Sequence <UInt, UInt> m_deleted;


	static const UInt32 kVersion;

	static const UInt64 kLegacyHeaderID;

	static const System::FileHandle::Mode kOpenModes[];

};

const UInt64 MonolithImpl::kLegacyHeaderID = 7526756791689834317ull;	//id64("Monolith");

const UInt32 MonolithImpl::kVersion = 1;

const System::FileHandle::Mode MonolithImpl::kOpenModes[] = { System::FileHandle::kModeRead, System::FileHandle::kModeAppend };

MonolithImpl::MonolithImpl(System::FileHandle & file, UInt32 clientheader)
	: m_stream(file),
	m_clientheader(clientheader),
	m_writeable(file.IsWriteable()),
	m_status(false),
	m_nstreams(0)
{
	typedef Header Header;

	REFLEX_STATIC_ASSERT(sizeof(Header) == 12);


	//restore

	auto & stream = *m_stream;

	stream.SetPosition(0);

	if (stream.GetSize() > sizeof(Header))
	{
		Header header;

		stream.Read(&header, sizeof(Header));

		if (header.monolithid == kHeader && header.version_reserved == kVersion)
		{
			//m_write_count = header.writecount;

			m_status = header.clientheader == m_clientheader;

			ReadValue(stream, m_index);

			stream.SetPosition(m_index.position);

			auto archive = ReadBytes(stream, m_index.size);

			auto stream = ToView(archive);

			Data::Deserialize(stream, m_partitions, m_deleted);

			if (m_status)
			{
				Validate();
			}
			else
			{
				Clear();

				m_status = m_writeable;
			}

			return;
		}
		else if (Reinterpret<UInt64>(header) == kLegacyHeaderID)
		{
			struct LegacyHeader
			{
				UInt64 monolithid;

				UInt32 version;

				UInt32 clientheader;
			};

			struct LegacyPartition
			{
				UInt32 position;
				UInt32 capacity;
				UInt32 size;
				UInt32 flags;
			};

			REFLEX_STATIC_ASSERT(sizeof(LegacyHeader) == 16);

			LegacyHeader legacy_header;

			MemCopy(&header, &legacy_header, 12);

			ReadValue(stream, legacy_header.clientheader);

			output.Warn("Import Legacy Monolith");

			m_status = true;	// legacyheader.clientheader == m_clientheader;	//CLIENT ID WAS INCONSISTENTLY USED BEFORE

			Sequence <WString,LegacyPartition> partitions_legacy;

			Sequence <UInt,LegacyPartition> deleted;

			auto legacyindex = ReadValue<LegacyPartition>(stream);

			stream.SetPosition(legacyindex.position);

			m_index.position = legacyindex.position;

			m_index.size = legacyindex.capacity;

			auto archive = File::ReadBytes(stream, legacyindex.size);

			auto decoder = ToView(archive);

			WString id;

			REFLEX_LOOP(idx, Data::Deserialize<UInt32>(decoder))
			{
				Data::Detail::RestoreLegacyWString(decoder, id);

				if (legacy_header.version == 0) id = File::CorrectStrokes(id);

				LegacyPartition & partition = partitions_legacy.Insert(id);

				Data::Deserialize(decoder, partition);

				if (legacy_header.version == 0) partition.flags = 1;
			}

			struct EncodableWString : WString
			{
				using WString::WString;

				void Serialize(Data::Archive & stream) const
				{
					Data::SerializeUCS2(stream, *this);
				}

				using WString::operator=;
			};

			Map <Key64,EncodableWString> index;

			for (auto & i : partitions_legacy)
			{
				auto & filename = i.key;

				Key32 key32 = filename;

				auto key64 = Data::Detail::MakeKey64(0, key32.value);

				if (key64 == Data::Detail::MakeKey64(0, K32("<Data>")))
				{
					key64 = Data::Detail::MakeKey64(CC32("data"), 0);
				}
				else
				{
					index[key64] = filename;
				}

				auto & legacypartition = i.value;

				PartitionInfo & partition = m_partitions.Insert(key64);

				partition.position = legacypartition.position;

				partition.size = legacypartition.size;
			}


			//fill gaps (includes fix for old bug where discarded indexes where not added to m_deleted)

			auto sorted = Analyse();

			UInt start = sizeof(Header) + sizeof(PartitionInfo);

			UInt filesize = UInt32(stream.GetSize());

			if (sorted.GetFirst().key > start) m_deleted.Insert(sorted.GetFirst().key - start, start);

			REFLEX_LOOP(idx, sorted.GetSize() - 1)
			{
				auto & item = sorted[idx];

				auto & next = sorted[idx + 1];

				UInt end = item.key + item.value.b;

				if (end < next.key) m_deleted.Insert(next.key - end, end);
			}

			auto & last = sorted.GetLast();

			UInt end = last.key + last.value.b;

			if (end < filesize) m_deleted.Insert(filesize - end, end);


			if (m_writeable)
			{
				Reflex::Detail::SilentReference <MonolithImpl> retain(this);

				WritePartition(*this, Data::Detail::MakeKey64(CC32("indx"), 0), Data::ToBinary(index));
			}

			Validate();

			return;
		}
	}



	//initalise nw monolith

	if (m_writeable)
	{
		Data::Archive archive;
		
		Data::Serialize(archive, m_partitions, m_deleted);

		m_index.position = sizeof(Header) + sizeof(PartitionInfo);

		m_index.size = (sizeof(PartitionInfo) * (REFLEX_DEBUG ? 4 : 16));	//make bigger when stable

		REFLEX_ASSERT(m_index.size > archive.GetSize());

		stream.SetPosition(0);

		Header header = { kHeader, m_clientheader, kVersion };

		WriteValue(stream, header);

		WriteValue(stream, m_index);

		stream.SetPosition(m_index.position);

		WriteBytes(stream, archive);

		UInt end = m_index.position + m_index.size;

		stream.SetPosition(m_index.position + m_index.size - 1);

		WriteValue(stream, UInt8(0));
		
		UInt filesize = UInt32(stream.GetSize());

		if (filesize > end) m_deleted.Insert(filesize - end, end);

		stream.Flush(false);

		m_status = IsValid(stream);

		m_writeable = m_status;
	}

	Validate();
}

bool MonolithImpl::IsWriteable() const
{
	return m_writeable;
}

bool MonolithImpl::Status() const
{
	return m_status;
}

void MonolithImpl::Clear()
{
	for (auto & i : m_partitions)
	{
		auto & partition = i.value;

		if (partition.size) m_deleted.Insert(partition.size, partition.position);
	}

	m_partitions.Clear();

	Validate();
}

bool MonolithImpl::Remove(Key64 partitionid)
{
	if (Idx idx = m_partitions.Search(partitionid.value))
	{
		auto & partition = m_partitions[idx.value].value;

		if (partition.size) m_deleted.Insert(partition.size, partition.position);

		m_partitions.Remove(idx.value);

		Commit();

		return true;
	}

	return false;
}

void MonolithImpl::Commit()
{
	REFLEX_ASSERT(!m_nstreams);

	if (m_writeable)
	{
		Data::Archive archive;
		
		Data::Serialize(archive, m_partitions, m_deleted);

		UInt size = archive.GetSize();

		if (size > m_index.size)
		{
			m_deleted.Insert(m_index.size, m_index.position);

			archive.Clear();

			Data::Serialize(archive, m_partitions, m_deleted);

			m_index.position = GetSize();

			while (size > m_index.size) m_index.size *= 2;

			m_stream->SetPosition(m_index.position + m_index.size - 1);

			WriteValue(m_stream, UInt8(0));
		}

		m_stream->SetPosition(0);

		Header header = { kHeader, m_clientheader, kVersion };

		WriteValue(m_stream, MakeTuple(header, m_index));

		m_stream->SetPosition(m_index.position);

		WriteBytes(m_stream, archive);

		m_stream->Flush(false);
	}

	Validate();
}

void MonolithImpl::Enumerate(const Function <void(Key64,UInt32)> & callback) const
{
	Array < Pair <Key64, UInt32> > partitions;

	auto ptr = Extend(partitions, m_partitions.GetSize()).data;

	for (auto & i : m_partitions) *ptr = { i.key, i.value.size };

	for (auto & i : partitions) callback(i.a, i.b);
}

REFLEX_INLINE TRef <System::FileHandle> MonolithImpl::Read_ReadPartition(const PartitionInfo & partitioninfo) const
{
	REFLEX_ASSERT(!m_nstreams);

	return REFLEX_CREATE(FileRegion, *this, m_nstreams, m_stream, partitioninfo.position, partitioninfo.size);
}

TRef <System::FileHandle> MonolithImpl::Read(Key64 partitionid) const
{
	if (auto partitionref = m_partitions.SearchValue(partitionid.value))
	{
		return Read_ReadPartition(*partitionref);
	}
	else
	{
		return {};
	}
}

REFLEX_INLINE void MonolithImpl::Write_Shrink(PartitionInfo & partition, UInt size)
{
	UInt gap = partition.size - size;

	m_deleted.Insert(gap, partition.position + size);

	partition.size = size;
}

REFLEX_INLINE MonolithImpl::PartitionRef MonolithImpl::Write_ExpandPartition(Idx idx, UInt size)
{
	//aquire

	auto item = m_partitions[idx.value];

	auto & from = item.value;

	if (from.size) m_deleted.Insert(from.size, from.position);

	m_partitions.Remove(idx.value);

	PartitionRef to = Write_CreatePartition(item.key, size);


	//move content

	UInt content = Min(from.size, size);

	auto frompos = from.position;

	auto topos = to->position;

	UInt nblock = content / 1024;

	UInt remainder = content - (nblock * 1024);

	UInt8 buffer[1024];

	auto & stream = *m_stream;

	REFLEX_LOOP(y, nblock)
	{
		stream.SetPosition(frompos);

		stream.Read(buffer, 1024);

		frompos += 1024;

		stream.SetPosition(topos);

		stream.Write(buffer, sizeof(buffer));

		topos += 1024;
	}

	stream.SetPosition(frompos);

	stream.Read(buffer, remainder);

	stream.SetPosition(topos);

	stream.Write(buffer, remainder);

	return to;
}

MonolithImpl::PartitionRef MonolithImpl::Write_CreatePartition(UInt64 partitionid, UInt size)
{
	REFLEX_ASSERT(!m_partitions.Search(partitionid));

	auto & partition = m_partitions.Insert(partitionid);

	partition.size = size;

	if (Idx index = m_deleted.SearchGTE(size))
	{
		auto item = m_deleted[index.value];

		m_deleted.Remove(index.value);

		partition.position = item.value;

		if (item.key > size)						//split
		{
			m_deleted.Insert(item.key - size, item.value + size);
		}
	}
	else
	{
		partition.position = GetSize();

		if (size)
		{
			auto & stream = *m_stream;

			stream.SetPosition(partition.position + size - 1);

			WriteValue(stream, UInt8(0));
		}
	}

	return &partition;
}

REFLEX_INLINE MonolithImpl::PartitionRef MonolithImpl::Write_RetrievePartition(UInt64 partitionid, UInt size)
{
	PartitionRef partitionref = 0;

	if (auto idx = m_partitions.Search(partitionid))								//already exists
	{
		partitionref = &m_partitions[idx.value].value;

		if (size > partitionref->size)
		{
			partitionref = Write_ExpandPartition(idx, size);
		}
		else if (size < partitionref->size)
		{
			Write_Shrink(RemoveConst(*partitionref), size);
		}
		else
		{
			return partitionref;
		}
	}
	else																//newexists
	{
		partitionref = Write_CreatePartition(partitionid, size);
	}

	Commit();

	return partitionref;
}

TRef <System::FileHandle> MonolithImpl::Write(Key64 partitionid, UInt size)
{
	REFLEX_ASSERT(!m_nstreams);
	
	if (m_nstreams)
	{
		return {};
	}
	else
	{
		auto partitionref = Write_RetrievePartition(partitionid.value, size);

		return REFLEX_CREATE(FileRegionWriter, *this, m_nstreams, m_stream, partitionref->position, partitionref->size);
	}
}

UInt MonolithImpl::GetSize()
{
	m_stream->Flush(false);

	return UInt32(m_stream->GetSize());
}

Sequence <UInt, Pair <UInt8, UInt> > MonolithImpl::Analyse() const
{
	Sequence <UInt, Pair <UInt8, UInt> > sorted;

	sorted.Insert(m_index.position, { 1, m_index.size });

	for (auto & i : m_partitions)
	{
		auto & pinfo = i.value;

		sorted.Insert(pinfo.position, { 2, pinfo.size });
	}

	for (auto & i : m_deleted)
	{
		sorted.Insert(i.value, { 0, i.key });
	}

	return sorted;
}

bool MonolithImpl::Validate() const
{
	if (REFLEX_DEBUG)
	{
		auto sorted = Analyse();

		if (m_status && sorted)
		{
			REFLEX_LOOP(idx, sorted.GetSize() - 1)
			{
				auto & current = sorted[idx];

				if (current.value.b)
				{
					auto & next = sorted[idx + 1];

					if (current.key + current.value.b != next.key) output.Error("Monolith::Validate");
				}
			}
		}
	}

	return true;
}

struct NullMonolith : public Monolith
{
	bool IsWriteable() const override { return false; }
	bool Status() const override { return false; }
	void Clear() override {}
	bool Remove(Key64 partitionid) override { return false; }
	void Commit() override {}
	void Enumerate(const Function <void(Key64,UInt32)> & callback) const override {};
	TRef <System::FileHandle> Read(Key64 partitionid) const override { return {}; }
	TRef <System::FileHandle> Write(Key64 partitionid, UInt size) override { return {}; }
} 
g_null_monolith;

REFLEX_END_INTERNAL

const Reflex::UInt32 Reflex::File::Monolith::kHeader = K32("monolith");

Reflex::File::Monolith & Reflex::File::Monolith::null = Reflex::File::g_null_monolith;

Reflex::TRef <Reflex::File::Monolith> Reflex::File::Monolith::Create(System::FileHandle & file, UInt32 clientheader)
{
	return REFLEX_CREATE(MonolithImpl, file, clientheader);
}

Reflex::TRef <Reflex::File::Monolith> Reflex::File::Monolith::Create(const WString::View & filename, UInt32 clientheader, bool write)
{
	auto file = System::FileHandle::Create(filename, MonolithImpl::kOpenModes[write]);

	return REFLEX_CREATE(MonolithImpl, file, clientheader);
}

Reflex::TRef <Reflex::System::FileHandle> Reflex::File::CreateRegionReader(System::FileHandle & file, UInt start, UInt range)
{
	return REFLEX_CREATE(FileRegion, file, g_ignore_count, file, start, range);
}
