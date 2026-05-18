#pragma once

#include "types.h"




//
//Primary API

namespace Reflex::System
{

	class DiskIterator;

}




//
//DiskIterator

class Reflex::System::DiskIterator : public Object
{
public:

	REFLEX_OBJECT(System::DiskIterator, Object);

	static DiskIterator & null;



	//lifetime

	[[nodiscard]] static TRef <DiskIterator> Create();



	//interface

	virtual bool GetNext(bool & removable, WString & filename, WString & display_name) = 0;

};
