#pragma once

#include "../types.h"
#include "../detail/tokeniser.h"




//
//Primary API

namespace Reflex::Data
{

	void EncodeUTF8(Archive & output, const WString::View & text);

	Data::Archive EncodeUTF8(const WString::View & text);


	void DecodeUTF8(WString & output, const Archive::View & utf8);

	WString DecodeUTF8(const Archive::View & utf8);


	void EncodeUCS2(Archive & output, const WString::View & text);

	Data::Archive EncodeUCS2(const WString::View & text);


	void DecodeUCS2(WString & output, const Archive::View & ucs16);

	WString DecodeUCS2(const Archive::View & ucs16);


	void WriteLine(Archive & stream, const WString::View & line);		//encodes UTF8 + new line

	void WriteLine(Archive & stream, const CString::View & line = {});	//appends ascii + new line


	bool ReadLine(Archive::View & stream, WString & out);	//decodes UTF8

	bool ReadLine(Archive::View & stream, CString & out);	//decodes ascii

}




//
//impl

inline Reflex::Data::Archive Reflex::Data::EncodeUTF8(const WString::View & text)
{
	Archive a;

	EncodeUTF8(a, text);

	return a;
}

inline Reflex::WString Reflex::Data::DecodeUTF8(const Archive::View & utf8)
{
	WString rtn;

	DecodeUTF8(rtn, utf8);

	return rtn;
}

inline Reflex::Data::Archive Reflex::Data::EncodeUCS2(const WString::View & text)
{
	Archive a;

	EncodeUCS2(a, text);

	return a;
}

inline Reflex::WString Reflex::Data::DecodeUCS2(const Archive::View & ucs16)
{
	WString rtn;

	DecodeUCS2(rtn, ucs16);

	return rtn;
}

inline bool Reflex::Data::ReadLine(Archive::View & itr, WString & out)
{
	bool valid = True(itr);

	auto utf8_view = Detail::ReadLine(Reinterpret<CString::View>(itr));

	out.Clear();

	DecodeUTF8(out, Reinterpret<Archive::View>(utf8_view));

	return valid;
}

inline bool Reflex::Data::ReadLine(Archive::View & itr, CString & out)
{
	bool valid = True(itr);
	
	out = Detail::ReadLine(Reinterpret<CString::View>(itr));

	return valid;
}

