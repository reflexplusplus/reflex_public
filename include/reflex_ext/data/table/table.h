#pragma once

#include "defines.h"




//
//Addon API

namespace Reflex::Data
{

	class Table;

}




//
//Table

class Reflex::Data::Table :
	public Object,
	public State
{
public:

	REFLEX_OBJECT(Reflex::Data::Table, Object);

	static Table & null;


	enum ColumnType : UInt8
	{
		kColumnTypeBool,

		kColumnTypeUInt8,
		kColumnTypeUInt32,
		kColumnTypeUInt64,

		kColumnTypeInt32,
		kColumnTypeInt64,

		kColumnTypeFloat32,
		kColumnTypeFloat64,

		kColumnTypeKey32,

		kColumnTypeDate32,
		kColumnTypeDate64,

		kColumnTypeArrayOfUInt8,

		kColumnTypeArrayOfUInt32,
		kColumnTypeArrayOfUInt64,

		kColumnTypeArrayOfInt32,
		kColumnTypeArrayOfInt64,

		kColumnTypeArrayOfFloat32,
		kColumnTypeArrayOfFloat64,

		kColumnTypeArrayOfKey32,

		kColumnTypeStringASCII,
		kColumnTypeStringUTF8,
		kColumnTypeStringUCS2,

		kNumColumnType,

		kColumnTypeBinary = kColumnTypeArrayOfUInt8,	//will be removed
	};

	struct ColumnInfo
	{
		Key32 id;
		ColumnType type;
		UInt8 flags = 0;
		UInt16 offset = kMaxUInt16;
	};


	template <bool CONST> struct RowCursorImpl;

	using ConstRowCursor = RowCursorImpl <true>;

	using RowCursor = RowCursorImpl <false>;



	//lifetime

	[[nodiscard]] static TRef <Table> Create(ArrayView <ColumnInfo> columns);

	[[nodiscard]] static TRef <Table> Clone(const Table & table);

	[[nodiscard]] static TRef <Table> Deserialize(System::FileHandle & stream);


	virtual void Allocate(UInt num_row) = 0;
	
	
	virtual void Clear() = 0;	//clears all rows

	virtual void Extend(UInt num_extra_row) = 0;

	virtual void Shrink(UInt num_less_row) = 0;

	virtual void Compact() = 0;


	UInt GetNumRow() const { return m_num_row; }

	ArrayView <ColumnInfo> GetColumns() const { return m_columns; }

	virtual RowCursor AddRow() = 0;			//see Update helper

	virtual void RemoveRow(UInt idx) = 0;	//see Delete helper


	virtual UInt CalculateStorageSize() const = 0;	//pre-compute file size

	virtual void Serialize(System::FileHandle & stream) const = 0;



	//secondary access (direct row access, can go out of bounds)

	RowCursor operator[](UInt idx);

	ConstRowCursor operator[](UInt idx) const;


	RowCursor begin();

	RowCursor end();

	ConstRowCursor begin() const;

	ConstRowCursor end() const;



	//tertiary (advanced sub-row access, can corrupt data)

	template <bool CONST> struct CellPtrImpl;

	template <class TYPE, bool CONST> struct ValueCellImpl;

	template <class TYPE, bool CONST> struct ArrayCellImpl;

	template <class TYPE, bool CONST> struct CellImpl;

	template <class TYPE> using CellReader = CellImpl <TYPE, true>;

	template <class TYPE> using CellWriter = CellImpl <TYPE, false>;


	virtual void ClearRow(UInt8 * rowptr) = 0;

	virtual void ClearHeapCell(UInt32 * heapcell, UInt8 alignment) = 0;

	virtual void SetHeapCell(UInt32 * heapcell, UInt8 alignment, Data::Archive::View data) = 0;

	virtual Data::Archive::View ReadHeapCell(const UInt32 * cellref, UInt8 alignment) const = 0;


	ArrayView <UInt8> GetNullRow() const { return m_null_row; }

	const UInt8 * GetRawData() const { return m_data; }

	UInt GetRowByteSize() const { return m_null_row.size; }



	//debug

	const UInt32 debug_data_epoch = 0;



protected:

	void Publish(ArrayView <ColumnInfo> columns, ArrayView <UInt8> null_row);

	void SetData(UInt8 * ptr, UInt num_row);
		

	ArrayView <ColumnInfo> m_columns;

	ArrayView <UInt8> m_null_row;

	UInt m_num_row = 0;

	UInt8 * m_data;
};




//
//impl

template <bool CONST> 
struct Reflex::Data::Table::CellPtrImpl
{
	using Byte = ConditionalType < CONST, const UInt8, UInt8 >;

	using TableRef = ConditionalType < CONST, ConstTRef <Table>, TRef <Table> >;

	using RowType = ConditionalType < CONST, ConstRowCursor, RowCursor >;


	CellPtrImpl(TableRef table, const Table::ColumnInfo & col, UInt row = 0)
		: adr(RemoveConst(table->GetRawData()) + col.offset + (row * table->GetRowByteSize()))
		, rowbytesize(table->GetRowByteSize())
#if REFLEX_DEBUG
		, m_debug_table(table)
		, m_debug_data_epoch(table->debug_data_epoch)
#endif
	{
	}

	CellPtrImpl(const RowType & row, const Table::ColumnInfo & col)
		: adr(row.adr + col.offset)
		, rowbytesize(row.rowbytesize)
#if REFLEX_DEBUG
		, m_debug_table(row.table)
		, m_debug_data_epoch(row.table->debug_data_epoch)
#endif
	{
	}

	CellPtrImpl(Byte * adr, UInt rowbytesize)
		: adr(adr)
		, rowbytesize(rowbytesize)
#if REFLEX_DEBUG
		, m_debug_data_epoch(0)
#endif
	{
	}

	void operator+=(Int nrow) { ValidateAccess(); adr += (Reinterpret<Int>(rowbytesize) * nrow); }
	void operator-=(Int nrow) { ValidateAccess(); adr -= (Reinterpret<Int>(rowbytesize) * nrow); }

	void operator++() { ValidateAccess(); adr += rowbytesize; }
	void operator--() { ValidateAccess(); adr -= rowbytesize; }

	bool operator<(const CellPtrImpl & b) const { return adr < b.adr; }
	bool operator<=(const CellPtrImpl & b) const { return adr <= b.adr; }
	bool operator>=(const CellPtrImpl & b) const { return adr >= b.adr; }
	bool operator>(const CellPtrImpl & b) const { return adr > b.adr; }
	bool operator==(const CellPtrImpl & b) const { return adr == b.adr; }
	bool operator!=(const CellPtrImpl & b) const { return adr != b.adr; }


	template <class TYPE> TYPE ReadValue() const;

	void ValidateAccess() const
	{
#if REFLEX_DEBUG
		if (m_debug_table)
		{
			REFLEX_ASSERT(m_debug_data_epoch == m_debug_table->debug_data_epoch);
		}
#endif
	}


	Byte * adr;
	
	UInt rowbytesize;

#if REFLEX_DEBUG
	ConstTRef<Table> m_debug_table;

	UInt32 m_debug_data_epoch;
#endif
};

template <bool CONST>
struct Reflex::Data::Table::RowCursorImpl : public CellPtrImpl <CONST>
{
	using Base = CellPtrImpl <CONST>;

	using TableRef = typename Base::TableRef;

	using Byte = typename Base::Byte;


	RowCursorImpl()
		: RowCursorImpl(REFLEX_NULL(Table))
	{
	}

	RowCursorImpl(TableRef table)
		: RowCursorImpl(table, RemoveConst(table->m_null_row.data))
	{
	}

	RowCursorImpl(TableRef table, UInt row)
		: RowCursorImpl(table, RemoveConst(table->GetRawData()) + (row * table->m_null_row.size))
	{
	}

	RowCursorImpl(TableRef table, Int row)
		: RowCursorImpl(table, Reinterpret<UInt>(row))
	{
	}

	RowCursorImpl(TableRef table, Byte * rowptr)	//ADVANCED
		: Base(rowptr, table->m_null_row.size),
		table(table)
	{
#if REFLEX_DEBUG
		this->m_debug_table = table;
		this->m_debug_data_epoch = table->debug_data_epoch;
#endif
	}

	RowCursorImpl(const RowCursorImpl & v) = default;

	RowCursorImpl(RowCursorImpl && t) = default;


	void SetIndex(UInt row);

	UInt GetIndex() const;


	template <class TYPE> void WriteValue(const ColumnInfo & info, const TYPE & value) const;

	template <class TYPE> void WriteArray(const ColumnInfo & info, const ArrayView <TYPE> & values) const;


	template <class TYPE> TYPE ReadValue(const ColumnInfo & info) const;

	template <class TYPE> ArrayView <TYPE> ReadArray(const ColumnInfo & info) const;


	RowCursorImpl & operator*() { REFLEX_ASSERT(*this); return *this; }

	const RowCursorImpl & operator*() const { REFLEX_ASSERT(*this); return *this; }


	RowCursorImpl & operator=(const RowCursorImpl & row) = default;

	explicit operator bool() const 
	{ 
		Base::ValidateAccess();

		bool valid = adr != table->m_null_row.data;

		REFLEX_ASSERT(valid ? IsValid(table) : true);

		return valid;
	}


	void Clear() const;



	//links
	
	using Base::rowbytesize;

	using Base::adr;

	TableRef table;

};




//
//impl

REFLEX_NS(Reflex::Data)

template <bool CONST> inline Table::RowCursorImpl <CONST> operator+(const Table::RowCursorImpl <CONST> & a, Int32 nrow)
{
	auto t = a;

	t += nrow;

	return t;
}

template <bool CONST> inline Table::RowCursorImpl <CONST> operator-(const Table::RowCursorImpl <CONST> & a, Int32 nrow)
{
	auto t = a;

	t -= nrow;

	return t;
}

template <bool CONST> inline Table::CellPtrImpl <CONST> operator+(const Table::CellPtrImpl <CONST> & a, Int32 nrow)
{
	auto t = a;

	t += nrow;

	return t;
}

template <class TYPE, bool CONST> inline Table::CellImpl <TYPE,CONST> operator+(const Table::CellImpl <TYPE,CONST> & a, Int32 nrow)
{
	auto t = a;

	t += nrow;

	return t;
}

template <class TYPE> inline Data::Archive::View PackRawArray(const ArrayView <TYPE> & view)
{
	REFLEX_STATIC_ASSERT(Data::Detail::IsRawPackable< ArrayView<TYPE> >::value);

	if constexpr (IsType<TYPE,UInt8>::value)
	{
		return view;
	}
	else
	{
		return Data::Pack(view);
	}
}

template <class TYPE> inline ArrayView <TYPE> UnpackRawArray(const Data::Archive::View & raw)
{
	REFLEX_STATIC_ASSERT(Data::Detail::IsRawPackable<TYPE>::value);

	if constexpr (IsType<TYPE,UInt8>::value)
	{
		return raw;
	}
	else
	{
		return Data::Unpack<ArrayView<TYPE>>(raw);
	}
}

REFLEX_END

template <bool CONST> struct Reflex::IsBoolCastable < Reflex::Data::Table::RowCursorImpl <CONST> > { static const bool value = true; };

REFLEX_NS(Reflex::Data::Detail)

enum RawDataType : UInt8
{
	kRawDataTypeVariable = 0,

	kRawDataTypeValue8 = 1,
	kRawDataTypeValue32 = 4,
	kRawDataTypeValue64 = 8,

	kNumRawDataType,
};

template <class TYPE> 
struct ValueBytes
{
	static_assert(std::is_trivially_copyable_v<TYPE>);

	UInt8 bytes[sizeof(TYPE)];	//explicity *not* aligned

	TYPE Read() const
	{
		static_assert(sizeof(*this)==sizeof(TYPE));
		static_assert(alignof(ValueBytes) == 1);

		return std::bit_cast<TYPE>(*this);
	}

	void Write(const TYPE & value)
	{
		static_assert(sizeof(*this) == sizeof(TYPE));
		static_assert(alignof(ValueBytes) == 1);

		*this = std::bit_cast<ValueBytes>(value);
	}
};

template <class TYPE> consteval UInt8 GetHeapAlignment()
{
	if constexpr ((sizeof(TYPE) == 1) || (sizeof(TYPE) == 2) || (sizeof(TYPE) == 4) || (sizeof(TYPE) == 8))
	{
		return UInt8(sizeof(TYPE));
	}
	else
	{
		REFLEX_STATIC_ASSERT(kIsType<TYPE, void>);	//always fail
	}
}

constexpr UInt16 kRawDataTypeSize[9] = { 4, 1, 2, 3, 4, 5, 6, 7, 8 };

extern const RawDataType kColumnTypeToRawDataType[Table::kNumColumnType];

REFLEX_END

template <bool CONST> template <class TYPE> inline TYPE Reflex::Data::Table::CellPtrImpl<CONST>::ReadValue() const 
{
	ValidateAccess();

	return Reinterpret< Detail::ValueBytes <TYPE> >(adr)->Read();
}

template <class TYPE, bool CONST>
struct Reflex::Data::Table::ValueCellImpl : public CellPtrImpl <CONST>
{
	static constexpr NullType verify[IsScalar<TYPE>::value];

	using CellPtrImpl<CONST>::CellPtrImpl;

	TYPE operator*() const { return CellPtrImpl<CONST>::template ReadValue<TYPE>(); }

	void Write(Table & table, const TYPE & value)
	{
		REFLEX_STATIC_ASSERT(!CONST);

		if constexpr (!CONST)
		{
			Reinterpret<Detail::ValueBytes<TYPE>>(this->adr)->Write(value);

			table.Notify();
		}
	}
};

template <class TYPE, bool CONST>
struct Reflex::Data::Table::ArrayCellImpl : public CellPtrImpl <CONST>
{
	using Base = CellPtrImpl <CONST>;


	ArrayCellImpl(typename Base::TableRef table, const ColumnInfo & col, UInt row = 0);

	ArrayCellImpl(const typename Base::RowType & row, const ColumnInfo & col);

	ArrayView <TYPE> operator*() const;

	void Clear();

	void Write(const ArrayView <TYPE> & value);


	typename Base::TableRef table;

	UInt8 alignment;
};

template <class TYPE, bool CONST> 
struct Reflex::Data::Table::CellImpl : public ValueCellImpl <TYPE,CONST>
{
	using ValueCellImpl<TYPE,CONST>::ValueCellImpl;
};

template <class TYPE>
struct Reflex::Data::Table::CellImpl < Reflex::ArrayRegion < TYPE >, false > : public ArrayCellImpl <TYPE,false>
{
	using ArrayCellImpl<TYPE,false>::ArrayCellImpl;
};

template <class TYPE>
struct Reflex::Data::Table::CellImpl < Reflex::ArrayView < TYPE >, true > : public ArrayCellImpl <TYPE,true>
{
	using ArrayCellImpl<TYPE,true>::ArrayCellImpl;
};

inline void Reflex::Data::Table::SetData(UInt8 * ptr, UInt num_row)
{
	m_data = ptr;
	m_num_row = num_row;

	REFLEX_IF_DEBUG(RemoveConst(Table::debug_data_epoch)++);

	Notify();
}

template <bool CONST> REFLEX_INLINE void Reflex::Data::Table::RowCursorImpl<CONST>::SetIndex(UInt idx)
{
	Base::ValidateAccess();

	const_cast<Byte*&>(adr) = RemoveConst(table->GetRawData()) + (idx * rowbytesize);
}

template <bool CONST> REFLEX_INLINE Reflex::UInt Reflex::Data::Table::RowCursorImpl<CONST>::GetIndex() const
{
	Base::ValidateAccess();

	return UInt((adr - table->GetRawData()) / rowbytesize);
}

template <class TYPE, bool CONST> inline Reflex::Data::Table::ArrayCellImpl<TYPE, CONST>::ArrayCellImpl(typename Base::TableRef table, const ColumnInfo & col, UInt row)
	: CellPtrImpl<CONST>(table, col, row)
	, table(table)
	, alignment(Detail::GetHeapAlignment<TYPE>())
{
	REFLEX_ASSERT(Detail::kColumnTypeToRawDataType[col.type] == Detail::kRawDataTypeVariable);
}

template <class TYPE, bool CONST> inline Reflex::Data::Table::ArrayCellImpl<TYPE,CONST>::ArrayCellImpl(const typename Base::RowType & row, const ColumnInfo & col)
	: CellPtrImpl<CONST>(row, col)
	, table(row.table)
	, alignment(Detail::GetHeapAlignment<TYPE>())
{
	REFLEX_ASSERT(Detail::kColumnTypeToRawDataType[col.type] == Detail::kRawDataTypeVariable);
}

template <class TYPE, bool CONST> inline Reflex::ArrayView <TYPE> Reflex::Data::Table::ArrayCellImpl<TYPE,CONST>::operator*() const
{
	Base::ValidateAccess();

	return UnpackRawArray<TYPE>(table->ReadHeapCell(Reinterpret<UInt32>(Base::adr), alignment));
}

template <class TYPE, bool CONST> inline void Reflex::Data::Table::ArrayCellImpl<TYPE,CONST>::Clear()
{
	REFLEX_STATIC_ASSERT(!CONST);

	if constexpr (!CONST)
	{
		Base::ValidateAccess();

		table->ClearHeapCell(Reinterpret<UInt32>(Base::adr), alignment);
	}
}

template <class TYPE, bool CONST> inline void Reflex::Data::Table::ArrayCellImpl<TYPE,CONST>::Write(const ArrayView <TYPE> & value)
{
	REFLEX_STATIC_ASSERT(!CONST);

	if constexpr (!CONST)
	{
		Base::ValidateAccess();

		table->SetHeapCell(Reinterpret<UInt32>(Base::adr), alignment, PackRawArray(value));
	}
}

template <bool CONST> inline void Reflex::Data::Table::RowCursorImpl<CONST>::Clear() const
{
	REFLEX_STATIC_ASSERT(!CONST);

	if constexpr (!CONST)
	{
		Base::ValidateAccess();

		table->ClearRow(Base::adr);
	}
}

template <bool CONST> template <class TYPE> REFLEX_INLINE void Reflex::Data::Table::RowCursorImpl<CONST>::WriteValue(const ColumnInfo & info, const TYPE & value) const
{
	REFLEX_STATIC_ASSERT(!CONST);

	if constexpr (!CONST)
	{
		Base::ValidateAccess();

		Reinterpret<Detail::ValueBytes<TYPE>>(Base::adr + info.offset)->Write(value);

		table->Notify();
	}
}

template <bool CONST> template <class TYPE> REFLEX_INLINE void Reflex::Data::Table::RowCursorImpl<CONST>::WriteArray(const ColumnInfo & info, const ArrayView <TYPE> & value) const
{
	REFLEX_STATIC_ASSERT(!CONST);

	if constexpr (!CONST)
	{
		Base::ValidateAccess();

		table->SetHeapCell(Reinterpret<UInt32>(Base::adr + info.offset), Detail::GetHeapAlignment<TYPE>(), PackRawArray(value));
	}
}

template <bool CONST> template <class TYPE> REFLEX_INLINE TYPE Reflex::Data::Table::RowCursorImpl<CONST>::ReadValue(const ColumnInfo & info) const
{
	Base::ValidateAccess();

	REFLEX_ASSERT(Detail::kColumnTypeToRawDataType[info.type] == sizeof(TYPE));

	return Reinterpret<Detail::ValueBytes<TYPE>>(Base::adr + info.offset)->Read();
}

template <bool CONST> template <class TYPE> REFLEX_INLINE Reflex::ArrayView <TYPE> Reflex::Data::Table::RowCursorImpl<CONST>::ReadArray(const ColumnInfo & info) const
{
	Base::ValidateAccess();

	REFLEX_ASSERT(Detail::kColumnTypeToRawDataType[info.type] == Detail::kRawDataTypeVariable);

	return UnpackRawArray<TYPE>(table->ReadHeapCell(Reinterpret<UInt32>(Base::adr + info.offset), Detail::GetHeapAlignment<TYPE>()));
}

inline Reflex::Data::Table::RowCursor Reflex::Data::Table::operator[](UInt idx)
{
	REFLEX_ASSERT(idx < GetNumRow());

	return RowCursor(*this, idx);
}

inline Reflex::Data::Table::ConstRowCursor Reflex::Data::Table::operator[](UInt idx) const
{
	REFLEX_ASSERT(idx < GetNumRow());

	return ConstRowCursor(*this, idx);
}

inline Reflex::Data::Table::RowCursor Reflex::Data::Table::begin()
{
	return { *this, 0 };
}

inline Reflex::Data::Table::RowCursor Reflex::Data::Table::end()
{
	return { *this, GetNumRow() };
}

inline Reflex::Data::Table::ConstRowCursor Reflex::Data::Table::begin() const
{
	return { *this, 0 };
}

inline Reflex::Data::Table::ConstRowCursor Reflex::Data::Table::end() const
{
	return { *this, GetNumRow() };
}
