#include "pack.h"
#include "../include/docgen/cpp.h"
#include "../include/docgen/functions.h"




REFLEX_BEGIN_INTERNAL(Docgen)

constexpr CString::View kenum = "enum";
constexpr CString::View kobject = "object";
constexpr CString::View kvalue = "value";

typedef Map <const TypeItem*, const TypedefItem*> Typedefs;

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

		if (targ->category == kCategoryType)
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
		if (item.category == kCategoryTypedef)
		{
			typedefs.Set(Cast<TypedefItem>(&item)->source_type, Cast<TypedefItem>(&item));
		}
	});

	return MakeInstantiatedTemplateName(w, typedefs, name_, targs);
}

#if DOCGEN
void VisitModules(Map <const Module*> & visited, Map <const Module*> & marked, Array <const Module*> & sorted, const Module * m, Key32 language)
{
	if (visited.Search(m)) return;

	if (m->language != language) throw(CString("Module dependency belongs to a different language"));

	if (marked.Search(m)) throw(CString("Cyclic dependency detected"));

	marked.Set(m);

	for (auto & i : m->dependencies) VisitModules(visited, marked, sorted, i.Adr(), language);

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
		if (i.language == language && i.codebase == codebase)
		{
			VisitModules(visited, marked, sorted, &i, language);
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
	w.AddType({ ns, name }, type_id, 0, 0, kTypeFlagEnum, ns, name);

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

	w.AddType({ ns, name }, type_id, base_type_id, 0, object_t ? kTypeFlagObject : 0, ns, name, targs);
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

	Docformat::RegisterKeys(root);

	Array <const Item*> items_by_type[kNumCategory];

	writer.EnumerateSymbols([&items_by_type](const Item & item)
	{
		items_by_type[item.category].Push(&item);
	});

	const auto & types = items_by_type[kCategoryType];

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

			SetSymbol(fields, item.symbol);

			SetIndex(fields, item.index);


			SetCategory(fields, item.category);

			const TypeItem * null = nullptr;

			if (auto parent_type = *typenames.Search(item.ns, &null))
			{
				SetParent(fields, parent_type->symbol);

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

				if (actual_ns) SetNamespace(fields, actual_ns);
			}
			else
			{
				if (item.ns) SetNamespace(fields, item.ns);
			}

			SetName(fields, item.name);

			bool indexed = True(item.ns) || show_all;

			if (item.category == kCategoryType)
			{
				auto type = Cast<TypeItem>(item);

				bool template_instantiation = True(type->source_template);

				bool template_definition = True(type->targs) && Not(template_instantiation);

				if (type->targs)
				{
					Array <Symbol> targs;

					for (auto & i : type->targs)
					{
						targs.Push(i->symbol);
					}

					SetTemplateArgs(fields, targs);
				}

				SetTypeFlags(fields, TypeFlags(type->flags));
				
				if (auto base = type->base) SetTypeBase(fields, base->symbol);

				if (template_instantiation) SetTypeTemplateSource(fields, type->source_template->symbol);

				if (template_definition) SetTemplateDefinition(fields, template_definition);

				bool is_template_placeholder = True(type->flags & kTypeFlagTemplatePlaceholder);

				indexed = indexed && Not(template_instantiation || is_template_placeholder);
			}
			else
			{
				switch (item.category)
				{
				case kCategoryMember:
					SetParent(fields, Cast<MemberItem>(item)->owner->symbol);
				case kCategoryGlobal:
					Docformat::StoreField(fields, Cast<GlobalItem>(item)->variable);
					break;

				case kCategoryMethod:
					SetParent(fields, Cast<MethodItem>(item)->owner->symbol);
				case kCategoryFunction:
					Docformat::PackFunctionSignatures(fields, Cast<FunctionItem>(item)->overloads);
					break;

				case kCategoryTypedef:
					SetTypedefTarget(fields, Cast<TypedefItem>(item)->source_type->symbol);
					break;

				default:
					break;
				}
			}

			SetIndexed(fields, indexed);
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
