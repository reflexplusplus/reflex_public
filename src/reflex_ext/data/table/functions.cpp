#include "[include].h"




//
//

REFLEX_NS(Reflex::Data)

void ImportUInt32Property(Data::KeyMap *, const Table::RowCursor & row, const Table::ColumnInfo & column, const Object & object)
{
	row.WriteValue<UInt32>(column, Cast<Data::UInt32Property>(object)->value);
}

void ExportUInt32Property(const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
{
	Data::SetUInt32(fields, column.id, row.ReadValue<UInt32>(column));
}

void ImportUInt64Property(Data::KeyMap *, const Table::RowCursor & row, const Table::ColumnInfo & column, const Object & object)
{
	row.WriteValue<UInt64>(column, Cast<Data::UInt64Property>(object)->value);
}

void ExportUInt64Property(const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
{
	Data::SetUInt64(fields, column.id, row.ReadValue<UInt64>(column));
}

const UInt64 kNullValue64 = 0;
const UInt64 kNullKey64 = kHashSeed;

REFLEX_END

const Reflex::Data::Detail::PropertyConverter Reflex::Data::Detail::kPropertyConverters[Table::kNumColumnType] =
{
	//kDataTypeBool,
	{
		&REFLEX_TYPEID(Data::BoolProperty),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteValue<bool>(column, Cast<Data::BoolProperty>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetBool(fields, column.id, row.ReadValue<bool>(column));
		}
	},

	//kDataTypeUInt8,
	{
		&REFLEX_TYPEID(Data::UInt8Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteValue<UInt8>(column, Cast<Data::UInt8Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetUInt8(fields, column.id, row.ReadValue<UInt8>(column));
		}
	},

	//kDataTypeUInt32,
	{
		&REFLEX_TYPEID(Data::UInt32Property),
		&ImportUInt32Property,
		&ExportUInt32Property
	},
	
	//kDataTypeUInt64,
	{
		&REFLEX_TYPEID(Data::UInt64Property),
		&ImportUInt64Property,
		&ExportUInt64Property
	},

	//kDataTypeInt32
	{
		&REFLEX_TYPEID(Data::Int32Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteValue<Int32>(column, Cast<Data::Int32Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetInt32(fields, column.id, row.ReadValue<Int32>(column));
		}
	},

	//kDataTypeInt64
	{
		&REFLEX_TYPEID(Data::Int64Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteValue<Int64>(column, Cast<Data::Int64Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetInt64(fields, column.id, row.ReadValue<Int64>(column));
		}
	},

	//kDataTypeFloat32
	{
		&REFLEX_TYPEID(Data::Float32Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteValue<Float32>(column, Cast<Data::Float32Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetFloat32(fields, column.id, row.ReadValue<Float32>(column));
		}
	},

	//kDataTypeFloat64
	{
		&REFLEX_TYPEID(Data::Float64Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteValue<Float64>(column, Cast<Data::Float64Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetFloat64(fields, column.id, row.ReadValue<Float64>(column));
		}
	},

	//kDataTypeKey32
	{
		&REFLEX_TYPEID(Data::UInt32Property),
		&ImportUInt32Property,
		&ExportUInt32Property
	},

	//kDataTypeDate32
	{
		&REFLEX_TYPEID(Data::UInt32Property),
		&ImportUInt32Property,
		&ExportUInt32Property
	},

	//kDataTypeDate64
	{
		&REFLEX_TYPEID(Data::UInt64Property),
		&ImportUInt64Property,
		&ExportUInt64Property
	},

	//kDataTypeArrayOfUInt8
	{
		&REFLEX_TYPEID(Data::ArchiveObject),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<UInt8>(column, Cast<Data::ArchiveObject>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetBinary(fields, column.id, row.ReadArray<UInt8>(column));
		}
	},

	//kDataTypeArrayOfUInt32
	{
		&REFLEX_TYPEID(Data::ArrayOfUInt32Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<UInt32>(column, Cast<Data::ArrayOfUInt32Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetUInt32Array(fields, column.id, row.ReadArray<UInt32>(column));
		}
	},

	//kDataTypeArrayOfUInt64
	{
		&REFLEX_TYPEID(Data::ArrayOfUInt64Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<UInt64>(column, Cast<Data::ArrayOfUInt64Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetUInt64Array(fields, column.id, row.ReadArray<UInt64>(column));
		}
	},

	//kDataTypeArrayOfInt32
	{
		&REFLEX_TYPEID(Data::ArrayOfInt32Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<Int32>(column, Cast<Data::ArrayOfInt32Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetInt32Array(fields, column.id, row.ReadArray<Int32>(column));
		}
	},

	//kDataTypeArrayOfInt64
	{
		&REFLEX_TYPEID(Data::ArrayOfInt64Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<Int64>(column, Cast<Data::ArrayOfInt64Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetInt64Array(fields, column.id, row.ReadArray<Int64>(column));
		}
	},

	//kDataTypeArrayOfFloat32
	{
		&REFLEX_TYPEID(Data::ArrayOfFloat32Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<Float32>(column, Cast<Data::ArrayOfFloat32Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetFloat32Array(fields, column.id, row.ReadArray<Float32>(column));
		}
	},

	//kDataTypeArrayOfFloat64
	{
		&REFLEX_TYPEID(Data::ArrayOfFloat64Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<Float64>(column, Cast<ObjectOf<Array<Float64>>>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			fields.SetProperty(column.id, REFLEX_CREATE(ArrayOfFloat64Property, row.ReadArray<Float64>(column)));
		}
	},

	//kDataTypeArrayOfKey32
	{
		&REFLEX_TYPEID(Data::ArrayOfKey32Property),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<Key32>(column, Cast<Data::ArrayOfKey32Property>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetKey32Array(fields, column.id, row.ReadArray<Key32>(column));
		}
	},

	//kDataTypeStringASCII
	{
		&REFLEX_TYPEID(Data::CStringProperty),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<char>(column, Cast<Data::CStringProperty>(object)->value);
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetCString(fields, column.id, row.ReadArray<char>(column));
		}
	},

	//kDataTypeStringUTF8
	{
		&REFLEX_TYPEID(Data::WStringProperty),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			row.WriteArray<char>(column, Data::Unpack<CString::View>(Data::EncodeUTF8(Cast<Data::WStringProperty>(object)->value)));
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			Data::SetWString(fields, column.id, Data::DecodeUTF8(row.ReadArray<UInt8>(column)));
		}
	},

	//kDataTypeStringUCS2
	{
		&REFLEX_TYPEID(Data::WStringProperty),
		[](Data::KeyMap *, const Table::RowCursor& row, const Table::ColumnInfo & column, const Object & object)
		{
			auto binary = Data::EncodeUCS2(Cast<Data::WStringProperty>(object)->value);

			row.WriteArray<WString16::Type>(column, Data::Unpack<ArrayView<WString16::Type>>(binary));
		},
		[](const Data::KeyMap *, const Table::ConstRowCursor & row, const Table::ColumnInfo & column, Data::PropertySet & fields)
		{
			auto wchar16 = row.ReadArray<WString16::Type>(column);

			auto object = New<Data::WStringProperty>(wchar16.size);

			auto dst = object->value.GetData();

			for (auto & i : wchar16) *dst++ = WChar(i);

			fields.SetProperty(column.id, object);
		}
	},
};

const Reflex::UInt64 * Reflex::Data::Detail::kNullValues[Table::kNumColumnType] =
{
	&kNullValue64,

	&kNullValue64,
	&kNullValue64,
	&kNullValue64,

	&kNullValue64,
	&kNullValue64,

	&kNullValue64,
	&kNullValue64,

	&kNullKey64,

	&kNullValue64,
	&kNullValue64,

	&kNullValue64,
	&kNullValue64,
	&kNullValue64,
	&kNullValue64,

	&kNullValue64,
	&kNullValue64,

	&kNullValue64,
	&kNullValue64,

	&kNullValue64,
	&kNullValue64,

	&kNullValue64,
};

void Reflex::Data::Detail::ImportRow(ArrayView <PropertyConverter> converters, Data::KeyMap * keymap, const Table::RowCursor & row, ArrayView <Table::ColumnInfo> columns, const Data::PropertySet & fields)
{
	for (auto & i : fields.Iterate())
	{
		if (auto column = QueryColumn(columns, i.key.id))
		{
			auto & handler = converters[column->type];

			if (i.key.type_id == *handler.a)
			{
				handler.b(keymap, row, *column, i.value);
			}
			else
			{
				REFLEX_ASSERT(false);
			}
		}
	}
}

void Reflex::Data::Detail::ExportRow(ArrayView <PropertyConverter> converters, const Data::KeyMap * keymap, const Table::ConstRowCursor & row, Data::PropertySet & fields)
{
	for (auto & i : row.table->GetColumns())
	{
		auto handler = converters[i.type];

		handler.c(keymap, row, i, fields);
	}
}

void Reflex::Data::ExportRow(const Table::ConstRowCursor & row, Data::PropertySet & fields)
{
	Detail::ExportRow(ToView(Detail::kPropertyConverters), nullptr, row, fields);
}

const Reflex::Data::Table::ColumnInfo * Reflex::Data::QueryColumn(ArrayView <Table::ColumnInfo> columns, Key32 id)
{
	return Reinterpret<Table::ColumnInfo>(SearchValue<KeyCompare>(Reinterpret< ArrayView <ColumnInfoAsTuple> >(columns), id));
}

const Reflex::Data::Table::ColumnInfo & Reflex::Data::AssumeColumn(ArrayView <Table::ColumnInfo> columns, Key32 id, Table::ColumnType type)
{
	auto column = Reinterpret<Table::ColumnInfo>(SearchValue<KeyCompare>(Reinterpret< ArrayView <ColumnInfoAsTuple> >(columns), id));

	if (!column) throw(false);

	REFLEX_ASSERT(column->offset < kMaxUInt16);	//this likely means you havent read the columsns from Table::GetColumns()

	if (column->type != type) throw(false);

	return *column;
}

Reflex::WString Reflex::Data::ToWString(WString16::View string)
{
	WString rtn;

	rtn.SetSize(string.size);

	auto pto = rtn.GetData();

	REFLEX_LOOP_PTR(string.data, pfrom, string.size)
	{
		(*pto++) = *pfrom;
	}

	return rtn;
}

Reflex::Data::WString16 Reflex::Data::ToWString16(WString::View string)
{
	WString16 rtn;

	rtn.SetSize(string.size);

	auto pto = rtn.GetData();

	REFLEX_LOOP_PTR(string.data, pfrom, string.size)
	{
		(*pto++) = *pfrom;
	}

	return rtn;
}
