#pragma once

#include "warning.h"




//
//Secondary API

namespace Reflex
{

	enum OutputFlags
	{
		kOutputQueue = 1,
		kOutputConsole = 2,
		kOutputFile = 4,
	};

	enum LogType : UInt8
	{
		kLogNormal = 0,
		kLogWarning = 1,
		kLogError = 2,
	};

	constexpr CString::View kSpace = " ";

}
