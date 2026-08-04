#include "../include/docformat/data.h"




//
//

REFLEX_NOINLINE Reflex::CString Docformat::PrependNamespace(CString::View ns, CString::View symbol)
{
	return ns ? Join(ns, "::", symbol) : CString(symbol);
}

REFLEX_NOINLINE void Docformat::RegisterKeys(Data::PropertySet & data)
{
	auto keymap = Data::AcquireKeyMap(data);

	const char * keys[] =
	{
		"Symbol",
		"Category",
		"TypeFlags",
		"ParentID",
		"Name",
		"Namespace",
		"Index",
		"Indexed",
		"BaseID",
		"IsTemplateDefinition",
		"TemplateSourceID",
		"TypeID",
		"IsConst",
		"IsRef",
		"IsPointer",
		"Overloads",
		"Return",
		"Arguments",
		"IsStatic",
		"IsVirtual",
		"Constructors",
		"Methods",
		"Members",
		"TemplateArgs",
		"Data", // binary data, not indexable

		"enum",
		"value",
		"object",
	};

	for (auto & key : keys) Data::RegisterKey(keymap, key);
}

REFLEX_NOINLINE void Docformat::StoreField(Data::PropertySet & node, const Field & field)
{
	if (field.name) SetName(node, field.name);

	SetSymbol(node, field.type, kTypeID);

	if (field.is_const) Data::SetBool(node, kIsConst, true);

	if (field.is_ref) Data::SetBool(node, kIsRef, true);

	if (field.is_pointer) Data::SetBool(node, kIsPointer, true);
}

REFLEX_NOINLINE Docformat::Field Docformat::RestoreField(const Data::PropertySet & node)
{
	Field arg;

	arg.name = Data::GetCString(node, kName);
	arg.type = Reinterpret<Symbol>(Data::GetUInt64(node, kTypeID));
	arg.is_const = Data::GetBool(node, kIsConst);
	arg.is_ref = Data::GetBool(node, kIsRef);
	arg.is_pointer = Data::GetBool(node, kIsPointer);

	return arg;
}

REFLEX_NOINLINE void Docformat::PackFunctionSignatures(Data::PropertySet & data, ArrayView <FunctionSignature> in)
{
	constexpr Key32 kargs_targs[2] = { kArguments, kTemplateArgs };

	auto signatures = Data::AcquirePropertySetArray(data, kOverloads);

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

				for (auto & field : args) StoreField(Data::AddPropertySet(arguments), field);
			}
		}
	}
}

REFLEX_NOINLINE Reflex::Array <Docformat::FunctionSignature> Docformat::UnpackFunctionSignatures(const Data::PropertySet & data)
{
	constexpr Key32 kargs_targs[2] = { kArguments, kTemplateArgs };

	Array <FunctionSignature> rtn;

	for (auto & i : Data::GetPropertySetArray(data, kOverloads))
	{
		auto & node = *i;

		auto & signature = rtn.Push();

		signature.is_const = Data::GetBool(node, kIsConst);
		signature.is_virtual = Data::GetBool(node, kIsVirtual);

		signature.rtn = RestoreField(Data::GetPropertySet(i, kReturn));

		Array <Field> * parguments[] = { &signature.arguments, &signature.targs };

		REFLEX_LOOP(idx, 2)
		{
			if (auto args = Data::GetPropertySetArray(i, kargs_targs[idx]))
			{
				auto & arguments = *parguments[idx];

				for (auto & argument : args)
				{
					arguments.Push(RestoreField(argument));
				}
			}
		}
	}

	return rtn;
}

Reflex::Data::PropertySet Docformat::GetTypeConstructors(const Data::PropertySet & data)
{
	return Data::GetPropertySet(data, kConstructors);
}

Reflex::ArrayView <Docformat::Symbol> Docformat::GetTemplateArgs(const Data::PropertySet & data)
{
	return Reinterpret<ArrayView<Docformat::Symbol>>(Data::GetUInt64Array(data, kTemplateArgs));
}
