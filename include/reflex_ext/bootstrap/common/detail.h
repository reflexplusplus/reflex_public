#pragma once

//remove some windows macros that mess up reflex

#undef RGB
#undef near
#undef far

#include "[require].h"




//
//Detail

namespace Reflex::Bootstrap::Detail
{

	WString MakeProductPath(System::Path system_path, const CString::View & vendor, const CString::View & name, const WString::View & filename = {});

	constexpr WString::View kPrefsFilename = L"config";


	//entry helpers

	inline WString ExtractProjectDir(const char * filepath)
	{
		#if REFLEX_DEBUG
		//allow reloading of edited script files from local machine in debug mode
		return File::ResolveRelativePath(Join(File::SplitFilename(File::CorrectStrokes(ToWString(ToView(filepath)))).a, L"..", System::kPathDelimiter));
		#else
		return {};
		#endif
	}

}
