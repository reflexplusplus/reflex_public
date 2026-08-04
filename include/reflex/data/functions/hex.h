#pragma once

#include "../[require].h"




//
//declarations

namespace Reflex::Data
{

	CString BytesToHex(const Archive::View & bytes);

	Archive HexToBytes(const CString::View & hex);

}





//
//impl

REFLEX_NS(Reflex::Data::Detail)

void BytesToHex(const CString::Region & output, const Archive::View & bytes);	//client responsibility to ensure output size is correct

void HexToBytes(const Archive::Region & output, const CString::View & hex);

REFLEX_END

REFLEX_NS(Reflex::Data)

inline CString BytesToHex(const Archive::View & bytes)
{
	CString rtn;

	rtn.SetSize(bytes.size * 2);

	Detail::BytesToHex(rtn, bytes);

	return rtn;
}

inline Archive HexToBytes(const CString::View & hex)
{
	REFLEX_ASSERT((hex.size & 1) == 0);

	Archive rtn;

	rtn.SetSize(hex.size / 2);

	Detail::HexToBytes(rtn, hex);

	return rtn;
}

REFLEX_END
