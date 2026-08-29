#pragma once

#include "table.h"




//
//Addon API

namespace Reflex::Data
{

	REFLEX_DECLARE_KEY32(count);	//output column written by Aggregate/GroupBy/CountBy

	REFLEX_DECLARE_KEY32(ratio);	//output column written by CountBy(ratio=true)


	enum QueryOp : UInt32
	{
		kQueryEquals = K32("="),
		kQueryLessThan = K32("<"),
		kQueryGreaterThanOrEqual = K32(">="),
		kQueryGreaterThan = K32(">"),
		kQueryInequal = K32("!="),
	};

	enum AggregateOp : UInt32
	{
		kAggregateMostFrequent = K32("most"),
		kAggregateMax = K32("max"),
		kAggregateMin = K32("min"),
	};

	typedef Tuple <Key32, QueryOp, Archive> QueryData;	//column | op | value packed as binary, use MakeQuery and Equals


	Reference <Table> Select(const Table & table, ArrayView <QueryData> ops, ArrayView <Key32> columns = {}, bool match_all = true, const KeyMap * keymap = nullptr);

	template <class CLIENT> void Select(const Table & table, ArrayView <QueryData> ops, bool match_all, CLIENT & client, FunctionPointer <void(CLIENT&, const Table::ConstRowCursor&)> callback, const KeyMap * keymap = nullptr);


	Reference <Table> Slice(const Table & table, ArrayView <Key32> columns, ArrayView <UInt> row_indices);

	Reference <Table> Aggregate(const Table & table, ArrayView < Pair <Key32, AggregateOp> > ops);

	Reference <Table> GroupBy(const Table & table, Key32 column, ArrayView < Pair <Key32, AggregateOp> > ops);

	Reference <Table> CountBy(const Table & table, Key32 column, bool ratio = false);


	void Delete(Table & table, ArrayView <QueryData> ops, bool match_all = true, const KeyMap * keymap = nullptr);

	void SortBy(Table & table, Key32 column, bool ascending, const KeyMap * keymap = nullptr);


	template <QueryOp OP, class TYPE> QueryData MakeQuery(Key32 id, const TYPE & value);

	template <class TYPE> QueryData Equals(Key32 id, const TYPE & value);


	Table::ConstRowCursor FindFirst(const Table & table, ArrayView <QueryData> ops, bool match_all = true, const KeyMap * keymap = nullptr);

	Array <UInt> MakeAllRows(const Table & table);

	Array <UInt> SelectRows(const Table & table, ArrayView <QueryData> ops, bool match_all = true, const KeyMap * keymap = nullptr);

}





//
//impl

REFLEX_NS(Reflex::Data::Detail)

[[nodiscard]] TRef <Table> Select(const Table & table, ArrayView <QueryData> ops, ArrayView<Key32> columns, bool match_all, const KeyMap * keymap);

void Select(const Table & table, ArrayView <QueryData> ops, bool match_all, void * client, FunctionPointer <void(void*, const Table::ConstRowCursor&)> callback, const KeyMap * keymap);

[[nodiscard]] TRef <Table> Slice(const Table & table, ArrayView <Key32> columns, ArrayView <UInt> row_indices);

[[nodiscard]] TRef <Table> Aggregate(const Table & table, ArrayView < Pair <Key32,AggregateOp> > ops);

[[nodiscard]] TRef <Table> GroupBy(const Table & table, Key32 column, ArrayView < Pair <Key32,AggregateOp> > ops);

[[nodiscard]] TRef <Table> CountBy(const Table & table, Key32 column, bool ratio = false);

void Delete(Table & table, ArrayView <UInt> row_indices_ascending);

template <class TYPE> inline Archive::View PackRaw(const TYPE & view)
{
	REFLEX_STATIC_ASSERT(Detail::IsRawPackable<TYPE>::value);

	if constexpr (IsType<TYPE,Archive,Archive::View>::value)
	{
		return view;
	}
	else
	{
		return Pack(view);
	}
}

REFLEX_END

inline Reflex::TRef <Reflex::Data::Table> Reflex::Data::Detail::Select(const Table & table, ArrayView <QueryData> ops, ArrayView <Key32> columns, bool match_all, const KeyMap * keymap)
{
	return Detail::Slice(table, columns, SelectRows(table, ops, match_all, keymap));
}

inline Reflex::Reference <Reflex::Data::Table> Reflex::Data::Select(const Table & table, ArrayView <QueryData> ops, ArrayView<Key32> columns, bool match_all, const KeyMap * keymap)
{
	return Detail::Select(table, ops, columns, match_all, keymap);
}

inline Reflex::Reference <Reflex::Data::Table> Reflex::Data::Slice(const Table & table, ArrayView <Key32> columns, ArrayView <UInt> row_indices)
{
	return Detail::Slice(table, columns, row_indices);
}

inline Reflex::Reference <Reflex::Data::Table> Reflex::Data::Aggregate(const Table & table, ArrayView < Pair <Key32,AggregateOp> > ops)
{
	return Detail::Aggregate(table, ops);
}

inline Reflex::Reference <Reflex::Data::Table> Reflex::Data::GroupBy(const Table & table, Key32 column, ArrayView < Pair <Key32,AggregateOp> > ops)
{
	return Detail::GroupBy(table, column, ops);
}

inline Reflex::Reference <Reflex::Data::Table> Reflex::Data::CountBy(const Table & table, Key32 column, bool ratio)
{
	return Detail::CountBy(table, column, ratio);
}

template <class CLIENT> inline void Reflex::Data::Select(const Table & table, ArrayView <QueryData> ops, bool match_all, CLIENT & client, FunctionPointer <void(CLIENT&, const Table::ConstRowCursor&)> callback, const KeyMap * keymap)
{
	typedef FunctionPointer <void(void*, const Table::ConstRowCursor&)> FunctionPointer;

	Detail::Select(table, ops, match_all, &client, reinterpret_cast<FunctionPointer>(callback), keymap);
}

inline void Reflex::Data::Delete(Table & table, ArrayView <QueryData> ops, bool match_all, const KeyMap * keymap)
{
	Detail::Delete(table, SelectRows(table, ops, match_all, keymap));
}

template <Reflex::Data::QueryOp OP, class TYPE> inline Reflex::Data::QueryData Reflex::Data::MakeQuery(Key32 id, const TYPE & value)
{ 
	return { id, OP, Detail::PackRaw(value) }; 
}

template <class TYPE> inline Reflex::Data::QueryData Reflex::Data::Equals(Key32 id, const TYPE & value)
{
	return MakeQuery<kQueryEquals>(id, value); 
}
