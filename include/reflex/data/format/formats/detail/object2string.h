#pragma once

#include "../../../types.h"




//
//Detail

REFLEX_NS(Reflex::Data::Detail)

using ObjectToStringFn = FunctionPointer <CString(const KeyMap&, const Object&)>;

CString Float64ToString(Float64 value, UInt max_precision);	//auto precision

CString BoolToString(const KeyMap & keymap, const Object & objectof_bool);

CString Int32ToString(const KeyMap & keymap, const Object & objectof_int32);

CString Int64ToString(const KeyMap & keymap, const Object & objectof_int32);

CString UInt32ToString(const KeyMap & keymap, const Object & value);

CString UInt64ToString(const KeyMap & keymap, const Object & value);

CString Float32ToString(const KeyMap & keymap, const Object & value);

CString Float64ToString(const KeyMap & keymap, const Object & value);

CString Key32ToString(const KeyMap & keymap, const Object & key);

CString BinaryToString(const KeyMap & keymap, const Object & value);

CString CStringToQuotedString(const KeyMap & keymap, const Object & cstring_object);

CString WStringToQuotedString(const KeyMap & keymap, const Object & wstring_object);

REFLEX_END
