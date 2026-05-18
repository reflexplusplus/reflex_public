#pragma once

#include "[require].h"




//
//Experimental API

REFLEX_NS(Reflex::VM)

struct Source
{
	UInt16 line = 0;
	UInt8 file = 0;
};

struct Error : public Object
{
	Error(const Source & source = {}, CString && stage = {}, CString && msg = {})
		: source(source),
		stage(std::move(stage)),
		msg(std::move(msg))
	{
	}

	Source source;

	CString stage;

	CString msg;
};

template <class TYPE> TYPE FAIL(const Source & src, const CString::View & stage, CString && msg);

extern const CString::View kErrorStageParse, kErrorStageCompile, kErrorStageLink, kErrorStageRuntime;

REFLEX_END




//
//

template <class TYPE = void> inline TYPE Reflex::VM::FAIL(const Source & src, const CString::View & stage, CString && msg)
{
	Error error;

	error.source = src;

	error.stage = stage;

	error.msg = std::move(msg);

	throw error;

	return TYPE(0);
}
