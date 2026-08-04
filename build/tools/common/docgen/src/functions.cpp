#include "pack.h"
#include "../include/docgen/cpp.h"
#include "../include/docgen/functions.h"




REFLEX_BEGIN_INTERNAL(Docgen)

constexpr CString::View kenum = "enum";
constexpr CString::View kobject = "object";
constexpr CString::View kvalue = "value";

typedef Map <const TypeItem*, const TypedefItem*> Typedefs;

void SetSymbolProperty(Data::PropertySet & properties, Key32 id, Symbol symbol)
{
	REFLEX_ASSERT(symbol);

	Data::SetUInt64(properties, id, UInt64(symbol));
}

REFLEX_NOINLINE void StoreField(Data::PropertySet & node, const Field & arg)
{
	if (arg.name) Data::SetCString(node, kName, arg.name);

	SetSymbolProperty(node, kTypeID, arg.type);

	if (arg.is_const) Data::SetBool(node, kIsConst, true);

	if (arg.is_ref) Data::SetBool(node, kIsRef, true);

	if (arg.is_pointer) Data::SetBool(node, kIsPointer, true);
}

void PackFunctionSignatures(Data::PropertySet & propertyset, const decltype(FunctionItem::overloads) & in)
{
	auto signatures = Data::AcquirePropertySetArray(propertyset, kOverloads);

	for (auto & i : in)
	{
		auto signature = Data::AddPropertySet(signatures);

		if (i.is_const) Data::SetBool(signature, kIsConst, true);

		if (i.is_virtual) Data::SetBool(signature, kIsVirtual, true);

		StoreField(Data::AcquirePropertySet(signature, kReturn), i.rtn);

		ArrayView <Field> args_targs[2] = { i.arguments, i.targs };

		REFLEX_LOOP(idx, 2)
		{
			if (auto args = args_targs[idx])
			{
				auto arguments = Data::AcquirePropertySetArray(signature, kargs_targs[idx]);

				for (auto & i : args) StoreField(Data::AddPropertySet(arguments), i);
			}
		}
	}
}

REFLEX_NOINLINE CString::View MakeInstantiatedTemplateName(Writer & w, const Typedefs & typedefs, const CString::View & name_, const ArrayView <TypeID> & targs)
{
	//REFLEX_SHOWSTOPPER("this doesnt expand targ names properly with full namespace");

	REFLEX_ASSERT(w.language == kcpp);

	CString name = name_;

	name.Push('<');

	for (auto & i : targs)
	{
		auto targ = w.GetType(i);

		auto targ_name = PrependNamespace(targ->ns, targ->name);

		if (targ->category == Item::kCategoryType)
		{
			auto type_targ = Cast<TypeItem>(targ);

			auto & sub_targs = type_targ->targs;

			if (auto palias = typedefs.Search(type_targ))
			{
				auto alias = *palias;

				name.Append(PrependNamespace(alias->ns, alias->name));
			}
			else if (auto ntarg = sub_targs.GetSize())
			{
				TypeID sub_targs_ids[8];

				REFLEX_LOOP(idx, ntarg)
				{
					auto type_id = w.GetTypeID(sub_targs[idx]->symbol);

					Assert(type_id, "unknown type", {}, {});

					sub_targs_ids[idx] = type_id;
				}

				name.Append(MakeInstantiatedTemplateName(w, typedefs, targ_name, { sub_targs_ids, ntarg }));
			}
			else
			{
				name.Append(targ_name);
			}
		}
		else
		{
			name.Append(targ_name);
		}

		name.Push(',');
	}

	name.GetLast() = '>';

	return w.GetString(w.RegisterString(std::move(name)));
}

CString::View MakeInstantiatedTemplateName(Writer & w, const CString::View & name_, const ArrayView <TypeID> & targs)
{
	Typedefs typedefs;

	w.EnumerateSymbols([&typedefs](const Item & item)
	{
		if (item.category == Docgen::Item::kCategoryTypedef)
		{
			typedefs.Set(Cast<TypedefItem>(&item)->source_type, Cast<TypedefItem>(&item));
		}
	});

	return MakeInstantiatedTemplateName(w, typedefs, name_, targs);
}

#if DOCGEN
void VisitModules(Map <const Module*> & visited, Map <const Module*> & marked, Array <const Module*> & sorted, const Module * m)
{
	if (visited.Search(m)) return;

	if (marked.Search(m)) throw(CString("Cyclic dependency detected"));

	marked.Set(m);

	for (auto & i : m->dependencies) VisitModules(visited, marked, sorted, i.Adr());

	marked.Unset(m);

	visited.Set(m);

	sorted.Push<kAllocateNone>(m);
}

Array <const Module*> FilterAndSortModules(Key32 language, Key32 codebase)
{
	Map <const Module*> visited, marked;

	UInt n = 0;

	for (auto & i : Module::range) n += UInt(i.language == language);

	Array <const Module*> sorted;

	sorted.Allocate(n);

	for (auto & i : Module::range)
	{
		if (i.language == language)
		{
			VisitModules(visited, marked, sorted, &i);
		}
	}

	return sorted;
}

#endif

REFLEX_END_INTERNAL

REFLEX_NOINLINE void Docgen::Assert(bool test, const CString::View & error, const CString::View & ns, const CString::View & name)
{
	if (!test)
	{
		throw(Join(error, ':', ' ', ns, "::", name));
	}
}

REFLEX_NOINLINE void Docgen::AddEnum(Writer & w, TypeID type_id, const CString::View & ns, const CString::View & name, const ArrayView <CString::View> & values)
{
	w.AddType({ ns, name }, type_id, 0, 0, TypeItem::kFlagEnum, ns, name);

	for (auto & i : values)
	{
		Writer::Field field;
		
		field.type = type_id;
		field.name = Trim(i);
		field.is_const = true;

		w.AddMember(type_id, field);
	}
}

REFLEX_NOINLINE void Docgen::AddType(Writer & w, TypeID type_id, const CString::View & ns, const CString::View & name, UInt16 flags)
{
	w.AddType({ ns, name }, type_id, 0, 0, flags, ns, name);
}

REFLEX_NOINLINE void Docgen::AddTemplateDefinition(Writer & w, TypeID type_id, Reflex::Detail::DynamicTypeRef object_t, const CString::View & ns, const CString::View & name, const ArrayView <TypeID> & targs)
{
	TypeID base_type_id = object_t ? object_t->base->type_id : 0;

	w.AddType({ ns, name }, type_id, base_type_id, 0, object_t ? TypeItem::kFlagObject : 0, ns, name, targs);
}

REFLEX_NOINLINE void Docgen::AddTemplateInstantiation(Writer & w, Symbol template_definition, TypeID type_id, const ArrayView <TypeID> & targs)
{
	auto tmpl = Cast<TypeItem>(w.GetItem(template_definition));

	Assert(tmpl, "unknown template", {}, {});

	auto ns = tmpl->ns;
	auto name = tmpl->name;

	if (w.GetTypeID(tmpl->targs.GetFirst()->symbol) != REFLEX_TYPEID(VARGS))
	{
		Assert(tmpl->targs.GetSize() == targs.size, "template args mismatch", ns, name);
	}

	TypeID base_type_id = tmpl->base ? w.GetTypeID(tmpl->base->symbol) : 0;

	w.AddType({ tmpl->symbol.ns, MakeInstantiatedTemplateName(w, name, targs) }, type_id, base_type_id, w.GetTypeID(template_definition), tmpl->flags, ns, name, targs);
}

REFLEX_NOINLINE void Docgen::EnumerateModules(Key32 codebase, Output & output, Writer & w)
{
#if DOCGEN
	try
	{
		auto modules = FilterAndSortModules(w.language, codebase);

		for (auto & i : modules)
		{
			i->instantiate(w);
		}
	}
	catch (const CString & error)
	{
		output.Error(error);

		REFLEX_ASSERT_EX(false, error.GetData());
	}
#endif
}

REFLEX_NOINLINE void Docgen::ExportSymbols(const Writer & writer, Output & output, Data::PropertySet & root)
{
	bool show_all = writer.language != kcpp;

	auto keymap = Data::AcquireKeyMap(root);

	const char * kKeys[] =
	{
		"Symbol",
		"Category",
		"TypeFlags",
		"ParentID",
		"TypeID",
		"TemplateSourceID",
		"BaseID",
		"Name",
		"Namespace",
		"Index",
		"Indexed",
		"Overloads",
		"Return",
		"Arguments",
		"IsConst",
		"IsRef",
		"IsPointer",
		"IsStatic",
		"IsVirtual",
		"Data", // binary data, not indexable

		"enum",
		"value",
		"object",
	};

	for (auto & i : kKeys) Data::RegisterKey(keymap, i);

	Array <const Item*> items_by_type[Item::kNumCategory];

	writer.EnumerateSymbols([&items_by_type](const Item & item)
	{
		items_by_type[item.category].Push(&item);
	});

	const auto & types = items_by_type[Item::kCategoryType];

	Map <CString, const TypeItem*> typenames;

	for (auto i : types)
	{
		auto type = Cast<TypeItem>(i);

		typenames.Set(PrependNamespace(type->ns, type->name), type);
	}

	Map <UInt16,bool> index_check;

	for (auto & item_group : items_by_type)
	{
		for (auto i : item_group)
		{
			auto & item = *i;

		
			REFLEX_ASSERT(SetFiltered(index_check[item.index], true));

			auto fields = Data::AcquirePropertySet(root, item.index);

			SetSymbolProperty(fields, kSymbol, item.symbol);

			Data::SetUInt32(fields, kIndex, item.index);


			Data::SetCString(fields, kCategory, kCategories[item.category].b);

			const TypeItem * null = nullptr;

			if (auto parent_type = *typenames.Search(item.ns, &null))
			{
				SetSymbolProperty(fields, kParentID, parent_type->symbol);

				auto parts = Split(item.ns, kNamespaceDelimiter);

				CString actual_ns;

				parts.Pop();

				while (parts)
				{
					actual_ns = Merge(parts, kNamespaceDelimiter);

					if (typenames.Search(actual_ns))
					{
						parts.Pop();
					}
					else
					{
						break;
					}
				}

				REFLEX_ASSERT(actual_ns != "Debug");

				if (actual_ns) Data::SetCString(fields, kNamespace, actual_ns);
			}
			else
			{
				if (item.ns) Data::SetCString(fields, kNamespace, item.ns);
			}

			Data::SetCString(fields, kName, item.name);

			bool indexed = True(item.ns) || show_all;

			if (item.category == Item::kCategoryType)
			{
				auto type = Cast<TypeItem>(item);

				bool template_instantiation = True(type->source_template);

				bool template_definition = True(type->targs) && Not(template_instantiation);

				if (type->targs)
				{
					Array <UInt64> targs;

					for (auto & i : type->targs)
					{
						targs.Push(UInt64(i->symbol));
					}

					Data::SetUInt64Array(fields, kTemplateArgs, targs);
				}

				Data::SetUInt32(fields, kTypeFlags, UInt32(type->flags));
				
				if (auto base = type->base) SetSymbolProperty(fields, kBaseID, base->symbol);

				if (template_instantiation) SetSymbolProperty(fields, kTemplateSourceID, type->source_template->symbol);

				if (template_definition) Data::SetBool(fields, kIsTemplateDefinition, template_definition);

				bool is_template_placeholder = True(type->flags & TypeItem::kFlagTemplatePlaceholder);

				indexed = indexed && Not(template_instantiation || is_template_placeholder);
			}
			else
			{
				switch (item.category)
				{
				case Item::kCategoryMember:
					SetSymbolProperty(fields, kParentID, Cast<MemberItem>(item)->owner->symbol);
				case Item::kCategoryGlobal:
					StoreField(fields, Cast<GlobalItem>(item)->variable);
					break;

				case Item::kCategoryMethod:
					SetSymbolProperty(fields, kParentID, Cast<MethodItem>(item)->owner->symbol);
				case Item::kCategoryFunction:
					PackFunctionSignatures(fields, Cast<FunctionItem>(item)->overloads);
					break;

				case Item::kCategoryTypedef:
					SetSymbolProperty(fields, kTypeID, Cast<TypedefItem>(item)->source_type->symbol);
					break;

				default:
					break;
				}
			}

			Data::SetBool(fields, kIndexed, indexed);
		}
	}
}

void Docgen::ExportSymbols(Key32 language, Key32 codebase, Output & logger, const WString::View & path)
{
	auto writer = Make<Docgen::Writer>(language);

	Data::PropertySet output;

	EnumerateModules(codebase, logger, writer);

	ExportSymbols(writer, logger, output);

	auto encoded = Data::EncodePropertySet(Data::kPropertySheetFormat, output);

	if (File::Open(path) != encoded)
	{
		File::Save(path, encoded);
	}
}
