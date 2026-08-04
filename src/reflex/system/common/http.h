#pragma once

#include "[require].h"




//
//declarations

REFLEX_NS(Reflex::System::Common)

CString MakeUrlBase(bool https, const CString::View & domain, UInt port);	//returns eg "https://www.website.com:8000/"

REFLEX_END
