#pragma once

#include "../../../../include/reflex_ext/data/table.h"




REFLEX_NS(Reflex::Data)

typedef Tuple <Key32, Table::ColumnType, UInt8, UInt16> ColumnInfoAsTuple;

struct HeapBlockHeader
{
	UInt16 size;
	UInt8 alignment;
	UInt8 payload_offset;
};

constexpr UInt32 kNullHeapAdr = kSizeOf<HeapBlockHeader>;

extern const UInt8 kColumnTypeToHeapAlignment[Table::kNumColumnType];

Array < Pair <Table::ColumnInfo> > MatchColumns(const ArrayView <Table::ColumnInfo> & a, const ArrayView <Table::ColumnInfo> & b)
{
	Array < Pair <Table::ColumnInfo> > columns;

	for (auto & x : a)
	{
		for (auto & y : b)
		{
			if (x.id == y.id && x.type == y.type)
			{
				columns.Push({ x, y });

				break;
			}
		}
	}

	return columns;
}

void CopyCells(const Array < Pair <Table::ColumnInfo> > & columns, const Table::ConstRowCursor & from, const Table::RowCursor & to)
{
	for (auto & [in, out] : columns)
	{
		if (auto raw_data_type = Detail::kColumnTypeToRawDataType[in.type])
		{
			MemCopy(from.adr + in.offset, to.adr + out.offset, raw_data_type);
		}
		else
		{
			auto src_cell = Reinterpret<const UInt32>(from.adr + in.offset);

			if (*src_cell != kNullHeapAdr)
			{
				auto alignment = kColumnTypeToHeapAlignment[in.type];
				auto dst_cell = Reinterpret<UInt32>(to.adr + out.offset);

				to.table->SetHeapCell(dst_cell, alignment, from.table->ReadHeapCell(src_cell, alignment));
			}
		}
	}
}

REFLEX_END

REFLEX_STATIC_ASSERT(sizeof(Reflex::Data::ColumnInfoAsTuple) == sizeof(Reflex::Data::Table::ColumnInfo));
