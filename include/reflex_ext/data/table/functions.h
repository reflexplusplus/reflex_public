#pragma once

#include "table.h"




//
//Addon API

namespace Reflex::Data
{

	CString::View GetKey(const KeyMap & keymap, Key32 key);


	void ImportRow(const Table::RowCursor & row, const PropertySet & fields);

	void ExportRow(const Table::ConstRowCursor & row, PropertySet & fields);

	PropertySet ExportRow(const Table::ConstRowCursor & row);


	const Table::ColumnInfo * QueryColumn(ArrayView <Table::ColumnInfo> columns, Key32 id);

	const Table::ColumnInfo * QueryColumn(const Table & table, Key32 id);

	const Table::ColumnInfo & AssumeColumn(ArrayView <Table::ColumnInfo> columns, Key32 id, Table::ColumnType type);	//throw(false)

	const Table::ColumnInfo & AssumeColumn(const Table & table, Key32 id, Table::ColumnType type);	//throw(false)


	WString ToWString(WString16::View string);

	WString16 ToWString16(WString::View string);

}





//
//impl

REFLEX_NS(Reflex::Data::Detail)

using PropertyImportFunction = FunctionPointer<void(KeyMap * keymap, const Table::RowCursor &, const Table::ColumnInfo &, const Object &)>;

using PropertyExportFunction = FunctionPointer<void(const KeyMap * keymap, const Table::ConstRowCursor &, const Table::ColumnInfo &, PropertySet &)>;

using PropertyConverter = Tuple <const TypeID *, PropertyImportFunction, PropertyExportFunction>;


template <class TYPE> Reflex::Detail::RangeHolder < Table::CellReader <TYPE> > Iterate(const Table & table, const Table::ColumnInfo & col);

void ImportRow(ArrayView <PropertyConverter> converters, KeyMap * keymap, const Table::RowCursor & row, ArrayView <Table::ColumnInfo> columns, const PropertySet & fields);	//optimisation -> supply less columns

void ExportRow(ArrayView <PropertyConverter> converters, const KeyMap * keymap, const Table::ConstRowCursor & row, PropertySet & fields);

UInt GetByteSize(Table::ColumnType datatype);

Archive::View GetNullValue(Table::ColumnType datatype);

inline Array <UInt32> PreAllocate(UInt nrows) { Array <UInt32> t; t.Allocate(nrows); return t; }


extern const UInt64 * kNullValues[Table::kNumColumnType];

extern const PropertyConverter kPropertyConverters[Table::kNumColumnType];

REFLEX_END

inline void Reflex::Data::ImportRow(const Table::RowCursor & row, const PropertySet & fields)
{
	Detail::ImportRow(ToView(Detail::kPropertyConverters), nullptr, row, row.table->GetColumns(), fields);
}

inline Reflex::Data::PropertySet Reflex::Data::ExportRow(const Table::ConstRowCursor & row)
{
	PropertySet fields;

	ExportRow(row, fields);

	return fields;
}

inline const Reflex::Data::Table::ColumnInfo * Reflex::Data::QueryColumn(const Table & table, Key32 id)
{
	return QueryColumn(table.GetColumns(), id);
}

inline const Reflex::Data::Table::ColumnInfo & Reflex::Data::AssumeColumn(const Table & table, Key32 id, Table::ColumnType type)
{
	return AssumeColumn(table.GetColumns(), id, type);
}

template <class TYPE> inline Reflex::Detail::RangeHolder < Reflex::Data::Table::CellReader <TYPE> > Reflex::Data::Detail::Iterate(const Table & table, const Table::ColumnInfo & col)
{
	Table::CellReader <TYPE> begin = { table, col, 0 };

	return { std::move(begin), begin + table.GetNumRow() };
}

REFLEX_INLINE Reflex::UInt Reflex::Data::Detail::GetByteSize(Table::ColumnType datatype)
{
	return kRawDataTypeSize[kColumnTypeToRawDataType[datatype]];
}

inline Reflex::Data::Archive::View Reflex::Data::Detail::GetNullValue(Table::ColumnType datatype)
{ 
	return { Reinterpret<UInt8>(kNullValues[datatype]), GetByteSize(datatype) }; 
}
