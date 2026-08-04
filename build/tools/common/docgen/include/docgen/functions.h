#pragma once

#include "writer.h"
#include "types.h"




//
//declarations

namespace Docgen
{

	void AddEnum(Writer & w, TypeID type_id, const CString::View & ns, const CString::View & name, const ArrayView <CString::View> & values);

	void AddEnumFromMacro(Writer & w, TypeID type_id, const CString::View & ns, const CString::View & name, const CString::View & values);


	void AddType(Writer & w, TypeID type_id, const CString::View & ns, const CString::View & name, UInt16 flags = 0);

	void AddTemplateDefinition(Writer & w, TypeID type_id, Reflex::Detail::DynamicTypeRef object_t, const CString::View & ns, const CString::View & name, const ArrayView <TypeID> & targs);

	void AddTemplateInstantiation(Writer & w, Symbol template_symbol, TypeID type_id, const ArrayView <TypeID> & targs);


	CString PrependNamespace(const CString::View & ns, const CString::View & symbol);

	void Assert(bool t, const CString::View & error, const CString::View & ns, const CString::View & name);

}




//
//reflex specific

#define DOC_REFLEX_FUNCTION_INSTANTIATION(w, SIG) Docgen::AddReflexFunctionInstantiation<SIG>(w, REFLEX_STRINGIFY(SIG))

REFLEX_NS(Docgen)

template <class SIG> inline void AddReflexFunctionInstantiation(Writer & w, const CString::View & sig_name)
{
	w.AddType({ {}, sig_name }, REFLEX_TYPEID(SIG), 0, 0, TypeItem::kFlagTemplatePlaceholder, "", sig_name);

	AddTemplateInstantiation(w, { "Reflex", "Function" }, REFLEX_TYPEID(Function<SIG>), { REFLEX_TYPEID(SIG) });
}

REFLEX_END

inline void Docgen::AddEnumFromMacro(Writer & w, TypeID type_id, const CString::View & ns, const CString::View & name, const CString::View & values)
{
	Array <CString::View> items = Split(values, ',');

	Remove(items, CString::View());

	AddEnum(w, type_id, ns, name, items);
}
