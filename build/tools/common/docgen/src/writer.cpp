#include "../include/docgen.h"




//
//writer

REFLEX_SET_TRAIT(Docgen::Item, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Docgen::TypeItem, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Docgen::TypedefItem, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Docgen::FunctionItem, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Docgen::GlobalItem, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Docgen::MethodItem, IsSingleThreadExclusive);
REFLEX_SET_TRAIT(Docgen::MemberItem, IsSingleThreadExclusive);

REFLEX_NS(Reflex::Detail)

template <> struct Stringizer <Docgen::Writer::Field>
{
	static inline Docgen::Writer * st_current = 0;

	static UInt Call(ArrayRegion <char> & buffer, const Docgen::Writer::Field & value)
	{
		auto itr = Copy(buffer);

		if (value.is_const) WriteString(itr, "const ");

		WriteString(itr, st_current->GetType(value.type)->name);

		if (value.is_pointer) WriteString(itr, " *");
		if (value.is_ref) WriteString(itr, " &");

		return buffer.size - itr.size;
	}
};

REFLEX_END

REFLEX_BEGIN_INTERNAL(Docgen)

REFLEX_NOINLINE UInt64 HashFunctionSignature(const Writer::Field & rtn, const ArrayView <Writer::Field> & args)
{
	UInt64 hash = kHashSeed;

	Reflex::Detail::IncrementHash(hash, rtn.type);

	Reflex::Detail::IncrementHash(hash, MakeBits(rtn.is_const, rtn.is_pointer, rtn.is_ref));

	for (auto & i : args)
	{
		Reflex::Detail::IncrementHash(hash, i.type);

		Reflex::Detail::IncrementHash(hash, MakeBits(i.is_const, i.is_pointer, i.is_ref));
	}

	return hash;
}

struct WriterImpl : public Writer
{
	WriterImpl(Key32 language);

	~WriterImpl();


	Key32 RegisterString(CString && string) override
	{
		Key32 id = string;

		m_strings.Acquire(id, std::move(string));

		return id;
	}

	const TypeItem * AddType(Symbol symbol, TypeID type_id, TypeID base, TypeID src_template_type_id, UInt16 type_flags, const CString::View & ns, const CString::View & name, const ArrayView <TypeID> & targs) override;

	void AddTypedef(TypeID type_id, const CString::View & ns, const CString::View & name) override;

	void AddGlobal(const CString::View & ns, Field type) override;

	void AddFunction(const CString::View & ns, const CString::View & fn, Field rtn, const ArrayView <Field> & args, const ArrayView <Field> & targs) override;

	void AddMember(TypeID type_id, Field variable) override;

	void AddMethod(TypeID type_id, const CString::View & fn, Field rtn_t, const ArrayView <Field> & args, const ArrayView <Field> & targs, bool is_const, bool is_virtual) override;

	CString::View GetString(Key32 key) const override;

	const Item * GetItem(Symbol symbol) const override;

	const TypeItem * GetType(TypeID type_id) const override
	{
		TypeItem * null = nullptr;

		return *m_types.Search(type_id, &null);
	}

	TypeID GetTypeID(Symbol symbol) const override
	{
		for (auto & i : m_types)
		{
			if (i.value->symbol == symbol) return i.key;
		}

		return 0;
	}

	void EnumerateSymbols(const Function <void(const Item & item)> & visitor) const override
	{
		for (auto & i : m_symbols)
		{
			visitor(i.value);
		}
	}



	//internal

	template <typename SYMBOL_TYPE> SYMBOL_TYPE * RegisterSymbol(Symbol symbol, const CString::View & ns, const CString::View & name);

	Item * RegisterSymbol(Symbol symbol, Item::Category symbol_type, const CString::View & ns, const CString::View & name);

	FunctionItem::Signature & AddFunctionImpl(Item::Category symbol_type, const CString::View & ns, const CString::View & name, const ArrayView <Field> & targs, Field rtn, const ArrayView <Field> & args, FunctionPointer <void(FunctionItem&)> on_init)
	{
		Reflex::Detail::Stringizer<Field>::st_current = this;

		CheckFunctionSignature(ns, name, rtn, args);

		Symbol symbol = { ns, name };

		auto & pfunction = m_functions[symbol];

		if (!pfunction)
		{
			pfunction = Cast<FunctionItem>(RegisterSymbol(symbol, symbol_type, ns, name));

			on_init(*pfunction);
		}

		auto signature = HashFunctionSignature(rtn, args);

		for (auto & i : pfunction->overloads)
		{
			Assert(signature != i.hash, "duplicate overload", ns, name);
		}

		FunctionItem::Signature overload;

		for (auto & i : targs) overload.targs.Push(ConvertField(pfunction, i));
		overload.hash = signature;
		overload.rtn = ConvertField(pfunction, rtn);
		for (auto & i : args) overload.arguments.Push(ConvertField(pfunction, i));

		return pfunction->overloads.Push(overload);
	}

	const TypeItem * AssumeAnyType(TypeID type_id, const CString::View & error, const CString::View & ns, const CString::View & name) const
	{
		auto type = GetType(type_id);

		Assert(type, error, ns, name);

		return type;
	}

	const TypeItem * AssumeNonEnumType(TypeID type_id, const CString::View & error, const CString::View & ns, const CString::View & name) const
	{
		auto type = AssumeAnyType(type_id, error, ns, name);
		
		Assert(Not(type->flags & TypeItem::kFlagEnum), error, ns, name);

		return type;
	}

	Docgen::Field ConvertField(const Item * powner, Field field) const
	{
		auto type = AssumeAnyType(field.type, "invalid field", powner->ns, powner->name);

		Docgen::Field c;
		
		c.type = type->symbol;
		
		c.is_const = field.is_const;
		c.is_pointer = field.is_pointer;
		c.is_ref = field.is_ref;
		c.name = field.name;

		return c;
	}

	void CheckFunctionSignature(const CString::View & ns, const CString::View & name, Field rtn, const ArrayView <Field> & args) const
	{
		AssumeAnyType(rtn.type, "unknown return type", ns, name);

		char string[] = "unknown argument type [1]";

		ArrayRegion <char> region(string);

		for (auto & i : args)
		{
			AssumeAnyType(i.type, ToView(region), ns, name);

			region[region.size - 2]++;
		}
	}



	Item m_unknown;

	Item * m_fallback;

	CString m_null_string;

	
	Map <Key32,CString> m_strings;

	Map < Symbol, Reference <Item> > m_symbols;

	Map < TypeID, TypeItem* > m_types;

	Map < Symbol, FunctionItem* > m_functions;

	UInt16 m_idx_counter;


	inline static const TypeItem * st_method_class = 0;
};

#define DOC_TEMPLATE_PLACEHOLDER(w,TYPE) w.AddType({{}, REFLEX_STRINGIFY(TYPE)}, REFLEX_TYPEID(TYPE), 0, 0, Docgen::TypeItem::kFlagTemplatePlaceholder, "", REFLEX_STRINGIFY(TYPE))

Docgen::WriterImpl::WriterImpl(Key32 language)
	: Writer(language)
	, m_fallback(&m_unknown)
	, m_idx_counter(0)
{
	m_unknown.category = Item::kNumCategory;

	if (language == kcpp)
	{
		Writer & w = *this;

		DOC_TEMPLATE_PLACEHOLDER(w, TYPE);
		DOC_TEMPLATE_PLACEHOLDER(w, TYPE1);
		DOC_TEMPLATE_PLACEHOLDER(w, TYPE2);
		w.AddType({ {}, "VARGS..." }, REFLEX_TYPEID(VARGS), 0, 0, TypeItem::kFlagTemplatePlaceholder, "", "VARGS...", {});
		DOC_TEMPLATE_PLACEHOLDER(w, KEY);
		DOC_TEMPLATE_PLACEHOLDER(w, VALUE);
		DOC_TEMPLATE_PLACEHOLDER(w, SIZE);

		DOC_TEMPLATE_PLACEHOLDER(w, RTN);
		w.AddType({ {}, "RTN(VARGS...)" }, REFLEX_TYPEID(SIGNATURE), 0, 0, TypeItem::kFlagTemplatePlaceholder, "", "RTN(VARGS...)", {});

		DOC_TEMPLATE_PLACEHOLDER(w, MEMBER);
	}
}

Docgen::WriterImpl::~WriterImpl()
{
}

const TypeItem * WriterImpl::AddType(Symbol symbol, TypeID type_id, TypeID base, TypeID src_template, UInt16 type_flags, const CString::View & ns, const CString::View & name, const ArrayView <TypeID> & targs)
{
	if (auto type = RegisterSymbol<TypeItem>(symbol, ns, name))
	{
		if (base)
		{
			type->base = AssumeNonEnumType(base, "unknown base type", ns, name);
		}

		if (src_template)
		{
			type->source_template = AssumeNonEnumType(src_template, "unknown src template", ns, name);
		}

		type->class_ns = RegisterString(PrependNamespace(ns, name));

		for (auto & i : targs)
		{
			auto targ = AssumeAnyType(i, "unknown template arg", ns, name);

			type->targs.Push(targ);
		}

		type->flags = type_flags;

		type->method_count = 0;

		type->member_count = 0;

		m_types.Set(type_id, type);

		return type;
	}

	return nullptr;
}

void WriterImpl::AddTypedef(TypeID type_id, const CString::View & ns, const CString::View & name)
{
	Reflex::Detail::Stringizer<Field>::st_current = this;

	auto type = AssumeAnyType(type_id, "AddTypeDef", ns, name);

	RegisterSymbol<TypedefItem>({ ns, name }, ns, name)->source_type = type;
}

void WriterImpl::AddGlobal(const CString::View & ns, Field variable)
{
	Reflex::Detail::Stringizer<Field>::st_current = this;

	AssumeAnyType(variable.type, "AddGlobal", ns, variable.name);

	auto global = RegisterSymbol<GlobalItem>({ ns, variable.name }, ns, variable.name);

	global->variable = ConvertField(global, variable);
}

void WriterImpl::AddFunction(const CString::View & ns, const CString::View & name, Field rtn, const ArrayView <Field> & args, const ArrayView <Field> & targs)
{
	AddFunctionImpl(Item::kCategoryFunction, ns, name, targs, rtn, args, [](FunctionItem&)
	{
	});
}

void WriterImpl::AddMember(TypeID type_id, Field variable)
{
	Reflex::Detail::Stringizer<Field>::st_current = this;

	auto type = AssumeAnyType(type_id, "AddMember", {}, variable.name);

	auto class_ns = GetString(type->class_ns);

	AssumeAnyType(variable.type, "AddMember", class_ns, variable.name);

	auto member = RegisterSymbol<MemberItem>({ class_ns, variable.name }, class_ns, variable.name);

	member->variable = ConvertField(member, variable);

	member->owner = type;

	member->index = RemoveConst(type->member_count)++;
}

void WriterImpl::AddMethod(TypeID type_id, const CString::View & name, Field rtn, const ArrayView<Field> & args, const ArrayView <Field> & targs, bool is_const, bool is_virtual)
{
	Reflex::Detail::Stringizer<Field>::st_current = this;

	auto type = AssumeNonEnumType(type_id, "unknown type_id", "AddMethod", name);

	st_method_class = type;

	auto & overload = AddFunctionImpl(Item::kCategoryMethod, GetString(type->class_ns), name, targs, rtn, args, [](FunctionItem & function)
	{
		auto method = Cast<MethodItem>(function);

		method->owner = st_method_class;

		method->index = RemoveConst(st_method_class->method_count)++;
	});

	overload.is_const = is_const;

	overload.is_virtual = is_virtual;
}

CString::View WriterImpl::GetString(Key32 key) const
{
	return *m_strings.Search(key, &m_null_string);
}

const Item * WriterImpl::GetItem(Symbol symbol) const
{
	TRef <Item> fallback(kNoValue);

	return m_symbols.Search(symbol, &Reinterpret<Reference<Item>>(fallback))->Adr();
}

template <typename SYMBOL_TYPE> inline SYMBOL_TYPE * WriterImpl::RegisterSymbol(Symbol symbol, const CString::View & ns, const CString::View & name)
{
	if constexpr (IsType<SYMBOL_TYPE, TypeItem>::value)
	{
		return Cast<TypeItem>(RegisterSymbol(symbol, Item::kCategoryType, ns, name));
	}
	else if constexpr (IsType<SYMBOL_TYPE, TypedefItem>::value)
	{
		return Cast<TypedefItem>(RegisterSymbol(symbol, Item::kCategoryTypedef, ns, name));
	}
	else if constexpr (IsType<SYMBOL_TYPE, FunctionItem>::value)
	{
		return Cast<FunctionItem>(RegisterSymbol(symbol, Item::kCategoryFunction, ns, name));
	}
	else if constexpr (IsType<SYMBOL_TYPE, GlobalItem>::value)
	{
		return Cast<GlobalItem>(RegisterSymbol(symbol, Item::kCategoryGlobal, ns, name));
	}
	else if constexpr (IsType<SYMBOL_TYPE, MethodItem>::value)
	{
		return Cast<MethodItem>(RegisterSymbol(symbol, Item::kCategoryMethod, ns, name));
	}
	else if constexpr (IsType<SYMBOL_TYPE, MemberItem>::value)
	{
		return Cast<MemberItem>(RegisterSymbol(symbol, Item::kCategoryMember, ns, name));
	}
	else
	{
		REFLEX_ASSERT(false); // fallback for unhandled types

		return nullptr;
	}
}

Item * WriterImpl::RegisterSymbol(Symbol symbol, Item::Category type, const CString::View & ns, const CString::View & name)
{
	REFLEX_ASSERT(name);

	constexpr UInt32 kSymbolSizes[] = { kSizeOf<TypeItem>, kSizeOf<TypedefItem>, kSizeOf<FunctionItem>, kSizeOf<GlobalItem>, kSizeOf<MethodItem>, kSizeOf<MemberItem> };

	Assert(!m_symbols.Search(symbol), "duplicate symbol", ns, name);

	Reflex::Detail::Stringizer<Field>::st_current = this;

	auto where = g_default_allocator->Allocate(kSymbolSizes[type], REFLEX_ALLOCINFO(Item));

	switch (type)
	{
	case Item::kCategoryType:
		Reflex::Detail::Constructor<TypeItem>::Construct(where);
		break;

	case Item::kCategoryTypedef:
		Reflex::Detail::Constructor<TypedefItem>::Construct(where);
		break;

	case Item::kCategoryFunction:
		Reflex::Detail::Constructor<FunctionItem>::Construct(where);
		break;

	case Item::kCategoryGlobal:
		Reflex::Detail::Constructor<GlobalItem>::Construct(where);
		break;

	case Item::kCategoryMethod:
		Reflex::Detail::Constructor<MethodItem>::Construct(where);
		break;

	case Item::kCategoryMember:
		Reflex::Detail::Constructor<MemberItem>::Construct(where);
		break;

	default:
		throw(CString("Unknown symbol type"));
	}

	auto sym = Reinterpret<Item>(where);

	sym->SetOnHeap(g_default_allocator);

	sym->symbol = symbol;
	sym->category = type;
	sym->index = ++m_idx_counter;
	sym->ns = ns;
	sym->name = name;

	m_symbols.Set(symbol, sym);

	REFLEX_ASSERT(m_idx_counter < kMaxUInt16);

	return sym;
}

REFLEX_END_INTERNAL

Docgen::Writer::Writer(Key32 language)
	: language(language)
{
}

TRef <Docgen::Writer> Docgen::Writer::Create(Key32 language)
{
	return New<WriterImpl>(language);
}

