#include "filter.h"




REFLEX_BEGIN_INTERNAL(Reflex::Data)

REFLEX_INLINE Data::Archive::View ReadHeap(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo)
{
	return row.table->ReadHeapCell(Reinterpret<UInt32>(row.adr + columninfo.offset), kColumnTypeToHeapAlignment[columninfo.type]);
}

bool Unavailable(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	return false;
}

bool InequalBinary(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	Table::CellReader <Data::Archive::View> col = { row, columninfo };

	return *col != value;
}

template <class TYPE> bool EqualValue(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	return row.ReadValue<TYPE>(columninfo) == Data::Unpack<TYPE>(value);
}

template <class TYPE> bool InequalValue(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	return row.ReadValue<TYPE>(columninfo) != Data::Unpack<TYPE>(value);
}

template <class TYPE> bool GreaterThanOrEqualValue(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{ 
	return row.ReadValue<TYPE>(columninfo) >= Data::Unpack<TYPE>(value);
}

template <class TYPE> bool GreaterThanOrEqualBinary(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	auto a = Data::Unpack<ArrayView<TYPE>>(ReadHeap(row, columninfo));
	auto b = Data::Unpack<ArrayView<TYPE>>(value);

	return b < a;
}

template <class TYPE> bool LessThanValue(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	return row.ReadValue<TYPE>(columninfo) < Data::Unpack<TYPE>(value);
}

template <class TYPE> bool LessThanArray(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	auto a = Data::Unpack<ArrayView<TYPE>>(ReadHeap(row, columninfo));
	auto b = Data::Unpack<ArrayView<TYPE>>(value);

	return a < b;
}

template <class TYPE> bool GreaterThanValue(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	return row.ReadValue<TYPE>(columninfo) > Data::Unpack<TYPE>(value);
}

template <class TYPE> bool GreaterThanBinary(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	auto a = Data::Unpack<ArrayView<TYPE>>(ReadHeap(row, columninfo));
	auto b = Data::Unpack<ArrayView<TYPE>>(value);

	if (a < b)
	{
		return false;
	}
	else
	{
		return (a != b);
	}
}

template <auto FUNC> bool Not(const Table::ConstRowCursor & row, const Table::ColumnInfo & column, const Data::Archive & value, const Data::KeyMap * keymap)
{
	return !FUNC(row, column, value, keymap);
}

struct CompiledQuery
{
	typedef Tuple <Table::ColumnInfo, Data::Archive, QueryCompare*> Item;

	enum Optimisation : UInt8
	{
		kOptimisationNone,
		kOptimisationSingle,
		kOptimisationEquals32
	};

	CompiledQuery(const Table & input, ArrayView <QueryData> query, bool match_all, const Data::KeyMap * keymap);

	static bool MatchOne(const Array <Item> & items, const Table::ConstRowCursor & row, const Data::KeyMap * keymap)
	{
		auto q = items.GetFirst();

		return q.c(row, q.a, q.b, keymap);
	}

	static bool MatchAny(const Array <Item> & items, const Table::ConstRowCursor & row, const Data::KeyMap * keymap)
	{
		for (auto & q : items)
		{
			if (q.c(row, q.a, q.b, keymap)) return true;
		}

		return false;
	}

	static bool MatchAll(const Array <Item> & items, const Table::ConstRowCursor & row, const Data::KeyMap * keymap)
	{
		for (auto & q : items)
		{
			if (!q.c(row, q.a, q.b, keymap)) return false;
		}

		return true;
	}


	const ConstAlreadyRetained <Table> table;

	const Data::KeyMap * const keymap;

	Array <Item> items;

	decltype (&MatchAll) match;

	Optimisation optimisation;
};

REFLEX_NOINLINE CompiledQuery::CompiledQuery(const Table & input, ArrayView <QueryData> query, bool match_all, const Data::KeyMap * query_keymap)
	: table(input)
	, keymap(query_keymap)
	, match([](const Array <Item> & items, const Table::ConstRowCursor & row, const Data::KeyMap * keymap)
	{
		return false;
	}),
	optimisation(kOptimisationNone)
{
	if (input.object_t == MemoryTable::kDynamicTypeInfo)
	{
		for (auto & i : query)
		{
			if (auto column = QueryColumn(input, i.a))
			{
				auto type = column->type;

				auto raw_data_type = Detail::kColumnTypeToRawDataType[type];

				if ((raw_data_type == Detail::kRawDataTypeVariable) || (raw_data_type == i.c.GetSize()))
				{
					switch (i.b)
					{
					case kQueryEquals:
						items.Push({ *column, i.c, kEqual[type] });
						continue;

					case kQueryLessThan:
						items.Push({ *column, i.c, GetLessThanFunction(type, keymap) });
						continue;

					case kQueryGreaterThanOrEqual:
						items.Push({ *column, i.c, GetGreaterThanOrEqualFunction(type, keymap) });
						continue;

					case kQueryGreaterThan:
						items.Push({ *column, i.c, GetGreaterThanFunction(type, keymap) });
						continue;

					case kQueryInequal:
						items.Push({ *column, i.c, kInequal[type] });
						continue;

					default:
						return;
					}
				}
				else
				{
					return;
				}
			}
		}

		if (items.GetSize() == 1)
		{
			match = &MatchOne;

			optimisation = (items.GetFirst().c == &EqualValue<UInt32>) ? kOptimisationEquals32 : kOptimisationSingle;
		}
		else
		{
			match = match_all ? &MatchAll : &MatchAny;
		}
	}
}

REFLEX_END_INTERNAL

Reflex::Data::QueryCompare * const Reflex::Data::kEqual[Table::kNumColumnType] =
{
	&EqualValue<UInt8>,

	&EqualValue<UInt8>,
	&EqualValue<UInt32>,
	&EqualValue<UInt64>,

	&EqualValue<UInt32>,
	&EqualValue<UInt64>,

	&EqualValue<UInt32>,	//Float32
	&EqualValue<UInt64>,	//Float64

	&EqualValue<UInt32>,	//Key32

	&EqualValue<UInt32>,	//Date32
	&EqualValue<UInt64>,	//Date64

	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
	&EqualBinary,
};

Reflex::Data::QueryCompare * const Reflex::Data::kLessThan[Table::kNumColumnType] =
{
	&Unavailable,

	&LessThanValue<UInt8>,
	&LessThanValue<UInt32>,
	&LessThanValue<UInt64>,

	&LessThanValue<Int32>,	//Float32
	&LessThanValue<Int64>,	//Float64

	&LessThanValue<Float32>,	//Float32
	&LessThanValue<Float64>,	//Float64

	&LessThanValue<UInt32>,

	&LessThanValue<UInt32>,	//Date32
	&LessThanValue<UInt64>,	//Date32

	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&LessThanArray<char>,
	&LessThanArray<char>,
	&Unavailable,
};

Reflex::Data::QueryCompare * const Reflex::Data::kGreaterThanOrEqual[Table::kNumColumnType] =
{
	&Unavailable,

	&Not<LessThanValue<UInt8>>,
	&Not<LessThanValue<UInt32>>,
	&Not<LessThanValue<UInt64>>,

	&Not<LessThanValue<Int32>>,	//Float32
	&Not<LessThanValue<Int64>>,	//Float64

	&Not<LessThanValue<Float32>>,	//Float32
	&Not<LessThanValue<Float64>>,	//Float64

	&Not<LessThanValue<UInt32>>,

	&Not<LessThanValue<UInt32>>,	//Date32
	&Not<LessThanValue<UInt64>>,	//Date64

	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Not<LessThanArray<char>>,
	&Not<LessThanArray<char>>,
	&Unavailable,
};

Reflex::Data::QueryCompare * const Reflex::Data::kGreaterThan[Table::kNumColumnType] =
{
	&Unavailable,

	&GreaterThanValue<UInt8>,
	&GreaterThanValue<UInt32>,
	&GreaterThanValue<UInt64>,

	&GreaterThanValue<Int32>,	//Float32
	&GreaterThanValue<Int64>,	//Float64

	&GreaterThanValue<Float32>,	//Float32
	&GreaterThanValue<Float64>,	//Float64

	&GreaterThanValue<UInt32>,

	&GreaterThanValue<UInt32>,	//Date32
	&GreaterThanValue<UInt64>,	//Date32

	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&Unavailable,
	&GreaterThanBinary<char>,
	&GreaterThanBinary<char>,
	&Unavailable,
};

Reflex::Data::QueryCompare * const Reflex::Data::kInequal[Table::kNumColumnType] =
{
	&InequalValue<UInt8>,

	&InequalValue<UInt8>,
	&InequalValue<UInt32>,
	&InequalValue<UInt64>,

	&InequalValue<UInt32>,
	&InequalValue<UInt64>,

	&InequalValue<UInt32>,	//Float32
	&InequalValue<UInt64>,	//Float64

	&InequalValue<UInt32>,	//Key32

	&InequalValue<UInt32>,	//Date32
	&InequalValue<UInt64>,	//Date64

	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
	&InequalBinary,
};

bool Reflex::Data::EqualBinary(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap *)
{
	Table::CellReader <Data::Archive::View> col = { row, columninfo };

	return *col == value;
}

bool Reflex::Data::LessThanKey32(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap)
{
	REFLEX_ASSERT(keymap);

	auto a = GetKey(*keymap, row.ReadValue<Key32>(columninfo));
	auto b = GetKey(*keymap, Data::Unpack<Key32>(value));

	return CaseInsensitive::lt(a, b);
}

bool Reflex::Data::GreaterThanOrEqualKey32(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap)
{
	return !LessThanKey32(row, columninfo, value, keymap);
}

bool Reflex::Data::GreaterThanKey32(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap)
{
	REFLEX_ASSERT(keymap);

	auto a = GetKey(*keymap, row.ReadValue<Key32>(columninfo));
	auto b = GetKey(*keymap, Data::Unpack<Key32>(value));

	return CaseInsensitive::lt(b, a);
}

Reflex::Data::Table::ConstRowCursor Reflex::Data::FindFirst(const Table & table, ArrayView <QueryData> ops, bool all, const Data::KeyMap * keymap)
{
	CompiledQuery query(table, ops, all, keymap);

	if (auto & items = query.items)
	{
		auto & first = items.GetFirst();

		switch (query.optimisation)
		{
		case CompiledQuery::kOptimisationNone:
			for (auto & row : query.table) if (query.match(items, row, query.keymap)) return row;
			break;

		case CompiledQuery::kOptimisationSingle:
			for (auto & row : query.table) if (first.c(row, first.a, first.b, query.keymap)) return row;
			break;

		case CompiledQuery::kOptimisationEquals32:
			for (auto & row : query.table) if (EqualValue<UInt32>(row, first.a, first.b, query.keymap)) return row;
			break;
		}
	}

	return {};
}

Reflex::Array <Reflex::UInt> Reflex::Data::SelectRows(const Table & table, ArrayView <QueryData> ops, bool all, const Data::KeyMap * keymap)
{
	CompiledQuery query(table, ops, all, keymap);

	if (auto & items = query.items)
	{
		auto & first = items.GetFirst();

		UInt row_idx = 0;

		auto rtn = Detail::PreAllocate(table.GetNumRow());

		switch (query.optimisation)
		{
		case CompiledQuery::kOptimisationNone:
			for (auto & row : query.table)
			{
				if (query.match(items, row, query.keymap)) rtn.Push<kAllocateNone>(row_idx);

				++row_idx;
			}
			break;

		case CompiledQuery::kOptimisationSingle:
			for (auto & row : query.table)
			{
				if (first.c(row, first.a, first.b, query.keymap)) rtn.Push<kAllocateNone>(row_idx);

				++row_idx;
			}
			break;

		case CompiledQuery::kOptimisationEquals32:
			for (auto & row : query.table)
			{
				if (EqualValue<UInt32>(row, first.a, first.b, query.keymap)) rtn.Push<kAllocateNone>(row_idx);

				++row_idx;
			}
			break;
		}
	
		return rtn;
	}

	return {};
}

void Reflex::Data::Detail::Select(const Table & table, ArrayView <QueryData> ops, bool match_all, void * client, FunctionPointer <void(void*, const Table::ConstRowCursor&)> callback, const Data::KeyMap * keymap)
{
	auto rows = SelectRows(table, ops, match_all, keymap);

	for (auto & idx : rows)
	{
		callback(client, table[idx]);
	}
}
