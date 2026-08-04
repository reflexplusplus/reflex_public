#include "pack.h"




//
//

REFLEX_NOINLINE CString Docgen::PrependNamespace(const CString::View & ns, const CString::View & symbol)
{
	return ns ? Join(ns, "::", symbol) : CString(symbol);
}

REFLEX_NOINLINE Docgen::Field Docgen::RestoreField(const Data::PropertySet & node)
{
	Field arg;

	arg.name = Data::GetCString(node, kName);

	arg.type = Reinterpret<Symbol>(Data::GetUInt64(node, kTypeID));

	arg.is_const = Data::GetBool(node, kIsConst);

	arg.is_ref = Data::GetBool(node, kIsRef);

	arg.is_pointer = Data::GetBool(node, kIsPointer);

	return arg;
}

REFLEX_NOINLINE decltype(Docgen::FunctionItem::overloads) Docgen::UnpackFunctionSignatures(const Data::PropertySet & data)
{
	decltype (FunctionItem::overloads) rtn;

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

Data::PropertySet Docgen::GetTypeConstructors(const Data::PropertySet & data)
{
	return Data::GetPropertySet(data, kConstructors);
}

ArrayView <Docgen::Symbol> Docgen::GetTemplateArgs(const Data::PropertySet & data)
{
	return Reinterpret<ArrayView<Docgen::Symbol>>(Data::GetUInt64Array(data, kTemplateArgs));
}
