#pragma once

#include "model.h"




// 
//Writer

class Docgen::Writer : public Object
{
public:

	using Field = AbstractField <TypeID>;



	//lifetime

	static Reflex::TRef <Writer> Create(Key32 language);



	//write

	virtual Key32 RegisterString(CString && string) = 0;

	virtual const TypeItem * AddType(Symbol symbol, TypeID type_id, TypeID base_type_id, TypeID src_template_type_id, UInt16 type_flags, const CString::View & ns, const CString::View & name, const ArrayView <TypeID> & targs = {}) = 0;

	virtual void AddTypedef(TypeID type_id, const CString::View & ns, const CString::View & name) = 0;

	virtual void AddGlobal(const CString::View & ns, Field variable) = 0;

	virtual void AddFunction(const CString::View & ns, const CString::View & fn, Field rtn, const ArrayView <Field> & args, const ArrayView <Field> & targs = {}) = 0;

	virtual void AddMember(TypeID class_type, Field variable) = 0;

	virtual void AddMethod(TypeID class_type, const CString::View & fn, Field rtn, const ArrayView <Field> & args, const ArrayView <Field> & targs = {}, bool is_const = false, bool is_virtual = false) = 0;



	//info

	virtual CString::View GetString(Key32 key) const = 0;

	virtual const Item * GetItem(Symbol symbol) const = 0;

	virtual const TypeItem * GetType(TypeID type_id) const = 0;

	virtual TypeID GetTypeID(Symbol symbol) const = 0;


	virtual void EnumerateSymbols(const Function <void(const Item & item)> & visitor) const = 0;


	const Key32 language;



protected:

	Writer(Key32 language);

};
