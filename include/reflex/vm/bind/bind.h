#pragma once

#include "../detail.h"
#include "../stack.h"




//
//bind

#define VM_TEMPLATE_TARG(ns_name, type) Reflex::VM::Detail::TemplateOf <type,K32(ns_name)>

#define VM_REGISTER_SYMBOL(bindings, name) (bindings).RegisterStaticString(K32(name), name)




//
//impl

#define VM_DOCUMENT(X, GROUP) Reflex::VM::Document(X, GROUP)

#define VM_DETAIL_GETARG(TRAITS, IDX) typename TRAITS::template ArgT<IDX>

REFLEX_NS(Reflex::VM::Detail)

template <class TYPE, UInt RTTID> struct TemplateOf : public TYPE { using Type = TYPE; static constexpr TypeID kRTTID = RTTID; using TYPE::TYPE; };

template <class TYPE>
struct TypeGetter
{
	static TypeRef Get(const Bindings & bindings) { return GetType<TYPE>(bindings); }
};

template <class TYPE, UInt RTTID>
struct TypeGetter < TemplateOf<TYPE,RTTID> >
{
	static TypeRef Get(const Bindings & bindings) { return bindings.GetTypeByRTTID(TemplateOf<TYPE, RTTID>::kRTTID); }
};

REFLEX_END




//
//detail

namespace Reflex { template <class TYPE, UInt RTTID> struct IsSingleThreadExclusive < VM::Detail::TemplateOf <TYPE,RTTID> > { static constexpr bool value = kIsSingleThreadExclusive<TYPE>; }; }

