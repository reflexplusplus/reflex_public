#include "[include].h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

void CountBinaries(const Table & sheet, Table::CellPtrImpl <true> && itr, Table::CellPtrImpl <true> && end, Table & output, UInt8 alignment)
{
	Sequence <Data::Archive::View,UInt> counts;

	for (; itr != end; ++itr)
	{
		auto binary = UnpackRawArray<UInt8>(sheet.ReadHeapCell(Reinterpret<UInt32>(itr.adr), alignment));

		counts.Acquire(binary)++;
	}

	if (counts)
	{
		output.Allocate(counts.GetSize());

		auto itr = counts.begin();
		
		auto end = counts.end();

		if (!(*itr).key)
		{
			Reinterpret<Pair<UInt32>>(output.AddRow().adr)->b = (*itr).value;

			++itr;
		}

		while (itr != end)
		{
			auto prow = output.AddRow().adr;

			auto & i = *itr;

			++itr;

			output.SetHeapCell(Reinterpret<UInt32>(prow), alignment, i.key);

			Reinterpret<Pair<UInt32>>(prow)->b = i.value;
		}
	}
}

template <class TYPE> void CountValues(const Table & sheet, Table::CellPtrImpl <true> && itr, Table::CellPtrImpl <true> && end, Table & output, UInt8)
{
	REFLEX_ASSERT(output.GetRowByteSize() == sizeof(TYPE) + sizeof(UInt));

	Map <TYPE,UInt> counts;

	for (; itr != end; ++itr)
	{
		counts[itr.ReadValue<TYPE>()]++;
	}

	output.Allocate(counts.GetSize());

	if constexpr (sizeof(Tuple<TYPE,UInt>) == sizeof(TYPE) + sizeof(UInt))
	{
		for (auto & i : counts)
		{
			*Reinterpret<Tuple<TYPE,UInt>>(output.AddRow().adr) = { i.key, i.value };
		}
	}
	else
	{
		for (auto & i : counts)
		{
			auto row = output.AddRow().adr;

			*Reinterpret<TYPE>(row) = i.key;
			*Reinterpret<UInt>(row + sizeof(TYPE)) = i.value;
		}
	}
}

const decltype (&CountBinaries) kCountFunctions[Table::kNumColumnType] =
{
	&CountValues<UInt8>,

	&CountValues<UInt8>,
	&CountValues<UInt32>,
	&CountValues<UInt64>,

	&CountValues<UInt32>,
	&CountValues<UInt64>,
	
	&CountValues<UInt32>,
	&CountValues<UInt64>,
	
	&CountValues<UInt32>,
	
	&CountValues<UInt32>,
	&CountValues<UInt64>,

	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
	&CountBinaries,
};

REFLEX_END_INTERNAL

Reflex::TRef <Reflex::Data::Table> Reflex::Data::Detail::CountBy(const Table & input, Key32 columnid, bool ratio)
{
	if (auto indexcolumn = QueryColumn(input, columnid))
	{
		constexpr Key32 kNames[] = { kcount, kratio };

		auto output = Table::Create({ *indexcolumn, { kNames[ratio], Table::kColumnTypeUInt32 } });

		kCountFunctions[indexcolumn->type](input, { input, *indexcolumn, 0 }, { input, *indexcolumn, input.GetNumRow() }, *output, kColumnTypeToHeapAlignment[indexcolumn->type]);

		if (ratio)
		{
			auto & countcol = output->GetColumns()[1];

			RemoveConst(countcol).type = Table::kColumnTypeFloat32;

			if (auto n = input.GetNumRow())
			{
				Table::CellPtrImpl <false> itr = { *output, countcol, 0 };

				auto end = itr + output->GetNumRow();

				auto nrowf = Float64(n);

				for (; itr != end; ++itr)
				{
					auto value = itr.ReadValue<UInt32>();

					Reinterpret<Detail::ValueBytes<Float32>>(itr.adr)->Write(Float32(Float64(value) / nrowf));
				}
			}
		}

		return output;
	}
	else
	{
		return REFLEX_NULL(Table);
	}
}
