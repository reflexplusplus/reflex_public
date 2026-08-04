#include "filter.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

constexpr bool LT(int a, int b)
{
	return a < b;
}

constexpr bool GT(int a, int b)
{
	return LT(b, a);
}

constexpr bool GTE(int a, int b)
{
	return !LT(a, b);
}

constexpr bool LTE(int a, int b)
{
	return !LT(b, a);
}

consteval bool run_comparison_tests()
{
	constexpr int kvalues[3] = {-1, 0, 1};

	constexpr UInt size = GetArraySize(kvalues);

	REFLEX_LOOP(i, size)
	{
		REFLEX_LOOP(j, size)
		{
			int a = kvalues[i];
			int b = kvalues[j];

			if (LT(a, b) != (a < b)) return false;
			if (GT(a, b) != (a > b)) return false;
			if (GTE(a, b) != (a >= b)) return false;
			if (LTE(a, b) != (a <= b)) return false;
		}
	}

	return true;
}

REFLEX_STATIC_ASSERT(run_comparison_tests());

template <class TYPE> Data::Archive::View ReadValue(const Table::ConstRowCursor & row, const Table::ColumnInfo & column)
{
	return { row.adr + column.offset, sizeof(TYPE) };
}

const decltype (&ReadHeap) kGet[Detail::kNumRawDataType] =
{
	&ReadHeap,
	&ReadValue<UInt8>,
	0,
	0,
	&ReadValue<UInt32>,
	0,
	0,
	0,
	&ReadValue<UInt64>,
};

struct QuickSorter
{
	typedef Table::ConstRowCursor Row;

	QuickSorter(Table & sheet, const Table::ColumnInfo & column, QueryCompare * lt, QueryCompare * gt, const Data::KeyMap * keymap)
		: sheet(sheet)
		, m_buffer(sheet.GetRowByteSize())
		, column(column)
		, m_get(kGet[Detail::kColumnTypeToRawDataType[column.type]])
		, m_lt(lt)
		, m_gt(gt)
		, m_keymap(keymap)
	{
	}

	void Sort()
	{
		if (UInt n = sheet.GetNumRow()) Sort({ sheet, UInt(0) }, { sheet, n - 1 });
	}

	void Sort(const Row & left, const Row & right)
	{
		auto i = left;

		auto j = right;

		auto mididx = (left.GetIndex() + right.GetIndex()) / 2;

		Data::Archive pivot = m_get({ sheet, mididx }, column);

		while (i <= j)
		{
			while (m_lt(i, column, pivot, m_keymap)) ++i;

			while (m_gt(j, column, pivot, m_keymap)) --j;

			if (i <= j)
			{
				SwapRows(m_buffer, i, j);

				++i;

				--j;
			}
		}

		if (left < j) Sort(left, j);

		if (i < right) Sort(i, right);
	}

	REFLEX_INLINE static void SwapRows(Data::Archive & buffer, const Row & a, const Row & b)
	{
		auto bytes = buffer.GetSize();

		auto t = buffer.GetData();

		auto pa = RemoveConst(a.adr);

		auto pb = RemoveConst(b.adr);

		MemCopy(pa, t, bytes);

		MemCopy(pb, pa, bytes);

		MemCopy(t, pb, bytes);
	}


	Table & sheet;

	Data::Archive m_buffer;

	const Table::ColumnInfo & column;

	decltype (&ReadValue<UInt8>) m_get;

	QueryCompare * m_lt, *m_gt;

	const Data::KeyMap * m_keymap;
};

REFLEX_END_INTERNAL

void Reflex::Data::Detail::Delete(Table & table, ArrayView <UInt> row_indices_ascending)
{
	for (auto & idx : ReverseIterate(row_indices_ascending))
	{
		table.RemoveRow(idx);
	}
}

void Reflex::Data::SortBy(Table & table, Key32 columnid, bool asc, const Data::KeyMap * keymap)
{
	if (auto index_column = QueryColumn(table, columnid))
	{
		QueryCompare * fns[] = { GetLessThanFunction(index_column->type, keymap), GetGreaterThanFunction(index_column->type, keymap) };

		auto lt = fns[!asc];
		auto gt = fns[asc];

		if (lt != &Unavailable)
		{
			QuickSorter sorter(table, *index_column, lt, gt, keymap);

			sorter.Sort();

			Cast<MemoryTable>(table)->Notify();
		}
	}
}
