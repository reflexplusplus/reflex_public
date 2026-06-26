#pragma once

#include "global.h"





VM_WRAP_INTERNAL(GLX)

struct StyleSheetCircular : public ObjectOf <VM::Detail::Circular>
{
	StyleSheetCircular(VM::Context & context, GLX::StyleSheet & sheet)
		: ObjectOf<VM::Detail::Circular>(context, sheet),
		sheet(sheet)
	{
	}

	void OnReleaseData() override
	{
		sheet.ReleaseData();
	}

	GLX::StyleSheet & sheet;
};

REFLEX_END_INTERNAL
