#include "impl.h"





REFLEX_BEGIN_INTERNAL(Reflex::Data)

struct NullTable : public Table
{
	NullTable()
	{
		columns[0] = { {}, Table::kColumnTypeUInt64 };	//allocate widest type for null

		Publish(columns, null_row);

		SetData(null_row, 1);
	}


	void Allocate(UInt num_row) override {}


	void Clear() override {}

	void Extend(UInt) override {}

	void Shrink(UInt) override {}

	void Compact() override {}


	RowCursor AddRow() override 
	{
		return { this, null_row };
	}

	void RemoveRow(UInt idx) override {}


	UInt CalculateStorageSize() const override
	{
		return 0;
	}

	void Serialize(System::FileHandle & stream) const override
	{
	}


	void ClearRow(UInt8 * rowptr) override {}

	Data::Archive::View ReadHeapCell(const UInt32 * cell, UInt8 alignment) const override { return {}; }

	void ClearHeapCell(UInt32 * cell, UInt8 alignment) override {}

	void SetHeapCell(UInt32 * cell, UInt8 alignment, Data::Archive::View binary) override {}

	
	ColumnInfo columns[1];

	UInt8 null_row[Detail::kRawDataTypeValue64];
};

NullTable g_null_table;

REFLEX_END_INTERNAL

Reflex::Data::Table & Reflex::Data::Table::null = g_null_table;
