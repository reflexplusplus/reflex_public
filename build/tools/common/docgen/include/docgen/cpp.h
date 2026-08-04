#pragma once

#include "module.h"
#include "writer.h"
#include "types.h"
#include "functions.h"




namespace Docgen
{

	REFLEX_DECLARE_KEY32(cpp);

	struct TYPE {};
	struct TYPE1 {};
	struct TYPE2 {};
	struct VARGS {};

	struct KEY {};
	struct VALUE {};

	struct SIZE {};

	//for Function
	struct RTN {};
	struct SIGNATURE { void operator()() {}; };

	struct MEMBER {};


	template <class FN> struct Fn { static void Add(Writer & w, const CString::View & ns, const CString::View & fn, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {}); };

	template <class CLASS> struct Member;

	template <class CLASS> struct Method { static void Add(Writer & w, const CString::View & fn, const ArrayView<CString::View> & args); };

	template <class CLASS, class SIG> using MethodPointer = typename Reflex::Detail::MemberFunctionTraits<CLASS, SIG>::PointerType;

}




//
//C++ Documentation Macros

#ifdef REFLEX_OS_WINDOWS


//
//primary

#define DOC_IMPORT_TYPE(w,NS,TYPE) //TODO register an "external" type, without writing it to the database, for use for additional documentation databases for reflex-dependent projects


#define DOC_NAMESPACE(w, NS) w.RegisterKey(REFLEX_STRINGIFY(NS))


#define DOC_ENUM(w, NS, ENUM, ...) Docgen::AddEnumFromMacro(w, REFLEX_TYPEID(NS::ENUM), REFLEX_STRINGIFY(NS), REFLEX_STRINGIFY(ENUM), #__VA_ARGS__)


#define DOC_TYPE_EX(w,NS,TYPE,BASE_TYPEID,FLAGS) w.AddType({K32(REFLEX_STRINGIFY(NS)), K32(REFLEX_STRINGIFY(TYPE))}, REFLEX_TYPEID(NS::TYPE), BASE_TYPEID, 0, FLAGS, REFLEX_STRINGIFY(NS), REFLEX_STRINGIFY(TYPE))

#define DOC_VALUE_TYPE_EX(w,NS,TYPE,BASE_TYPE) REFLEX_STATIC_ASSERT(!Reflex::kIsObject<NS::TYPE>); DOC_TYPE_EX(w,NS,TYPE,Docgen::GetValueTypeBaseRTTID<BASE_TYPE>(),0)

#define DOC_VALUE_TYPE(w,NS,TYPE) DOC_VALUE_TYPE_EX(w,NS,TYPE,void)

#define DOC_OBJECT_TYPE_BASE_EX(w,NS,TYPE,BASE_TYPE) DOC_TYPE_EX(w,NS,TYPE,REFLEX_TYPEID(BASE_TYPE), Docgen::TypeItem::kFlagObject)

#define DOC_OBJECT_TYPE_EX(w,NS,TYPE,BASE_TYPE) DOC_OBJECT_TYPE_BASE_EX(w,NS,TYPE,BASE_TYPE)

#define DOC_OBJECT_TYPE(w,NS,TYPE) DOC_TYPE_EX(w,NS,TYPE,Docgen::GetObjectBaseRTTID<NS::TYPE>(), Docgen::TypeItem::kFlagObject)

#define DOC_TYPEDEF(w,ALIAS_NS,ALIAS_TYPE) w.AddTypedef(REFLEX_TYPEID(ALIAS_NS::ALIAS_TYPE), REFLEX_STRINGIFY(ALIAS_NS), REFLEX_STRINGIFY(ALIAS_TYPE));


#define DOC_TEMPLATE_OBJECT(w,NS,NAME,...) Docgen::AddTemplateDefinition(w, REFLEX_TYPEID(NS::NAME<__VA_ARGS__>), NS::NAME<__VA_ARGS__>::kDynamicTypeInfo, REFLEX_STRINGIFY(NS),REFLEX_STRINGIFY(NAME), Docgen::GetTypeIDs<__VA_ARGS__>())

#define DOC_TEMPLATE_VALUE(w,NS,NAME,...) Docgen::AddTemplateDefinition(w, REFLEX_TYPEID(NS::NAME<__VA_ARGS__>), nullptr, REFLEX_STRINGIFY(NS),REFLEX_STRINGIFY(NAME), Docgen::GetTypeIDs<__VA_ARGS__>())

#define DOC_TEMPLATE_INSTANTIATION(w,NS,NAME,...) Docgen::AddTemplateInstantiation(w, { REFLEX_STRINGIFY(NS),REFLEX_STRINGIFY(NAME) }, REFLEX_TYPEID(NS::NAME<__VA_ARGS__>), Docgen::GetTypeIDs<__VA_ARGS__>())


#define DOC_GLOBAL(w,NS,NAME) w.AddGlobal(REFLEX_STRINGIFY(NS), DOC_ARG(decltype(NS::NAME), REFLEX_STRINGIFY(NAME)));


#define DOC_FUNCTION_OVERLOAD_EX(w,SIG,NS,FN,...) Docgen::Fn<SIG>::Add(w,REFLEX_STRINGIFY(NS),REFLEX_STRINGIFY(FN),{ __VA_ARGS__ })

#define DOC_FUNCTION_OVERLOAD(w,SIG,NS,FN,...) DOC_FUNCTION_OVERLOAD_EX(w, FunctionPointer<SIG>, NS, FN, __VA_ARGS__); { [[maybe_unused]] FunctionPointer <SIG> verify = &NS::FN; }

#define DOC_FUNCTION(w,NS,FN,...) DOC_FUNCTION_OVERLOAD_EX(w,decltype(&NS::FN),NS,FN,__VA_ARGS__)

#define DOC_TEMPLATE_FUNCTION_1P(w,NS,FN,TYPE,...) Docgen::Fn<decltype(&NS::FN<Docgen::BoolResolverT<TYPE>::Type>)>::Add(w, REFLEX_STRINGIFY(NS), REFLEX_STRINGIFY(FN), { __VA_ARGS__ }, { DOC_RTN(TYPE) })

#define DOC_TEMPLATE_FUNCTION_2P(w,NS,FN,TYPE1,TYPE2,...) Docgen::Fn<decltype(&NS::FN<Docgen::BoolResolverT<TYPE1>::Type,Docgen::BoolResolverT<TYPE2>::Type>)>::Add(w,REFLEX_STRINGIFY(NS),REFLEX_STRINGIFY(FN),{ __VA_ARGS__ })


#define DOC_MEMBER(w,CLASS,MEMBER) Docgen::Member<decltype(&CLASS::MEMBER)>::Add(w,REFLEX_STRINGIFY(MEMBER))


#define DOC_METHOD(w,CLASS,FN,...) Docgen::Method<decltype(&CLASS::FN)>::Add(w,REFLEX_STRINGIFY(FN),{ __VA_ARGS__ })




//
//secondary

#define DOC_ARG(ARG,NAME) Docgen::Writer::Field { REFLEX_TYPEID(Docgen::StrippedType<ARG>), Reflex::IsConst<Reflex::NonRefT<Reflex::NonPointerT<ARG>>>::value, Reflex::IsPointer<ARG>::value, Reflex::IsReference<ARG>::value, false, NAME }

#define DOC_RTN(ARG) DOC_ARG(ARG,{})




//
//impl

REFLEX_NS(Docgen)

template <class TYPE>
struct BoolResolverT
{
	typedef TYPE Type;
};

template <>
struct BoolResolverT <bool>
{
	static const bool Type = true;
};

template <typename... TYPES> ArrayView<TypeID> GetTypeIDs()
{
	static TypeID ids[sizeof...(TYPES)];

	const TypeID * pids[] = { &Reflex::Detail::TypeIndex<TYPES>::value... };

	REFLEX_LOOP(idx, sizeof...(TYPES))
	{
		ids[idx] = *pids[idx];
	}

	return ToView(ids);
}

constexpr CString::View kMissingArgName = "missing argument name";

template <class TYPE> Reflex::TypeID GetObjectBaseRTTID()
{
	REFLEX_STATIC_ASSERT(Reflex::IsObject<TYPE>::value);

	if (auto base = TYPE::kDynamicTypeInfo.base)
	{
		return base->type_id;
	}
	else
	{
		return REFLEX_TYPEID(Object);
	}
}

template <class TYPE> constexpr Reflex::TypeID GetValueTypeBaseRTTID()
{
	if constexpr (Reflex::kIsType<TYPE, void>)
	{
		return 0;
	}
	else
	{
		return REFLEX_TYPEID(TYPE);
	}
}

template <class TYPE> using StrippedType = NonConstT<NonPointerT<NonRefT<TYPE>>>;

REFLEX_END

template <class RTN>
struct Docgen::Fn <RTN(*)()>
{
	static void Add(Writer & w, const CString::View & ns, const CString::View & name, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {})
	{
		w.AddFunction(ns, name, DOC_RTN(RTN), {}, targs);
	}
};

template <class RTN, class P1>
struct Docgen::Fn <RTN(*)(P1)>
{
	static void Add(Writer & w, const CString::View & ns, const CString::View & name, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {})
	{
		Assert(args.size == 1, kMissingArgName, ns, name);

		w.AddFunction(ns, name, DOC_RTN(RTN), { DOC_ARG(P1,args[0])}, targs);
	}
};

template <class RTN, class P1, class P2>
struct Docgen::Fn <RTN(*)(P1,P2)>
{
	static void Add(Writer & w, const CString::View & ns, const CString::View & name, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {})
	{
		Assert(args.size == 2, kMissingArgName, ns, name);

		w.AddFunction(ns, name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]),DOC_ARG(P2,args[1]) }, targs);
	}
};

template <class RTN, class P1, class P2, class P3>
struct Docgen::Fn <RTN(*)(P1,P2,P3)>
{
	static void Add(Writer & w, const CString::View & ns, const CString::View & name, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {})
	{
		Assert(args.size == 3, kMissingArgName, ns, name);

		w.AddFunction(ns, name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]),DOC_ARG(P2,args[1]),DOC_ARG(P3,args[2]) }, targs);
	}
};

template <class RTN, class P1, class P2, class P3, class P4>
struct Docgen::Fn <RTN(*)(P1,P2,P3,P4)>
{
	static void Add(Writer & w, const CString::View & ns, const CString::View & name, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {})
	{
		Assert(args.size == 4, kMissingArgName, ns, name);

		w.AddFunction(ns, name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]),DOC_ARG(P2,args[1]),DOC_ARG(P3,args[2]),DOC_ARG(P4,args[3]) }, targs);
	}
};

template <class RTN, class P1, class P2, class P3, class P4, class P5>
struct Docgen::Fn <RTN(*)(P1, P2, P3, P4, P5)>
{
	static void Add(Writer & w, const CString::View & ns, const CString::View & name, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {})
	{
		Assert(args.size == 5, kMissingArgName, ns, name);

		w.AddFunction(ns, name, DOC_RTN(RTN), { DOC_ARG(P1, args[0]), DOC_ARG(P2, args[1]), DOC_ARG(P3, args[2]), DOC_ARG(P4, args[3]), DOC_ARG(P5, args[4]) }, targs);
	}
};

template <class RTN, class P1, class P2, class P3, class P4, class P5, class P6>
struct Docgen::Fn <RTN(*)(P1, P2, P3, P4, P5, P6)>
{
	static void Add(Writer & w, const CString::View & ns, const CString::View & name, const ArrayView<CString::View> & args, const ArrayView <Writer::Field> & targs = {})
	{
		Assert(args.size == 6, kMissingArgName, ns, name);

		w.AddFunction(ns, name, DOC_RTN(RTN), { DOC_ARG(P1, args[0]), DOC_ARG(P2, args[1]), DOC_ARG(P3, args[2]), DOC_ARG(P4, args[3]), DOC_ARG(P5, args[4]), DOC_ARG(P5, args[5]) }, targs);
	}
};

template <class CLASS, class TYPE>
struct Docgen::Member<TYPE CLASS::*>
{
	static void Add(Writer & w, const CString::View & name)
	{
		w.AddMember(REFLEX_TYPEID(CLASS), DOC_ARG(TYPE, name));
	}
};

template <class C, class RTN>
struct Docgen::Method <RTN(C:: *)()>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), {});
	}
};

template <class C, class RTN>
struct Docgen::Method <RTN(C::*)() const>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), {}, {}, true);
	}
};

template <class C, class RTN, class P1>
struct Docgen::Method <RTN(C:: *)(P1)>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 1, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]) });
	}
};

template <class C, class RTN, class P1>
struct Docgen::Method <RTN(C:: *)(P1) const>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 1, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]) }, {}, true);
	}
};

template <class C, class RTN, class P1, class P2>
struct Docgen::Method <RTN(C:: *)(P1,P2)>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 2, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]), DOC_ARG(P2,args[1]) });
	}
};

template <class C, class RTN, class P1, class P2>
struct Docgen::Method <RTN(C:: *)(P1,P2) const>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 2, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]), DOC_ARG(P2,args[1]) }, {}, true);
	}
};

template <class C, class RTN, class P1, class P2, class P3>
struct Docgen::Method <RTN(C:: *)(P1, P2, P3)>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 3, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1, args[0]), DOC_ARG(P2, args[1]), DOC_ARG(P3, args[2]) }, {});
	}
};

template <class C, class RTN, class P1, class P2, class P3>
struct Docgen::Method <RTN(C:: *)(P1, P2, P3) const>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 3, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1, args[0]), DOC_ARG(P2, args[1]), DOC_ARG(P3, args[2]) }, {}, true);
	}
};

template <class C, class RTN, class P1, class P2, class P3, class P4>
struct Docgen::Method <RTN(C:: *)(P1, P2, P3, P4)>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 4, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1, args[0]), DOC_ARG(P2, args[1]), DOC_ARG(P3, args[2]), DOC_ARG(P4, args[3]) });
	}
};

template <class C, class RTN, class P1, class P2, class P3, class P4, class P5>
struct Docgen::Method <RTN(C:: *)(P1,P2,P3,P4,P5)>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 5, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]), DOC_ARG(P2,args[1]), DOC_ARG(P3,args[2]), DOC_ARG(P4,args[3]), DOC_ARG(P5,args[4]) });
	}
};

template <class C, class RTN, class P1, class P2, class P3, class P4, class P5, class P6>
struct Docgen::Method <RTN(C:: *)(P1,P2,P3,P4,P5,P6)>
{
	static void Add(Writer & w, const CString::View & name, const ArrayView<CString::View> & args)
	{
		Assert(args.size == 6, kMissingArgName, "", name);

		w.AddMethod(REFLEX_TYPEID(C), name, DOC_RTN(RTN), { DOC_ARG(P1,args[0]), DOC_ARG(P2,args[1]), DOC_ARG(P3,args[2]), DOC_ARG(P4,args[3]), DOC_ARG(P5,args[4]), DOC_ARG(P6,args[5]) });
	}
};

template <class C, class RTN, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
struct Docgen::Method <RTN(C:: *)(P1,P2,P3,P4,P5,P6,P7)>
{
	static void Add(Writer & w, const CString::View & fn, const ArrayView<CString::View> & args)
	{
		w.AddMethod(REFLEX_TYPEID(C), fn, DOC_RTN(RTN), { DOC_ARG(P1,args[0]), DOC_ARG(P2,args[1]), DOC_ARG(P3,args[2]), DOC_ARG(P4,args[3]), DOC_ARG(P5,args[4]), DOC_ARG(P6,args[5]), DOC_ARG(P7,args[6])});
	}
};

#endif
