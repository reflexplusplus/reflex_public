#pragma once

#include "../[require].h"




//
//

REFLEX_NS(Reflex::VM)

TRef <Data::PropertySet> CreatePropertySet(Context & context);

TRef <ValueArray> CreateByteArray(const Data::Archive::View & bytes);

void Append(ValueArray & archive, const Data::Archive::View & bytes);


TRef <Data::PropertySet> ImportPropertySet(Context & context, const Data::PropertySet & native);	//converts all nodes to VM impl (resolves circulars, does NOT convert types)

void ImportPropertySets(Context & context, const ArrayView < ConstReference <Data::PropertySet> > & native, TRef <Data::PropertySet> * output);	//converts all nodes to VM impl (resolves circulars)

TRef <ObjectArray> ImportPropertySets(Context & context, const ArrayView < ConstReference <Data::PropertySet> > & native, TypeRef array_t);	//converts all nodes to VM impl (resolves circulars)

REFLEX_END
