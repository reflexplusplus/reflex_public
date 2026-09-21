#pragma once

#include "[include].h"




//
//impl

REFLEX_NS(Reflex::Data)

struct MemoryTable : public Table
{
	REFLEX_OBJECT(MemoryTable, Table);	//need this to check in Clone

	struct FileHeader
	{
		static constexpr UInt32 kMagic = K32("Table");

		Key32 magic;
		UInt32 size;

		UInt8 ncolumn;
		UInt8 flags;
		UInt16 version;

		UInt32 datasize;
		UInt32 heapsize;
		UInt32 freeheapsize;
	};

	
	MemoryTable(ArrayView <ColumnInfo> columns);

	MemoryTable(System::FileHandle & stream, FileHeader & header);

	MemoryTable(const MemoryTable & table);

	void Allocate(UInt num_row) override;	//pre allocate rows, use before multiple AddRow
		

	void Clear() override;	//size = 0

	void Extend(UInt num_extra_row) override;

	void Shrink(UInt num_less_row) override;

	void Compact() override;


	RowCursor AddRow() override;

	void RemoveRow(UInt idx) override;

	
	UInt CalculateStorageSize() const override;

	void Serialize(System::FileHandle & stream) const override;


	void ClearRow(UInt8 * rowptr) override;
		
	Data::Archive::View ReadHeapCell(const UInt32 * cell, UInt8 alignment) const override;

	void ClearHeapCell(UInt32 * cell, UInt8 alignment) override;

	void SetHeapCell(UInt32 * cell, UInt8 alignment, Data::Archive::View binary) override;

	UInt AllocateHeapData(UInt16 size, UInt8 alignment);

	
	using Table::SetData;	//for static ctor

	using State::Notify;


	Array <ColumnInfo> m_columninfo;

	Array <const ColumnInfo*> m_heap_columns;

	Data::Archive m_nullrow;

	Data::Archive m_data;

	Data::Archive m_heap;

	Sequence <UInt32,UInt> m_freeheap;	//encoded payload bytes + alignment, adr

	const CString m_null_string;

	bool m_has_key32_columns;
};

REFLEX_END
