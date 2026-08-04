#include "[include].h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

REFLEX_NOINLINE Data::Archive::View GetMostFrequentBinary(const Table & src, Table::CellPtrImpl <true> first, ArrayView <UInt> rows, UInt8 alignment)
{
	Map <Data::Archive::View,UInt> counts;

	Data::Archive::View best = {};

	UInt best_count = 0;

	for (auto & row : rows)
	{
		auto cell = first + row;

		auto value = src.ReadHeapCell(Reinterpret<UInt32>(cell.adr), alignment);

		auto & count = counts.Acquire(value);

		++count;

		if (count > best_count)
		{
			best = value;
			best_count = count;
		}
	}

	return best;
}

template <class TYPE> REFLEX_NOINLINE TYPE GetMostFrequentValue(Table::CellPtrImpl <true> first, ArrayView <UInt> rows)
{
	Map <TYPE,UInt> counts;

	TYPE best = {};

	UInt best_count = 0;

	for (auto & row : rows)
	{
		auto cell = first + row;

		auto value = cell.ReadValue<TYPE>();

		auto & count = counts.Acquire(value);

		++count;

		if (count > best_count)
		{
			best = value;
			best_count = count;
		}
	}

	return best;
}

void MostFrequentBinary(const Table & src, Table::CellPtrImpl <true> cell, ArrayView <UInt> rows, Table & dest, Table::CellPtrImpl <false> destcell, UInt8 alignment)
{
	if (auto best = GetMostFrequentBinary(src, cell, rows, alignment))
	{
		dest.SetHeapCell(Reinterpret<UInt32>(destcell.adr), alignment, best);
	}
}

template <class TYPE> void MostFrequentValue(const Table & src, Table::CellPtrImpl <true> first, ArrayView <UInt> rows, Table & dest, Table::CellPtrImpl <false> destcell, UInt8)
{
	Reinterpret<Detail::ValueBytes<TYPE>>(destcell.adr)->Write(GetMostFrequentValue<TYPE>(first, rows));
}

void MinMaxBinary(const Table & src, Table::CellPtrImpl <true> cell, ArrayView <UInt> rows, Table & dest, Table::CellPtrImpl <false> destcell, UInt8 alignment)
{
}

template <bool MAX, class TYPE> REFLEX_NOINLINE void MinMaxValue(const Table & src, Table::CellPtrImpl <true> first, ArrayView <UInt> rows, Table & dest, Table::CellPtrImpl <false> destcell, UInt8)
{
	TYPE value = {};

	if (rows)
	{
		value = (first + rows.GetFirst()).ReadValue<TYPE>();

		for (auto & row : rows)
		{
			auto cell = first + row;

			if constexpr (MAX)
			{
				value = Max(value, cell.ReadValue<TYPE>());
			}
			else
			{
				value = Min(value, cell.ReadValue<TYPE>());
			}
		}
	}

	Reinterpret<Detail::ValueBytes<TYPE>>(destcell.adr)->Write(value);
}

const decltype (&MostFrequentBinary) kAggregateMostFrequentFns[Table::kNumColumnType] =
{
	&MostFrequentValue<UInt8>,

	&MostFrequentValue<UInt8>,
	&MostFrequentValue<UInt32>,
	&MostFrequentValue<UInt64>,

	&MostFrequentValue<UInt32>,
	&MostFrequentValue<UInt64>,

	&MostFrequentValue<UInt32>,
	&MostFrequentValue<UInt64>,

	&MostFrequentValue<UInt32>,

	&MostFrequentValue<UInt32>,
	&MostFrequentValue<UInt64>,

	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
	&MostFrequentBinary,
};

const decltype (&MinMaxValue<false,UInt8>) kAggregateMinFns[Table::kNumColumnType] =
{
	&MinMaxValue<false,UInt8>,

	&MinMaxValue<false,UInt8>,
	&MinMaxValue<false,UInt32>,
	&MinMaxValue<false,UInt64>,

	&MinMaxValue<false,Int32>,
	&MinMaxValue<false,Int64>,
	
	&MinMaxValue<false,Float32>,
	&MinMaxValue<false,Float64>,
	
	&MinMaxValue<false,UInt32>,
	
	&MinMaxValue<false,UInt32>,
	&MinMaxValue<false,UInt64>,

	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
};

const decltype (&MinMaxValue<false, UInt8>) kAggregateMaxFns[Table::kNumColumnType] =
{
	&MinMaxValue<true,UInt8>,

	&MinMaxValue<true,UInt8>,
	&MinMaxValue<true,UInt32>,
	&MinMaxValue<true,UInt64>,

	&MinMaxValue<true,Int32>,
	&MinMaxValue<true,Int64>,

	&MinMaxValue<true,Float32>,
	&MinMaxValue<true,Float64>,

	&MinMaxValue<true,UInt32>,

	&MinMaxValue<true,UInt32>,
	&MinMaxValue<true,UInt64>,

	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
	&MinMaxBinary,
};

REFLEX_NOINLINE TRef <Reflex::Data::Table> SummariseImpl(ConstTRef <Table> input, const Table::ColumnInfo * pindexcol, const Map < Data::Archive::View, Array <UInt> > & collapsed, const ArrayView <Pair<Key32,AggregateOp>> & ops)
{
	UInt size = 0;

	auto inputcols = input->GetColumns();


	//prepare output

	Array <Table::ColumnInfo> toutputcols;

	toutputcols.Allocate(ops.size + 1);

	for (auto & op : ops)
	{
		if (auto pcol = QueryColumn(inputcols, op.a))
		{
			toutputcols.Push<kAllocateNone>(*pcol);
		}
	}

	toutputcols.Push<kAllocateNone>({ kcount, Table::kColumnTypeUInt32 });

	if (pindexcol)
	{
		size = Detail::kColumnTypeToRawDataType[pindexcol->type];

		Remove<KeyCompare>(Reinterpret<Array<ColumnInfoAsTuple>>(toutputcols), pindexcol->id);

		toutputcols.Insert(0, *pindexcol);
	}

	auto output = Table::Create(toutputcols);

	output->Extend(collapsed.GetSize());

	auto outputcols = output->GetColumns();

	auto count_offset = outputcols.GetLast().offset;
	auto poutputindexcol = pindexcol ? &outputcols.GetFirst() : nullptr;


	//prepare copy

	Array < Tuple < Table::ColumnInfo, Table::ColumnInfo, decltype (&MostFrequentBinary) > > cquery;

	for (auto & i : ops)
	{
		auto inputcol = QueryColumn(input, i.a);

		if (auto outputcol = QueryColumn(outputcols, i.a))
		{
			auto datatype = inputcol->type;

			switch (i.b)
			{
			case kAggregateMostFrequent:
				cquery.Push({ *inputcol, *outputcol, kAggregateMostFrequentFns[datatype] });
				continue;

			case kAggregateMax:
				cquery.Push({ *inputcol, *outputcol, kAggregateMaxFns[datatype] });
				continue;

			case kAggregateMin:
				cquery.Push({ *inputcol, *outputcol, kAggregateMinFns[datatype] });
				continue;

			default:
				Reference<Table> retain(output);
				return REFLEX_NULL(Table);
			}
		}
	}

	auto destrow = output->begin();

	auto srcrow = input->begin();

	for (auto & [key,value] : collapsed)
	{
		if (size)
		{
			MemCopy(key.data, destrow.adr, size);
		}
		else if (poutputindexcol)
		{
			output->SetHeapCell(Reinterpret<UInt32>(destrow.adr + poutputindexcol->offset), kColumnTypeToHeapAlignment[poutputindexcol->type], key);
		}

		for (auto & i : cquery)
		{
			i.c(*input, { srcrow, i.a }, value, *output, { destrow, i.b }, kColumnTypeToHeapAlignment[i.a.type]);
		}

		*Reinterpret<UInt32>(destrow.adr + count_offset) = value.GetSize();

		++destrow;
	}

	return output;
}

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Data::Table> Reflex::Data::Detail::Aggregate(const Table & input, ArrayView <Pair<Key32,AggregateOp>> ops)
{
	Map < Data::Archive::View, Array <UInt> > collapsed;

	collapsed[{}] = MakeAllRows(input);

	return SummariseImpl(input, 0, collapsed, ops);
}

Reflex::TRef <Reflex::Data::Table> Reflex::Data::Detail::GroupBy(const Table & input, Key32 columnid, ArrayView <Pair<Key32,AggregateOp>> ops)
{
	auto inputcols = input.GetColumns();

	if (auto pindexcol = QueryColumn(inputcols, columnid))
	{
		Map < Data::Archive::View, Array <UInt> > collapsed;

		auto srcrowsize = input.GetRowByteSize();

		UInt row = 0;

		if (auto size = kColumnTypeToRawDataType[pindexcol->type])	//TODO support binary
		{
			Data::Archive::View value = { ToPointer<UInt8>(0), size };

			for (auto ptr = input.GetRawData() + pindexcol->offset, end = ptr + (srcrowsize * input.GetNumRow()); ptr < end; ptr += srcrowsize)
			{
				value.data = ptr;

				collapsed[value].Push(row++);
			}
		}
		else
		{
			for (auto ptr = input.GetRawData() + pindexcol->offset, end = ptr + (srcrowsize * input.GetNumRow()); ptr < end; ptr += srcrowsize)
			{
				auto value = input.ReadHeapCell(Reinterpret<UInt32>(ptr), kColumnTypeToHeapAlignment[pindexcol->type]);

				collapsed[value].Push(row++);
			}

		}

		return SummariseImpl(input, pindexcol, collapsed, ops);
	}

	return REFLEX_NULL(Table);
}
