#pragma once

#include "impl.h"




REFLEX_NS(Reflex::Data)

using QueryCompare = bool(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap);


bool EqualBinary(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap);

bool LessThanKey32(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap);

bool GreaterThanOrEqualKey32(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap);

bool GreaterThanKey32(const Table::ConstRowCursor & row, const Table::ColumnInfo & columninfo, const Data::Archive & value, const Data::KeyMap * keymap);


QueryCompare * GetGreaterThanOrEqualFunction(Table::ColumnType type, const Data::KeyMap * keymap);

QueryCompare * GetLessThanFunction(Table::ColumnType type, const Data::KeyMap * keymap);

QueryCompare * GetGreaterThanFunction(Table::ColumnType type, const Data::KeyMap * keymap);


extern QueryCompare * const kEqual[Table::kNumColumnType];

extern QueryCompare * const kInequal[Table::kNumColumnType];

extern QueryCompare * const kLessThan[Table::kNumColumnType];

extern QueryCompare * const kGreaterThanOrEqual[Table::kNumColumnType];

extern QueryCompare * const kGreaterThan[Table::kNumColumnType];

REFLEX_END




//
//impl

inline Reflex::Data::QueryCompare * Reflex::Data::GetLessThanFunction(Table::ColumnType type, const Data::KeyMap * keymap)
{
	if (keymap && type == Table::kColumnTypeKey32)
	{
		return &LessThanKey32;
	}
	else
	{
		return kLessThan[type];
	}
}

inline Reflex::Data::QueryCompare * Reflex::Data::GetGreaterThanOrEqualFunction(Table::ColumnType type, const Data::KeyMap * keymap)
{
	if (keymap && type == Table::kColumnTypeKey32)
	{
		return &GreaterThanOrEqualKey32;
	}
	else
	{
		return kGreaterThanOrEqual[type];
	}
}

inline Reflex::Data::QueryCompare * Reflex::Data::GetGreaterThanFunction(Table::ColumnType type, const Data::KeyMap * keymap)
{
	if (keymap && type == Table::kColumnTypeKey32)
	{
		return &GreaterThanKey32;
	}
	else
	{
		return kGreaterThan[type];
	}
}
