#include "impl.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

REFLEX_INLINE void BuildHeapColumns(const Array <Table::ColumnInfo> & columns, Array <const Table::ColumnInfo*> & heap_columns)
{
	REFLEX_ASSERT(heap_columns.Empty());

	for (auto & column : columns)
	{
		if (Detail::kColumnTypeToRawDataType[column.type] == Detail::kRawDataTypeVariable)
		{
			heap_columns.Push(&column);
		}
	}
}

REFLEX_INLINE void InitialiseNullRowHeapCells(const Array <const Table::ColumnInfo*> & heap_columns, Data::Archive & nullrow)
{
	for (auto & column : heap_columns)
	{
		*Reinterpret<UInt32>(nullrow.GetData() + column->offset) = kNullHeapAdr;
	}
}

REFLEX_INLINE bool IsValidHeapAlignment(UInt8 alignment)
{
	return alignment && ((alignment & (alignment - 1)) == 0) && (alignment <= 8);
}

REFLEX_INLINE UInt32 EncodeHeapKey(UInt8 alignment, UInt16 size)
{
	REFLEX_ASSERT(IsValidHeapAlignment(alignment));

	return (UInt32(alignment) << 16) | size;
}

REFLEX_INLINE UInt8 DecodeHeapAlignment(UInt32 key)
{
	return UInt8(key >> 16);
}

REFLEX_INLINE UInt GetHeapPayloadOffset(UInt adr, UInt8 alignment)
{
	REFLEX_ASSERT(IsValidHeapAlignment(alignment));

	constexpr UInt kOffset = kSizeOf<HeapBlockHeader> - 1;

	UInt mask = UInt(alignment) - 1;

	return kSizeOf<HeapBlockHeader> + mask - ((adr + kOffset) & mask);
}

REFLEX_INLINE UInt GetHeapBlockSpan(UInt adr, UInt size, UInt8 alignment)
{
	return GetHeapPayloadOffset(adr, alignment) + size;
}

REFLEX_INLINE void InitialiseHeap(Data::Archive & heap)
{
	heap.SetSize(kSizeOf<HeapBlockHeader>);

	auto header = Reinterpret<HeapBlockHeader>(heap.GetData());

	header->size = 0;
	header->alignment = 1;
	header->payload_offset = UInt8(sizeof(HeapBlockHeader));
}

REFLEX_END_INTERNAL

Reflex::Data::MemoryTable::MemoryTable(ArrayView <ColumnInfo> columns)
	: m_has_key32_columns(false)
{
	m_columninfo.SetSize(columns.size);

	auto pcol = m_columninfo.GetData();

	m_nullrow.Allocate(columns.size * 8);

	REFLEX_LOOP_PTR(columns.data, desc, columns.size)
	{
		Table::ColumnType datatype = desc->type;

		*pcol++ = { desc->id, datatype, desc->flags, UInt16(m_nullrow.GetSize()) };

		m_nullrow.Append<kAllocateNone>(Detail::GetNullValue(datatype));

		if (datatype == kColumnTypeKey32)
		{
			m_has_key32_columns = true;
		}
	}

	BuildHeapColumns(m_columninfo, m_heap_columns);

	m_heap.Allocate(REFLEX_DEBUG ? 128 : 4096);

	InitialiseHeap(m_heap);

	InitialiseNullRowHeapCells(m_heap_columns, m_nullrow);

	Publish(m_columninfo, m_nullrow);

	Table::SetData(m_data.GetData(), 0);
}

Reflex::Data::MemoryTable::MemoryTable(System::FileHandle & stream, FileHeader & header)
	: m_has_key32_columns(false)
{
	auto columns = File::ReadBytes(stream, header.ncolumn * 6);

	auto ptr = columns.GetData();

	m_columninfo.SetSize(header.ncolumn);

	m_nullrow.Clear();

	m_nullrow.Allocate(header.ncolumn * 8);

	REFLEX_LOOP(idx, header.ncolumn)
	{
		auto & columninfo = m_columninfo[idx];

		auto data_type = ColumnType(ptr[4]);

		columninfo = { *Reinterpret<Key32>(ptr), data_type, ptr[5], UInt16(m_nullrow.GetSize()) };

		m_nullrow.Append<kAllocateNone>(Detail::GetNullValue(data_type));

		ptr += 6;

		if (data_type == kColumnTypeKey32)
		{
			m_has_key32_columns = true;
		}
	}

	BuildHeapColumns(m_columninfo, m_heap_columns);

	InitialiseNullRowHeapCells(m_heap_columns, m_nullrow);

	auto num_row = header.datasize / m_nullrow.GetSize();

	m_data.Allocate((num_row + 1) * m_nullrow.GetSize());

	m_data = File::ReadBytes(stream, header.datasize);

	m_heap = File::ReadBytes(stream, header.heapsize);

	auto remainder = header.size - ((header.ncolumn * 6) + header.datasize + header.heapsize);

	auto archive = File::ReadBytes(stream, remainder);

	Data::FromBinary(archive, m_freeheap);

	Publish(m_columninfo, m_nullrow);

	SetData(m_data.GetData(), num_row);

	REFLEX_ASSERT(stream.GetPosition() == stream.GetSize());
}

Reflex::Data::MemoryTable::MemoryTable(const MemoryTable & table)
	: m_columninfo(table.m_columninfo)
	, m_nullrow(table.m_nullrow)
	, m_data(table.m_data)
	, m_heap(table.m_heap)
	, m_freeheap(table.m_freeheap)
	, m_has_key32_columns(table.m_has_key32_columns)
{
	BuildHeapColumns(m_columninfo, m_heap_columns);

	Publish(m_columninfo, m_nullrow);

	SetData(m_data.GetData(), table.GetNumRow());
}

REFLEX_NOINLINE void Reflex::Data::MemoryTable::Allocate(UInt nrow)
{
	m_data.Allocate((nrow + 1) * GetRowByteSize());

	Table::SetData(m_data.GetData(), GetNumRow());
}

void Reflex::Data::MemoryTable::Clear()
{
	m_data.Clear();

	InitialiseHeap(m_heap);

	m_freeheap.Clear();

	SetData(m_data.GetData(), 0);
}

void Reflex::Data::MemoryTable::Extend(UInt num_extra_row)
{
	auto bytes = num_extra_row * GetRowByteSize();

	auto top = Reflex::Extend(m_data, bytes).data;
	auto nullrow = GetNullRow();

	REFLEX_LOOP(idx, num_extra_row)
	{
		MemCopy(nullrow.data, top + (idx * nullrow.size), nullrow.size);
	}

	SetData(m_data.GetData(), GetNumRow() + num_extra_row);

	if (m_has_key32_columns)
	{
		Array <UInt> key_cols;

		for (auto & i : GetColumns())
		{
			if (i.type == kColumnTypeKey32)
			{
				key_cols.Push(i.offset);
			}
		}

		while (num_extra_row--)
		{
			for (auto & offset : key_cols)
			{
				*Reinterpret<Key32>(top + offset) = kNullKey;
			}

			top += GetRowByteSize();
		}
	}
}

void Reflex::Data::MemoryTable::Shrink(UInt num_less_row)
{
	num_less_row = Min(num_less_row, GetNumRow());

	if (!num_less_row)
	{
		return;
	}

	auto rowbytes = GetRowByteSize();

	auto first_removed_row = GetNumRow() - num_less_row;

	auto rowptr = m_data.GetData() + (first_removed_row * rowbytes);

	REFLEX_LOOP(idx, num_less_row)
	{
		for (auto & column : m_heap_columns)
		{
			auto adr = Reinterpret<UInt32>(rowptr + column->offset);

			ClearHeapCell(adr, kColumnTypeToHeapAlignment[column->type]);
		}

		rowptr += rowbytes;
	}

	m_data.SetSize(first_removed_row * rowbytes);

	SetData(m_data.GetData(), first_removed_row);
}

Reflex::Data::Table::RowCursor Reflex::Data::MemoryTable::AddRow()
{
	auto empty = GetNullRow();

	auto prow = Reflex::Extend(m_data, empty.size).data;

	MemCopy(empty.data, prow, empty.size);

	SetData(m_data.GetData(), GetNumRow() + 1);

	return { Cast<Table>(this), prow };
}

void Reflex::Data::MemoryTable::RemoveRow(UInt idx)
{
	if (idx < GetNumRow())
	{
		auto rowbytes = GetRowByteSize();

		auto rowptr = m_data.GetData() + (idx * rowbytes);

		{
			auto nullrow = GetNullRow();

			for (auto & column : m_heap_columns)
			{
				auto adr = Reinterpret<UInt32>(rowptr + column->offset);

				ClearHeapCell(adr, kColumnTypeToHeapAlignment[column->type]);
			}

			MemCopy(nullrow.data, rowptr, nullrow.size);
		}

		m_data.Remove(idx * rowbytes, rowbytes);

		SetData(m_data.GetData(), GetNumRow() - 1);
	}
}

void Reflex::Data::MemoryTable::Compact()
{
	Data::Archive heap;

	heap.Allocate(m_heap.GetSize());

	InitialiseHeap(heap);

	auto rowptr = m_data.GetData();

	auto rowbytes = GetRowByteSize();

	REFLEX_LOOP(row, GetNumRow())
	{
		for (auto & column : m_heap_columns)
		{
			auto cell = Reinterpret<UInt32>(rowptr + column->offset);

			auto & adr = *cell;

			if (adr != kNullHeapAdr)
			{
				auto value = ReadHeapCell(cell, kColumnTypeToHeapAlignment[column->type]);

				UInt block_adr = heap.GetSize();

				UInt payload_offset = GetHeapPayloadOffset(block_adr, kColumnTypeToHeapAlignment[column->type]);
				UInt compact_adr = block_adr + payload_offset;

				heap.Expand(payload_offset + value.size);

				auto header = Reinterpret<HeapBlockHeader>(heap.GetData() + compact_adr - kSizeOf<HeapBlockHeader>);

				header->size = UInt16(value.size);
				header->alignment = kColumnTypeToHeapAlignment[column->type];
				header->payload_offset = UInt8(payload_offset);

				MemCopy(value.data, heap.GetData() + compact_adr, value.size);

				adr = UInt32(compact_adr);
			}
		}

		rowptr += rowbytes;
	}

	m_heap = std::move(heap);

	m_freeheap.Clear();

	Notify();
}

Reflex::UInt Reflex::Data::MemoryTable::CalculateStorageSize() const
{
	auto freeheap = Data::ToBinary(m_freeheap);

	return kSizeOf<FileHeader> + (m_columninfo.GetSize() * 6) + m_data.GetSize() + m_heap.GetSize() + freeheap.GetSize();
}

void Reflex::Data::MemoryTable::Serialize(System::FileHandle & stream) const
{
	REFLEX_ASSERT(m_columninfo);

	Data::Archive columns;

	columns.SetSize(m_columninfo.GetSize() * 6);

	auto ptr = columns.GetData();

	for (auto & i : m_columninfo)
	{
		*Reinterpret<UInt32>(ptr) = i.id.value;

		ptr[4] = i.type;
		ptr[5] = i.flags;

		ptr += 6;
	}

	auto freeheap = Data::ToBinary(m_freeheap);

	auto size = CalculateStorageSize();

	FileHeader header = 
	{ 
		.magic = FileHeader::kMagic,
		.size = size - kSizeOf<FileHeader>, 
		.ncolumn = UInt8(m_columninfo.GetSize()), 
		.flags = 0,
		.version = 0,//UInt16(client_version),
		.datasize = m_data.GetSize(), 
		.heapsize = m_heap.GetSize(), 
		.freeheapsize = freeheap.GetSize() 
	};

	File::WriteBytes(stream, Data::Pack(header));

	File::WriteBytes(stream, columns);

	File::WriteBytes(stream, m_data);

	File::WriteBytes(stream, m_heap);

	File::WriteBytes(stream, freeheap);

	REFLEX_ASSERT(size == kSizeOf<FileHeader> + columns.GetSize() + m_data.GetSize() + m_heap.GetSize() + freeheap.GetSize());

	//RemoveConst(Table::client_version) = client_version;
}

void Reflex::Data::MemoryTable::ClearRow(UInt8 * rowptr)
{
	auto nullrow = GetNullRow();

	for (auto & column : m_heap_columns)
	{
		auto adr = Reinterpret<UInt32>(rowptr + column->offset);

		ClearHeapCell(adr, kColumnTypeToHeapAlignment[column->type]);
	}

	MemCopy(nullrow.data, rowptr, nullrow.size);
}

Reflex::Data::Archive::View Reflex::Data::MemoryTable::ReadHeapCell(const UInt32 * cell, UInt8 alignment) const
{
	REFLEX_ASSERT(Inside(Reinterpret<UInt8>(cell), m_data.GetData(), m_data.GetSize()));

	if constexpr (REFLEX_DEBUG)
	{
		auto nullrow = GetNullRow();

		auto offset = UInt(Reinterpret<UInt8>(cell) - m_data.GetData()) % nullrow.size;

		bool ok = false;

		for (auto & i : m_columninfo)
		{
			ok = ok || (i.offset == offset && (Detail::kColumnTypeToRawDataType[i.type] == Detail::kRawDataTypeVariable));
		}

		REFLEX_ASSERT(ok);
	}

	auto adr = *cell;

	auto heap = m_heap.GetData();

	auto header = Reinterpret<HeapBlockHeader>(heap + adr - kSizeOf<HeapBlockHeader>);

	REFLEX_ASSERT(adr == kNullHeapAdr || header->alignment == alignment);

	return { heap + adr, header->size };
}

void Reflex::Data::MemoryTable::ClearHeapCell(UInt32 * cell, UInt8 alignment)
{
	REFLEX_ASSERT(Inside(Reinterpret<UInt8>(cell), m_data.GetData(), m_data.GetSize()));

	auto & adr = *cell;

	if (adr != kNullHeapAdr)
	{
		auto header = Reinterpret<HeapBlockHeader>(m_heap.GetData() + adr - kSizeOf<HeapBlockHeader>);

		REFLEX_ASSERT(header->alignment == alignment);

		auto block_adr = adr - header->payload_offset;

		m_freeheap.Insert(EncodeHeapKey(header->alignment, header->size), block_adr);
	}

	adr = kNullHeapAdr;

	Notify();
}

void Reflex::Data::MemoryTable::SetHeapCell(UInt32 * cell, UInt8 alignment, Data::Archive::View binary)
{
	REFLEX_ASSERT(binary.size <= kMaxUInt16);
	REFLEX_ASSERT(IsValidHeapAlignment(alignment));
	REFLEX_ASSERT(Inside(Reinterpret<UInt8>(cell), m_data.GetData(), m_data.GetSize()));

	if (binary.size <= kMaxUInt16)
	{
		auto & adr = *cell;

		if (adr != kNullHeapAdr)
		{
			auto header = Reinterpret<HeapBlockHeader>(m_heap.GetData() + adr - kSizeOf<HeapBlockHeader>);

			REFLEX_ASSERT(header->alignment == alignment);

			auto block_adr = adr - header->payload_offset;

			m_freeheap.Insert(EncodeHeapKey(header->alignment, header->size), block_adr);
		}

		*cell = kNullHeapAdr;

		if (binary)
		{
			adr = AllocateHeapData(UInt16(binary.size), alignment);

			//*cell = adr; done by ref

			auto pheap = m_heap.GetData();

			auto ptr = pheap + adr;

			MemCopy(binary.data, ptr, binary.size);
		}

		Notify();
	}
}

REFLEX_INLINE Reflex::UInt Reflex::Data::MemoryTable::AllocateHeapData(UInt16 size, UInt8 alignment)
{
	REFLEX_ASSERT(IsValidHeapAlignment(alignment));

	if (Idx result = m_freeheap.SearchGTE(EncodeHeapKey(alignment, size)))
	{
		auto item = m_freeheap[result.value];

		if (DecodeHeapAlignment(item.key) == alignment)
		{
			m_freeheap.Remove(result.value);

			UInt block_adr = item.value;

			UInt payload_offset = GetHeapPayloadOffset(block_adr, alignment);
			UInt adr = block_adr + payload_offset;

			auto header = Reinterpret<HeapBlockHeader>(m_heap.GetData() + adr - kSizeOf<HeapBlockHeader>);

			header->size = size;
			header->alignment = alignment;
			header->payload_offset = UInt8(payload_offset);

			return adr;
		}
	}

	UInt block_adr = m_heap.GetSize();

	UInt block_size = GetHeapBlockSpan(block_adr, size, alignment);

	REFLEX_ASSERT(block_size <= kMaxUInt16);

	m_heap.Expand(block_size);

	UInt payload_offset = GetHeapPayloadOffset(block_adr, alignment);
	UInt adr = block_adr + payload_offset;

	auto header = Reinterpret<HeapBlockHeader>(m_heap.GetData() + adr - kSizeOf<HeapBlockHeader>);

	header->size = size;
	header->alignment = alignment;
	header->payload_offset = UInt8(payload_offset);

	return adr;
}

//template <class UINT> void BuildScalarIndex(const Detail::DataSheetAccessor & sheet, const DataSheet::ColumnInfo & col, Sequence < UInt32, Array <UInt32> > & index)
//{
//	DataSheet::CellPtrImpl <true> itr(sheet, col);
//
//	DataSheet::CellPtrImpl <true> end = itr + sheet.m_nrow;
//
//	UInt row = 0;
//
//	while (itr != end)
//	{
//		index.Retrieve(itr.ReadValue<UINT>()).Push(row++);
//	}
//}
//
//void BuildVariableIndex(const Detail::DataSheetAccessor & sheet, const DataSheet::ColumnInfo & col, Sequence < UInt32, Array <UInt32> > & index)
//{
//	DataSheet::ArrayCellImpl <UInt8,true> itr(sheet, col);
//
//	DataSheet::ArrayCellImpl <UInt8, true> end = itr;// +sheet.m_nrow;
//
//	end += sheet.m_nrow;
//
//	UInt row = 0;
//
//	while (itr != end)
//	{
//		index.Retrieve(Data::FNV1a32(*itr, 0)).Push(row++);
//	}
//}

REFLEX_DATA_SET_STREAM_INDEX_TYPE(Reflex::Data::Table::ColumnInfo, UInt16);

Reflex::Unretained <Reflex::Data::Table> Reflex::Data::Table::Create(ArrayView <ColumnInfo> columns)
{
	if (columns.size && columns.size <= kMaxUInt8)
	{
		return New<MemoryTable>(columns);
	}
	else
	{
		return {};
	}
}

Reflex::Unretained <Reflex::Data::Table> Reflex::Data::Table::Clone(const Table & table)
{
	if (table.object_t == MemoryTable::kDynamicTypeInfo)
	{
		return New<MemoryTable>(Cast<MemoryTable>(table));
	}
	else
	{
		return Null<Table>();
	}
}

Reflex::Unretained <Reflex::Data::Table> Reflex::Data::Table::Deserialize(System::FileHandle & stream)
{
	MemoryTable::FileHeader header;

	if (File::ReadValue(stream, header))
	{
		bool magic = header.magic == MemoryTable::FileHeader::kMagic;
		bool version = header.flags == 0 && header.version == 0;
		bool size = File::GetRemainder(stream) >= header.size;

		if (magic && version && size)
		{
			return New<MemoryTable>(stream, header);
		}
	}

	return {};
}

void Reflex::Data::Table::Publish(ArrayView <ColumnInfo> columns, ArrayView <UInt8> null_row)
{
#if REFLEX_DEBUG
	UInt size = 0;

	for (auto & i : columns) size += Detail::kRawDataTypeSize[Detail::kColumnTypeToRawDataType[i.type]];

	REFLEX_ASSERT(size == null_row.size);
#endif

	RemoveConst(Table::m_columns) = columns;

	m_null_row = null_row;
}

//const Reflex::Data::Table::Index * Reflex::Data::Table::GetIndex(const ColumnInfo & column) const
//{
//	if (column.flags.Check(ColumnInfo::kFlagIndexed))
//	{
//		auto & index = m_indices.Retrieve(column.id);
//
//		if (SetFiltered(m_reindex, false))
//		{
//			switch (kToRawDataType[column.data_type])
//			{
//			case kRawDataTypeValue32:
//				BuildScalarIndex<UInt32>(Cast<Detail::DataSheetAccessor>(*this), column, index);
//				break;
//
//			case kRawDataTypeVariable:
//				BuildVariableIndex(Cast<Detail::DataSheetAccessor>(*this), column, index);
//				break;
//			}
//		}
//
//		return &index;
//	}
//
//	return 0;
//}

const Reflex::Data::Detail::RawDataType Reflex::Data::Detail::kColumnTypeToRawDataType[] =
{
	kRawDataTypeValue8,

	kRawDataTypeValue8,
	kRawDataTypeValue32,
	kRawDataTypeValue64,

	kRawDataTypeValue32,
	kRawDataTypeValue64,

	kRawDataTypeValue32,
	kRawDataTypeValue64,

	kRawDataTypeValue32,			//logically ENUM

	kRawDataTypeValue32,
	kRawDataTypeValue64,

	kRawDataTypeVariable,

	kRawDataTypeVariable,
	kRawDataTypeVariable,

	kRawDataTypeVariable,
	kRawDataTypeVariable,

	kRawDataTypeVariable,
	kRawDataTypeVariable,

	kRawDataTypeVariable,

	kRawDataTypeVariable,
	kRawDataTypeVariable,
	kRawDataTypeVariable,
};

const Reflex::UInt8 Reflex::Data::kColumnTypeToHeapAlignment[] =
{
	1,

	1,
	1,
	1,

	1,
	1,

	1,
	1,

	1,

	1,
	1,

	1,

	4,
	8,

	4,
	8,

	4,
	8,

	4,

	1,
	1,
	2,
};

//struct TableTest
//{
//	static constexpr UInt kWriterBit = 1 << 30;
//
//	struct Reader : public Object
//	{
//		Reader(TableTest *) {}
//	};
//
//	struct Writer : public Object
//	{
//		Writer(TableTest *) {}
//	};
//
//
//
//	AlreadyRetained <Reader> Read()
//	{
//		UInt32 expected = 0;
//
//		if (!m_flags.compare_exchange_strong(expected, kWriterBit))
//		{
//			return New<Reader>(this);
//		}
//
//		return kNoValue; //null
//	}
//
//	AlreadyRetained <Writer> Write()
//	{
//		auto state = m_flags.load();
//
//		while (!(state & kWriterBit))
//		{
//			if (m_flags.compare_exchange_weak(state, kWriterBit))
//			{
//				return New<Writer>(this);
//			}
//		}
//
//		return kNoValue; //null
//	}
//
//
//	AtomicUInt32 m_flags;
//};
