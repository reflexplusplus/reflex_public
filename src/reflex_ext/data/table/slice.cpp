#include "[include].h"




//
//

REFLEX_BEGIN_INTERNAL(Reflex::Data)

TRef <Table> SliceColumns(ConstTRef <Table> input, ArrayView <Key32> columns)
{
	auto inputcols = input->GetColumns();

	Array <Table::ColumnInfo> outputcols;

	for (auto id : columns)
	{
		if (auto pcolumn = QueryColumn(inputcols, id))
		{
			outputcols.Push(*pcolumn);
		}
	}

	if (!outputcols) return REFLEX_NULL(Table);

	return Table::Create(outputcols);
}

REFLEX_END_INTERNAL

Reflex::Array <Reflex::UInt> Reflex::Data::MakeAllRows(const Table & table)
{
	Array <UInt> rows(table.GetNumRow());

	UInt idx = 0;

	for (auto & i : rows) i = idx++;

	return rows;
}

Reflex::TRef <Reflex::Data::Table> Reflex::Data::Detail::Slice(const Table & input, ArrayView <Key32> columns, ArrayView <UInt32> rows)
{
	auto inputcols = input.GetColumns();

	TRef <Table> output;

	if (columns)
	{
		output = SliceColumns(input, columns);
	}
	else
	{
		output = Table::Create(inputcols);
	}

	output->Extend(rows.size);

	auto cols = MatchColumns(inputcols, output->GetColumns());

	Table::ConstRowCursor from = { input, 0 };

	Table::RowCursor to = { *output, 0 };

	for (auto row_idx : rows)
	{
		from.SetIndex(row_idx);

		CopyCells(cols, from, to);

		++to;
	}

	return output;
}
