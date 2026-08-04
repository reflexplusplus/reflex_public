#pragma once

#include "../[require].h"




//
//Secondary API

namespace Reflex::Data
{

	struct Url
	{
		CString scheme;			//eg "https"		
		Pair <CString> user;	//eg { "user", pass" }
		CString domain;			//eg "www.domain.com"
		UInt16 port;			//0 = default
		CString resource;		//eg "page?arg1=a&arg2=b"
		CString fragment;		//eg "section2"
	};

	CString MakeUrl(const Url & url);

	Url SplitUrl(const CString::View & url);

	bool IsHttps(const CString::View & url);

	Pair <CString::View> SplitUrlResource(const CString::View & url);	//"page?arg1=a&arg2=b" -> { "page", "arg1=a&arg2=b" }


	CString EncodeUrlSegment(const CString::View & view);	//% encodes

	CString EncodeUrlSegment(const WString::View & view);

	CString DecodeUrlSegment(const CString::View & view);	//% decodes

}




//
//impl

inline Reflex::CString Reflex::Data::EncodeUrlSegment(const WString::View & view)
{
	return EncodeUrlSegment(Unpack<CString::View>(EncodeUTF8(view)));
}
